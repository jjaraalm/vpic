#ifndef _py_grid_h_
#define _py_grid_h_

#include "../util/py_util.h"
#include "../vpic/py_simulation.h"

#define RANK_TO_INDEX(rank,gpx,gpy,gpz,ix,iy,iz) do {   \
    int _ix, _iy, _iz;                                  \
    _ix  = (rank);  /* ix = ix + gpx*( iy + gpy*iz ) */ \
    _iy  = _ix/gpx; /* iy = iy + gpy*iz */              \
    _ix -= _iy*gpx; /* ix = ix */                       \
    _iz  = _iy/gpy; /* iz = iz */                       \
    _iy -= _iz*gpy; /* iy = iy */                       \
    (ix) = _ix;                                         \
    (iy) = _iy;                                         \
    (iz) = _iz;                                         \
  } while(0)


class SimulationGrid {
public:

  SimulationGrid(
    Simulation &simulation,
    grid_t * g
  ) : simulation(simulation), g(g) {}

  ~SimulationGrid() {}

  Simulation &simulation;
  grid_t * g;

};

// Entrypoint for python declerations
void declare_grid(py::module &m);

#endif
