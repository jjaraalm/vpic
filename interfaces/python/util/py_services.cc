#include "py_services.h"

struct scoped_flag {
  scoped_flag(bool * flag) : flag(flag) { *flag = true; }
  ~scoped_flag() { *flag = false; }
  bool * flag;
};

namespace vpic {

  //============================================================================
  // Checkpoint/Restore methods
  //
  // Checkpointing/restore of C++ objects uses the normal VPIC machinery.
  // Additional data is present on python objects which we need to capture
  // and store as well. This includes user data which can be set as instance
  // attributes on Simulation or its subclasses, as well as the actual class
  // or subclass of Simulation that is being run. To store this data, we use
  // python pickling to translate live objects to/from a string. This string
  // is then stored in the global variable `pickle_str` and checkpointed and
  // restored using the VPIC machinery. Guards are used to ensure that
  // pickling can only be done during checkpoint and restore and not at other
  // times.

  bool _in_checkpt = false;
  bool _in_restore = false;
  std::string pickle_str;

  void
  checkpt_pickle_str(const void * ptr) {
    CHECKPT_STR(pickle_str.c_str());
  }

  void *
  restore_pickle_str() {
    char * tmp;
    RESTORE_STR(tmp);
    pickle_str = std::string(tmp);
    FREE(tmp);
    return nullptr;
  }

  void
  checkpt(py::object pysim, std::string filename) {
    SCOPED_STREAM_CAPTURE(DEVNULL);
    scoped_flag flag(&_in_checkpt);

    pickle_str = py::module::import("pickle").attr("dumps")(pysim).cast<std::string>();
    set_active_registry(pysim.cast<Simulation&>().vsim->checkpt_registry_id);
    checkpt_objects(filename.c_str());

  }

  py::object
  restore(std::string filename) {
    SCOPED_STREAM_CAPTURE(DEVNULL);
    scoped_flag flag(&_in_restore);

    pickle_str = "";
    restore_objects(filename.c_str());
    mp_barrier();
    reanimate_objects();
    mp_barrier();
    return py::module::import("pickle").attr("loads")(pickle_str);

  }

  bool
  in_checkpt() {
    return _in_checkpt;
  }

  bool
  in_restore() {
    return _in_restore;
  }

  //============================================================================
  // Module boot/halt
  //
  // This copies src/util/boot.cc logic, however since there is freedom
  // in python, the ordering cannot actually be guaranteed. For example,
  // the code
  //
  // >>> import vpic
  // >>> from mpi4py import COMM
  // >>> vpic.boot()
  //
  // will *always* boot MPI first and there is nothing we can do about it.

  void
  boot(int thread_pipelines,
       int serial_pipelines,
       bool dispatch_to_host) {

    SCOPED_STREAM_CAPTURE(DEVNULL);

    if (vpic::booted())
      throw std::runtime_error("VPIC has already been booted.");

    if (thread_pipelines < 1)
      throw std::runtime_error("Number of threads must be >= 1.");

    if (serial_pipelines < 1)
      throw std::runtime_error("Number of serial pipelines must be >= 1.");

    boot_checkpt(NULL, NULL);

    #if defined(VPIC_USE_PTHREADS) && !defined(VPIC_SWAP_MPI_PTHREAD_INIT)

      thread_pipelines = boot_threads(serial_pipelines,
                                      thread_pipelines,
                                      dispatch_to_host);
      boot_mp( NULL, NULL );      // Boot communication layer last.

    #else

      boot_mp( NULL, NULL );      // Boot communication layer first.
      thread_pipelines = boot_threads(serial_pipelines,
                                      thread_pipelines,
                                      dispatch_to_host);
    #endif

    mp_barrier();
    _boot_timestamp = 0;
    _boot_timestamp = uptime();

    _in_checkpt = false;
    _in_restore = false;
    REGISTER_OBJECT(&pickle_str, checkpt_pickle_str, restore_pickle_str, nullptr);

  };

  void
  halt() {
    if( booted() ) {
      set_active_registry( 0 );
      UNREGISTER_OBJECT(&pickle_str);
      halt_services();
    }
  };

  bool
  booted() {
    return _boot_timestamp > 0;
  }

}


void declare_services(py::module &m) {

  // Class methods
  m.def("boot", &vpic::boot,
    R"pydoc(
      Startup VPIC services.

      Parameters
      ----------
      threads : int
        Number of threads per rank to start.
      serial_pipelines : int
        Number of serial pipelines per rank to start.
      dispatch_to_host : bool
        Whether to use the host thread to process remaining elements.

      Notes
      -----
      VPIC services must be started before creating Simulation instances or
      using any other VPIC functionality.
    )pydoc",
    py::arg("threads") = 1,
    py::arg("serial_pipelines") = 1,
    py::arg("dispatch_to_host") = true);

  m.def("uptime", [](){
      if (!vpic::booted())
        return 0.0;
      return uptime();
    },
    R"pydoc(
      Returns the walltime in seconds that VPIC has been running. This
      is globally agreed on by all MPI ranks. Due to the communitcation
      cost, use sparingly.
    )pydoc");

}
