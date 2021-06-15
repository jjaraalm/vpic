#ifndef _py_boundary_h_
#define _py_boundary_h_

#include "../util/py_util.h"
#include "../vpic/py_lists.h"

// We need to complete the particle_bc_t type to make pybind happy.
#define IN_boundary
#include <boundary/boundary_private.h>


// Helper classes
struct ReflectPartricles : public PrototypeParticleBC {
  using PrototypeParticleBC::PrototypeParticleBC;
};

struct AbsorbParticles : public PrototypeParticleBC {
  using PrototypeParticleBC::PrototypeParticleBC;
};

struct MaxwellianReflux : public PrototypeParticleBC {
  using PrototypeParticleBC::PrototypeParticleBC;
};

struct AbsorbTally : public PrototypeParticleBC {
  using PrototypeParticleBC::PrototypeParticleBC;
};

// Entrypoint for python declarations
void declare_boundary_conditions(py::module &m);

#endif

