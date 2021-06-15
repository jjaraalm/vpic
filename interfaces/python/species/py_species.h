#ifndef _py_species_h_
#define _py_species_h_

#include "../util/py_util.h"
#include "../util/py_services.h"
#include "../vpic/py_lists.h"

// Helper class for delegating array access
class PyParticleArray {
public:

  PyParticleArray(vpic_simulation * vsim, species_t * sp) : vsim(vsim), sp(sp) {}

  int size() {
    return sp->np;
  }

  void sort() {
    if(sp->g->step != sp->last_sorted)
    sort_p(sp);
  }

  // Get some range of particles.
  py::array_t<particle_t> get_slice(py::slice slice) {
    size_t start, stop, step, len;

    if (!slice.compute(sp->np, &start, &stop, &step, &len))
      throw py::error_already_set();

    return py::array_t<particle_t>(
      {len},
      {step*sizeof(particle_t)},
      sp->p + start,
      NOCOPY(sp->p)
    );
  }

  // Get particles in a specific voxel
  py::array_t<particle_t> get_voxel(std::tuple<int, int, int> voxel) {
    grid_t *g = sp->g;
    int v = (g->sz * X(voxel) + g->sy * Y(voxel) + g->sx * Z(voxel));
    sort();
    return get_slice({sp->partition[v], sp->partition[v+1], 1});
  }

  // Get the particle partition
  py::array_t<int> get_partition() {
    sort();
    return numpy_view(sp->g, sp->partition);
  }

  // Inject particles.
  void inject(
    double x,  double y,  double z,
    double ux, double uy, double uz,
    double weight, double age, bool update_rhob
  ) {
    vsim->inject_particle(sp, x, y, z, ux, uy, uz, weight, age, update_rhob);
  }
  // decltype(auto) inject(
  //   py::array_t<double> x,  py::array_t<double> y,  py::array_t<double> z,
  //   py::array_t<double> ux, py::array_t<double> uy, py::array_t<double> uz,
  //   py::array_t<double> weight, py::array_t<double> age, bool update_rhob = true
  // ) {

  //   vpic_simulation * vsim = this->vsim;
  //   species_t * sp = this->sp;
  //   return py::vectorize([vsim, sp, update_rhob](
  //       double x,  double y,  double z,
  //       double ux, double uy, double uz,
  //       double weight, double age
  //     ) -> bool {
  //       auto np = sp->np;
  //       vsim->inject_particle(sp, x, y, z, ux, uy, uz, weight, age, update_rhob);
  //       return sp->np == np;
  //     }
  //   );

  // }

  vpic_simulation * vsim;
  species_t * sp;

};

// Helper class for species access
class PySpecies {
public:

  PySpecies(vpic_simulation * vsim, species_t * sp) : vsim(vsim), sp(sp) {}

  PyParticleArray get_particles() { return PyParticleArray(vsim, sp); }
  py::object get_hydro() { return numpy_view(sp->g, vsim->compute_hydro(sp)); }

  vpic_simulation * vsim;
  species_t * sp;

};

// Entrypoint for python declerations
void declare_species(py::module &m);

#endif
