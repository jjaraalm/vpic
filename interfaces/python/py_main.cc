#include "py_main.h"

PYBIND11_MODULE(VPIC_PYMOD_NAME, m) {

  declare_services(m);
  declare_datatypes(m);
  declare_grid(m);
  declare_boundary_conditions(m);
  declare_materials(m);
  declare_species(m);
  declare_simulation(m);
  declare_lists(m);

  //============================================================================
  // Module startup.
  //============================================================================

  // Initialize the boot timestamp and simulation pointer.
  _boot_timestamp = -1;

  // Register a callback function that is invoked on exit and when all
  // Simulation objects have been destroyed.
  py::cpp_function cleanup_callback(
    [](py::handle weakref) {
        vpic::halt();
        weakref.dec_ref();
    }
  );

  // Create a weak reference with a cleanup callback and initially leak it
  (void) py::weakref(m.attr("Simulation"), cleanup_callback).release();

}
