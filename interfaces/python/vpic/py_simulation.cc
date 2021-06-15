#include "py_simulation.h"
#include "py_lists.h"
#include "../util/py_services.h"
#include "../grid/py_grid.h"
#include "../py_deck.h"

// Constructor and destructor register simulations ids.

Simulation::Simulation()
: initialized(false), progress_bar(py::none()) {

  vsim = new vpic_simulation();

  // Do some nice initialization for users.
  vsim->define_units(1, 1);    // Default: natural units.

  // Default materials
  double inf = std::numeric_limits<double>::infinity();
  vsim->define_material("vacuum", 1);
  vsim->define_material("perfect_conductor", 1, 1, inf, 0);

}

Simulation::Simulation(vpic_simulation * vsim)
: vsim(vsim), initialized(true), progress_bar(py::none()) {

}

Simulation::~Simulation() {
  set_active_registry(vsim->checkpt_registry_id);
  delete vsim;
}


bool
Simulation::advance(int          steps,
                    std::string  logfile,
                    int          progress_interval) {

  int current_step;
  bool completed = false;

  // Sanity check.
  if (!vpic::booted())
    throw std::runtime_error("VPIC has not been booted.");

  // Get rank and disable logging/progress on all but rank 0.
  if (vsim->rank() != 0) {
    logfile = "";
    progress_interval = 0;
  }

  // Redirect VPIC output to logfiles.
  if (logfile.size() == 0)
    logfile = DEVNULL;
  SCOPED_STREAM_CAPTURE(logfile);

  // Capture callbacks
  update_user_functions(this, vsim);

  // Initialize.
  if (!initialized) {
    vsim->initialize(0, NULL);
    initialized = true;
  }

  // Create a progress bar.
  if (progress_interval) {
    py::object max_value;

    auto pbar = py::module::import("progressbar");

    if (vsim->num_step)
      max_value = py::cast(vsim->num_step);
    else
      max_value = pbar.attr("UnknownLength");

    if(progress_bar.is_none()) {
      // py::list widgets = py::cast<py::list>(pbar.attr('default_widget')());
      // widgets.append()

      progress_bar = pbar.attr("ProgressBar")();
    }

    py::setattr(progress_bar, "max_value", max_value);
    progress_bar.attr("update")(vsim->step());

  }

  // Run.
  do {
    py::gil_scoped_release release_gil;

    // Exit on ctrl-c by throwing an error. Requries the GIL, should we subcylce?
    {
      py::gil_scoped_acquire acquire_gil;
      if(PyErr_CheckSignals() != 0) {
        throw py::error_already_set();
      }
    }

    // Exit quietly when we have completed the number of steps specified
    if (steps == 0) {
      return false;
    }

    // Advance
    completed = !vsim->advance();
    current_step = vsim->step();

    if (steps > 0)
      --steps;

    // Update the progress bar. Requries the GIL.
    if (progress_interval && (current_step % progress_interval == 0) || completed) {
      py::gil_scoped_acquire acquire_gil;
      progress_bar.attr("update")(current_step, true);
    }

  } while(!completed);

  // Exit on completion.
  initialized = false;
  vsim->finalize();
  return true;

}

//==============================================================================
// Unit system interface
//==============================================================================

#define GET(FIELD) [](Simulation& s){ return s.vsim->FIELD; }
#define SET(FIELD, TYPE) [](Simulation& s, TYPE value){ s.vsim->FIELD = value; }
#define CALL(METHOD) [](Simulation& s){ return s.vsim->METHOD(); }

void declare_simulation(py::module &m) {

  //============================================================================
  // Simulation class
  //============================================================================

  py::class_<Simulation>(m, "Simulation", py::dynamic_attr())

    .def(py::init<>(), "A VPIC simulation.")

    // System lifecycle
    .def("define_units", [](Simulation &s, double cvac, double eps0) {
        s.vsim->define_units(cvac, eps0);
      },
      R"pydoc(
      Sets the internal unit system.

      Parameters
      ----------
      cvac : float
        The speed of light.
      eps0 : float
        The vacuum permittivity.
      )pydoc",
      py::arg("cvac") = 1.,
      py::arg("eps0") = 1.)

    .def("define_timestep", [](Simulation &s, double timestep, double start, double step) {
        s.vsim->define_timestep(timestep, start, step);
      },
      R"pydoc(
        Sets the timestep, starting time, and current step.

        Parameters
        ----------
        timestep : float
          The simulation timestep.
        start : float
          The simulation start time.
        step : int
          The current step.
      )pydoc",
      py::arg("timestep"),
      py::arg("start") = 0,
      py::arg("step") = 0)

    .def("define_field_array", [](Simulation &s, double damp) {
        s.vsim->define_field_array(NULL, damp);
      },
      R"pydoc(
        Setup the fields.

        Parameters
        ----------
        damp : double
          Radiation damping parameter. Set to 0 to disable (default).

        Notes
        -----
        Fields must be explicitly setup by calling this method after all
        materials have been defined, but before injecting particles.
      )pydoc",
      py::arg("damp") = 0)

    //==========================================================================
    // Simulation lifecycle
    //==========================================================================

    .def("advance", &Simulation::advance,
      R"pydoc(
        Runs the simulation for the given number of step or until completion.

        Parameters
        ----------
        steps : int
          The number of steps to run. If ``num_steps`` < 0, then the
          simulation will run to completion.
        logfile : str
          The output file to write VPIC logs to. If empty, logs won't be saved.
        progress_interval : int
          How often (in steps) to update the progress bar and ETA to completion.
          Set to 0 to disable the progress bar.

        Returns
        -------
        completed : bool
          Returns ``True`` if the simulation exited due to completion,
          otherwise returns ``False``.

      )pydoc",
      py::arg("steps") = -1,
      py::arg("logfile") = "",
      py::arg("progress_interval") = 100)

    //==========================================================================
    // Properties and interfaces
    //==========================================================================

    // Basic properties
    .def_property("dt",
      [](Simulation &s) { return s.vsim->grid->dt; },
      [](Simulation &s, double dt) {
        s.vsim->define_timestep(dt, s.vsim->grid->t0, s.vsim->grid->step);
      },
      "The timestep.")

    .def_property_readonly("rank", CALL(rank),
                           "The global MPI rank number of this domain.")

    .def_property_readonly("nproc", CALL(nproc),
                           "Total number of MPI ranks in use.")

    .def_property_readonly("step", CALL(step),
                           "The current simulation timestep.")

    .def_property_readonly("time", CALL(time),
                           "The current simulation time.")

    .def_property("clean_div_b_interval", GET(clean_div_b_interval), SET(clean_div_b_interval, int),
                   "How often (in steps) to clean magnetic field divergence.")

    .def_property("clean_div_e_interval", GET(clean_div_e_interval), SET(clean_div_e_interval, int),
                   "How often (in steps) to clean electric field divergence.")

    .def_property("num_div_b_round", GET(num_div_b_round), SET(num_div_b_round, int),
                   "Number of rounds of div b cleaning to perform per interval.")

    .def_property("num_div_e_round", GET(num_div_e_round), SET(num_div_e_round, int),
                   "Number of rounds of div e cleaning to perform per interval.")

    .def_property("sync_shared_interval", GET(sync_shared_interval), SET(sync_shared_interval, int),
                   "How often to synchronize shared faces.")

    .def_property("status_interval", GET(status_interval), SET(status_interval, int),
                   "How often to log status messages, if enabled.")

    .def_property("num_comm_round", GET(num_comm_round), SET(num_comm_round, int),
                   "Number of communication rounds when passing particles. "
                   "Should be at least 3 always.")

    .def_property("num_step", GET(num_step), SET(num_step, int),
                   "Maximum number of steps to take before completion.")

    .def(py::pickle(
      [](py::object self) {
        if(!vpic::in_checkpt())
          throw std::runtime_error("Simulations cannot be pickled, use checkpt instead.");
        return self.attr("__dict__").cast<py::dict>();
      },
      [](py::dict state) {
        if(!vpic::in_restore())
          throw std::runtime_error("Simulations cannot be unpickled, use restore instead.");
        return std::make_pair(
          Simulation((vpic_simulation *) object_ptr(1)),
          state
        );
      }
    ))


    //==========================================================================
    // Lists
    // FIMXE: Add docstrings
    //==========================================================================

    .def_property("materials", &MaterialList::get_list, &MaterialList::set_list)
    .def_property("species", &SpeciesList::get_list, &SpeciesList::set_list)
    .def_property("particle_bcs",  &ParticleBCList::get_list,  &ParticleBCList::set_list)

    // .def_property("emitters",      &EmitterList::get_list,     &EmitterList::set_list)
    // .def_property("collision_ops", &CollisionOpList::get_list, &CollisionOpList::set_list)

    //==========================================================================
    // Grid and field access
    // FIMXE: Add docstrings
    //==========================================================================

    .def_property_readonly("grid", [](Simulation &s) {
      return SimulationGrid(s, s.vsim->grid);
    })

    .def_property_readonly("field", [](Simulation &s) -> py::object {
      if (!s.vsim->field_array)
        return py::none();
      return numpy_view(s.vsim->grid, s.vsim->field_array->f);
    });

}
