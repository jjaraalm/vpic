#include "py_deck.h"

// Trampoline to redirect callbacks into python.
//
// This is a little different from the canonical pybind trampoline because
// the underlying methods are not virtual, but instead are incomplete. Here
// we just complete them using a macro based on the pybind docs..
//
// In the python implementation, we do not use "user_initialization"
// so that we can use the python __init__ method or scripting instead.


#define PY_INVOKE(METHOD) do {                                  \
  if (global(this)->py_##METHOD) {                              \
    py::gil_scoped_acquire gil;                                 \
    global(this)->py_##METHOD();                                \
  }                                                             \
} while(0)

void
update_user_functions(Simulation * py_sim, vpic_simulation * vsim) {

  global(vsim)->py_particle_injection = py::get_overload(py_sim, "particle_injection");
  global(vsim)->py_current_injection = py::get_overload(py_sim, "current_injection");
  global(vsim)->py_field_injection = py::get_overload(py_sim, "field_injection");
  global(vsim)->py_diagnostics = py::get_overload(py_sim, "diagnostics");
  global(vsim)->py_particle_collisions = py::get_overload(py_sim, "particle_collisions");

}

void vpic_simulation::user_initialization( int argc, char **argv ) {

}

void vpic_simulation::user_particle_injection() {
  PY_INVOKE(particle_injection);
}

void vpic_simulation::user_current_injection() {
  PY_INVOKE(current_injection);
}

void vpic_simulation::user_field_injection() {
  PY_INVOKE(field_injection);
}

void vpic_simulation::user_diagnostics() {
  PY_INVOKE(diagnostics);
}

void vpic_simulation::user_particle_collisions() {
  PY_INVOKE(particle_collisions);
}

