#ifndef _py_proto_h_
#define _py_proto_h_

#include "py_simulation.h"

// Before attaching an object to a list, we need to create it. However,
// instantiated but unattached objects are dangling and will emit warnings
// from the checkpoint machinery (and lead to ugly APIs). Here we define a
// VPICPrototypeObject wrapper that allows us to defer creation until items
// are explicitly attached to a list.

template<class T>
struct VPICPrototypeObject {
  using Func = std::function<void(vpic_simulation *)>;

  VPICPrototypeObject(Func proto) : proto(proto) {}
  ~VPICPrototypeObject() {}

  void operator()(vpic_simulation * vsim) { proto(vsim); }
  Func proto;
};

typedef VPICPrototypeObject<species_t*> PrototypeSpecies;
typedef VPICPrototypeObject<emitter_t*> PrototypeEmitter;
typedef VPICPrototypeObject<material_t*> PrototypeMaterial;
typedef VPICPrototypeObject<particle_bc_t*> PrototypeParticleBC;
typedef VPICPrototypeObject<collision_op_t*> PrototypeCollisionOp;

#endif
