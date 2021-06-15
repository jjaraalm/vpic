#include "py_species.h"

#define GET(FIELD) [](PySpecies& s){ return s.sp->FIELD; }

void declare_species(py::module &m_vpic) {

  // Create a species submodule
  py::module m = m_vpic.def_submodule("species",
    R"pydoc(
    VPIC particle species definitions.

    Particle species organize computational particles based on their physical
    properties. All particles in a species have the same physical charge,
    ``q``, and the same physical mass, ``m``, but may have distinct
    statistical weights, ``w``.
    )pydoc");

  //============================================================================
  // RegisteredSpecies class
  //============================================================================

  py::class_<PySpecies>(m, "RegisteredSpecies")
    .def_property_readonly("name", GET(name), "Species name")
    .def_property_readonly("id", GET(id), "Species id")
    .def_property_readonly("q", GET(q), "Species charge")
    .def_property_readonly("m", GET(m), "Species mass")
    .def_property_readonly("np", GET(np), "Number of particles in this domain")
    .def_property_readonly("nm", GET(nm), "Number of active movers in this domain")
    .def_property_readonly("max_np", GET(max_np), "Maximum number of particles allowed in this domain")
    .def_property_readonly("max_nm", GET(max_nm), "Maximum number of movers allowed in this domain")
    .def_property_readonly("sort_interval", GET(sort_interval), "How often (in steps) particles are sorted")
    .def_property_readonly("sort_out_of_place", GET(sort_out_of_place), "Sort method")
    .def_property_readonly("last_sorted", GET(last_sorted), "Step when particles were last sorted")

     // Particle interface. Return a ParticleArray to handle random
     // or voxel-based access as well as injection and deletion.
     .def_property_readonly("particles", &PySpecies::get_particles)

     // Hydro interface.
     .def_property_readonly("hydro", &PySpecies::get_hydro,
        R"pydoc(
        The gridded, hydrodynamic moments for this species.

        Notes
        -----
        To reduce the memory footprint and for performance reasons, VPIC uses a
        shared memory buffer for computing the moments of all species. To
        further enhance performance, only a view of this buffer is exposed
        to python. If concurrent access to the hydro properties of multiple
        species is requried, the returned hydro array **must** be explicitly
        copied, see below.

        Examples
        --------
        # Shallow access to a single species.
        >>> rho_a = species_a.hydro['rho'] # rho_a is still only a view

        # WRONG: no-copy access to multiple species
        >>> rho_a_view = species_a.hydro['rho'] # rho_a_view views species_a
        >>> rho_b_view = species_b.hydro['rho'] # rho_a_view now views species_b
        >>> rho_total = rho_a_view - rho_b_view # Will always return 0
        >>> np.allclose(rho_total, 0)
        True

        # RIGHT: Explicit copy to access multiple species
        >>> rho_a_copy = species_a.hydro['rho'].copy()
        >>> rho_b_view = species_b.hydro['rho']
        >>> rho_total = rho_a_copy - rho_b_view
        >>> np.allclose(rho_total, 0)
        False
      )pydoc");

  //============================================================================
  // Species class
  //============================================================================

  py::class_<PrototypeSpecies>(m, "Species")
    .def(py::init([](
      std::string name,
      float       q,
      float       m,
      long long   max_local_np,
      long long   max_local_nm,
      int         sort_interval,
      bool        sort_out_of_place) {

        return PrototypeSpecies(
          [name, q, m, max_local_np, max_local_nm, sort_interval, sort_out_of_place] (
            vpic_simulation * vsim
          ) {

            vsim->define_species(
              name.c_str(),
              q,
              m,
              max_local_np,
              max_local_nm,
              sort_interval,
              sort_out_of_place
            );

        });

      }),
      R"pydoc(
      Create a new particle species.

      Parameters
      ----------
      name : str
        Unique name for the species
      q : float
        Species electrical charge
      m : float
        Species mass
      max_local_np : int
        Maximum local number of particles per domain
      max_local_nm : int
        Maximum local number of particle movers per domain. If < 0, a reasonable
        default will be used
      sort_interval : int
        How often to sort the particle array, in steps
      sort_out_of_place : bool
        If true, the particles will be sorted using an out-of-place algorithm

      Returns
      -------
      sp : SpeciesPrototype
        A prototype species. It must be added to a Simulation's species list
        to be used.
      )pydoc",
      py::arg("name").none(false),
      py::arg("q") = 1,
      py::arg("m") = 1,
      py::arg("max_local_np") = 1,
      py::arg("max_local_nm") = -1,
      py::arg("sort_interval") = 100,
      py::arg("sort_out_of_place") = true);

  //============================================================================
  // ParticleArray class
  //============================================================================

  py::class_<PyParticleArray>(m, "ParticleArray")
    .def("__len__", &PyParticleArray::size)

    .def("sort", &PyParticleArray::sort,
      "Sort the particles in the array by voxel")

    .def_property_readonly("partition", &PyParticleArray::get_partition,
      "Sorts the particles and constructs a partition across voxels.")

    .def("__getitem__", &PyParticleArray::get_slice,
      R"pydoc(
      View some or all of the particles in this domain

      Parameters
      ----------
      slice : slice
        The slice into the particle array to view.

      Returns
      -------
      particles : array of particle_t
        A view into the particle array. Particles are not sorted beforehand
        and data is not copied.
      )pydoc")

    .def("__getitem__", &PyParticleArray::get_voxel,
      R"pydoc(
      View particles in a specific voxel

      Parameters
      ----------
      zindex : int
        The local z index of the voxel to select.
      yindex : int
        The local y index of the voxel to select.
      xindex : int
        The local x index of the voxel to select.

      Returns
      -------
      particles : array of particle_t
        A view of the particles in the selected voxel. Data is not copied.

      Notes
      -----
      Selecting particles by voxels requires a sorted particle array. If the
      current array is not sorted, sorting will be performed automatically.
      )pydoc")

    .def("inject", py::vectorize(&PyParticleArray::inject),
      R"pydoc(
      Inject particle(s) into the array

      Parameters
      ----------
      x : float or array-like
        The x-coordinates of the particles
      y : float or array-like
        The y-coordinates of the particles
      z : float or array-like
        The z-coordinates of the particles
      ux : float or array-like
        The x-momentum of the particles
      uy : float or array-like
        The y-momentum of the particles
      uz : float or array-like
        The z-momentum of the particles
      weight : float or array-like
        The statistical weight of the particles
      age : float or array-like
        The age (in timesteps) of the particles
      update_rhob : bool or array-like
        If true, the bound charge density will be updated to maintain
        charge conservation to roundoff.

      Examples
      --------

      # Inject a particle at the origin with momentum along the x-axis.
      >>> species.particles.inject(x=0, y=0, z=0, ux=1, uy=0, uz=0)

      # Inject a particle with different weights or ages
      >>> species.particles.inject(0, 0, 0, 1, 0, 0, weight=0.2, age=0.5)

      # Inject a group of particles with the same weight, age and momentum
      >>> lattice = np.linspace(0, 1)
      >>> x, y, z = np.meshgrid(lattice, lattice, lattice)
      >>> species.particles.inject(x=x, y=y, z=z, ux=1, uy=0, uz=0,
                                    weight=0.2, age=0.5)
    )pydoc",
    py::arg("x") = 0,
    py::arg("y") = 0,
    py::arg("z") = 0,
    py::arg("ux") = 0,
    py::arg("uy") = 0,
    py::arg("uz") = 0,
    py::arg("weight") = 0,
    py::arg("age") = 0,
    py::arg("update_rhob") = true);

}
