#ifndef _py_simulation_h_
#define _py_simulation_h_

#include "../util/py_util.h"

// This feels silly and unnecessary to have a wrapper here, but there is
// meaning to this madness. Suppose a user subclasses Simulation in python.
// This work fine, but then we checkpoint. At checkpoint, we upcast to
// Simulation and so when we resart, we load a Simulation. Now we need to
// downcast. By checkpoint __class__, we can dynamically load the target
// python class. However, this python class does not have an accessible C++
// representation so we can't do a simple C++ cast. One alternative is to
// add a static method to Simulation and invoke the inherited method, but
// this would require manually copying all fields from vpic_simulation
// ... not good. Instead, by making a wrapper, we only need copy in the
// restored pointer.

// Inner class. The whole point of this class is just to modify access
// from protected to public.

// Class definition for the main Simulation class

class Simulation {
public:

  Simulation();
  Simulation(vpic_simulation * vsim);
  ~Simulation();

  // Pointer to the underlying simulation
  vpic_simulation * vsim;

  // AAdditional atributes
  bool initialized;
  py::object progress_bar;

  // Methods
  bool advance(int steps, std::string logfile, int progress_interval);
  static py::object restore(py::object cls, std::string filename);

};

// Entrypoint for python declarations.
void declare_simulation(py::module &m);

#endif
