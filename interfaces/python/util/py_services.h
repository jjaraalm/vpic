#ifndef _py_services_h_
#define _py_services_h_

#include "py_util.h"
#include "../vpic/py_simulation.h"

// Module-scoped service functions
namespace vpic {

  void checkpt(py::object pysim, std::string filename);
  py::object restore(std::string filename);
  bool in_checkpt();
  bool in_restore();


  void boot(int thread_pipelines,
            int serial_pipelines,
            bool dispatch_to_host);

  void halt();

  bool booted();


}

// Entrypoint for python declerations
void declare_services(py::module &m);

#endif
