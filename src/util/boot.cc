#include "util.h"

#include "stdio.h"

double _boot_timestamp = 0;

double
uptime( void ) {
  double local_time = wallclock(), time_sum;
  mp_allsum_d( &local_time, &time_sum, 1 );
  return time_sum/(double)world_size - _boot_timestamp;
}

int
boot_threads( int serial_pipelines,
              int thread_pipelines,
              int dispatch_to_host ) {

#if defined(VPIC_USE_PTHREADS)

  serial.boot(serial_pipelines, dispatch_to_host);
  thread.boot(thread_pipelines, dispatch_to_host);
  return thread.n_pipeline;

#elif defined(VPIC_USE_OPENMP)

  omp_helper.boot(thread_pipelines, dispatch_to_host);
  return omp_get_num_threads();

#else

  #error "Either VPIC_USE_PTHREADS or VPIC_USE_OPENMP must be defined."

#endif

}

void
halt_threads() {

#if defined(VPIC_USE_PTHREADS)

    thread.halt();
    serial.halt();

#endif

}

void
boot_services( int * pargc,
               char *** pargv ) {

  // Start up the checkpointing service.  This should be first.

  boot_checkpt( pargc, pargv );

  // Start up the threads.  Note that some MPIs will bind threads to
  // cores if threads are booted _after_ MPI is initialized.  So we
  // start up the pipeline dispatchers _before_ starting up MPI.

  // FIXME: The thread utilities should take responsibility for
  // thread-core affinity instead of leaving this to chance.

  // Boot up the communications layer

  detect_old_style_arguments(pargc, pargv);
  int tpp      = strip_cmdline_int( pargc, pargv, "--tpp",               1 );
  int serial   = strip_cmdline_int( pargc, pargv, "--serial.n_pipeline", 1 );
  int dispatch = strip_cmdline_int( pargc, pargv, "--dispatch_to_host",  1 );
  int num_threads;

#if defined(VPIC_USE_PTHREADS) && !defined(VPIC_SWAP_MPI_PTHREAD_INIT)

  num_threads = boot_threads(serial, tpp, dispatch);
  boot_mp( pargc, pargv );      // Boot communication layer last.

#else

  boot_mp( pargc, pargv );      // Boot communication layer first.
  num_threads = boot_threads(serial, tpp, dispatch);

#endif

  // Set the boot_timestamp

  mp_barrier();
  _boot_timestamp = 0;
  _boot_timestamp = uptime();

  if (_world_rank == 0)
  {
      printf("Booting with %d threads (pipelines) and %d (MPI) ranks \n",
              num_threads, _world_size);
  }
}

// This operates in reverse order from boot_services

void
halt_services( void )
{
  _boot_timestamp = 0;

#if defined(VPIC_USE_PTHREADS) && !defined(VPIC_SWAP_MPI_PTHREAD_INIT)

  halt_mp();
  halt_threads();

#else

  halt_threads();
  halt_mp();

#endif

  halt_checkpt();

}
