#ifndef _py_deck_h_
#define _py_deck_h_

#include "util/py_util.h"
#include "vpic/py_simulation.h"

#define global(obj) ((struct user_global_t *)obj->user_global)

struct user_global_t {
  py::function py_particle_injection;
  py::function py_current_injection;
  py::function py_field_injection;
  py::function py_diagnostics;
  py::function py_particle_collisions;
  size_t pickle_id;
};

// Updates any defined python callbacks. We don't do this every step to
// avoid unnecssary translation costs.

void
update_user_functions(Simulation * py_sim, vpic_simulation * vsim);

#endif
