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


  struct scoped_acquire_registry {
    size_t id;

    scoped_acquire_registry(vpic_simulation * vsim) {
      id = get_active_registry();
      set_active_registry(vsim->checkpt_registry_id);
    }

    ~scoped_acquire_registry() {
      set_active_registry(id);
    }

  };

}

// Entrypoint for python declerations
void declare_services(py::module &m);

#endif
