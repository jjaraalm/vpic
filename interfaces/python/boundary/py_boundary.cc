#include "py_boundary.h"

void declare_boundary_conditions(py::module &m_vpic) {

  // Create a boundary submodule
  py::module m = m_vpic.def_submodule("boundary",
    R"pydoc(
    VPIC boundary condition definitions.

    Field bounadry conditions are declared using the
    ``FieldBoundaryConditions`` enum. Field boundary conditions can only be
    applied uniformly on each of the 6 domain surfaces. For voxel-level
    control over fields, materials should be used insted.

    Particle boundary conditions may be specified individually on each voxel
    surface. Boundary conditions should first be created using the methods in
    this module, then added to the simulation particle boundary condition
    list. Once added, boundary conditions may be applied to voxels.
    )pydoc");

  // FIXME: docstrings are hoisted from grid.h.
  // Is there a better way to maintain consistency? Probably not.
  py::enum_<grid_enums>(m, "FieldBoundaryConditions",
                       "Available field boundary conditions")

     .value("anti_symmetric_fields", grid_enums::anti_symmetric_fields,
            "Image charges are opposite signed (an ideal metal) and boundry rho"
            " and j are accumulated over partial voxels and images.")

     .value("pec_fields", grid_enums::pec_fields,
            "Alias for ``anti_symmetric_fields``")

     .value("metal_fields", grid_enums::metal_fields,
            "Alias for ``anti_symmetric_fields``")

     .value("symmetric_fields", grid_enums::symmetric_fields,
            "Image charges are same signed (a symmetry plane) and boundry rho "
            "and j are accumulated over partial voxels and images. Normal B is "
            "forced to 0.")

     .value("pmc_fields", grid_enums::pmc_fields,
            "Similar to ``symmetric_fields`` but normal B floats.")

     .value("absorb_fields", grid_enums::absorb_fields,
            "Applies a first-order Higdon absorbing boundary condition at a 15 "
            "degree angle from normal. No effort is made to clean divergence "
            "errors on the absorbing boundaries.")

     .export_values();

  //============================================================================
  // Particle Boundary List and Class
  //============================================================================

  py::class_<particle_bc_t>(m, "ParticleBC")
    .def_property_readonly("name", &get_particle_bc_name)
    .def_property_readonly("id", &get_particle_bc_id)
    .def_property_readonly("type", [](particle_bc_t * pbc) -> std::string {
        switch(get_particle_bc_type(pbc)) {
          case unknown_pbc_type:            return "unknown";
          case reflect_particles_pbc_type:  return "reflect particles";
          case absorb_particles_pbc_type:   return "absorb particles";
          case maxwellian_reflux_pbc_type:  return "maxwellian reflux";
          case absorb_tally_pbc_type:       return "absorb tally";
          default: throw py::key_error("Unknown ParticleBC type.");
        }
      });

  //============================================================================
  // Custom python particle boundary condition
  //============================================================================

  // m.def("particle_boundary_condtion", [](std::string  name,
  //                                        py::function interact) {
  //     return PrototypeParticleBC([name, interact](Simulation &simulation) {
  //         return python_particle_bc(name.c_str(),
  //                                   interact,
  //                                   simulation.field_array);
  //     });
  //   }, R"pydoc(
  //     Create custom particle boundary conditions.

  //     Parameters
  //     ----------
  //     name : str
  //       The name of the custom boundary condtion.
  //     interact : callable
  //       A custom boundary handler function. The boundary handler function
  //       should accept the folloing positional arguments

  //         - ``simulation`` : The simulation (Simulation)
  //         - ``speceis`` : The species of the boundary particle (ParticleSpecies)
  //         - ``face`` : The voxel face that the particle hit (int)
  //         - ``particle`` : The particle (particle_t)
  //         - ``mover`` : The remaining displacment that the particle needs to
  //                       move (particle_mover_t)
  //         - ``injectors`` : An array of available particle injectors for adding
  //                           new particles (particle_injector_t)

  //       The boundary handler should return exactly two positional values

  //         - ``absorb`` : Whether to absorb the original particle (bool)
  //         - ``n_inject`` : The number of injectors used (int)

  //     Returns
  //     -------
  //     bc : PrototypeParticleBC
  //       A prototype for the boundary condition. Must be added to a boundary
  //       condition list to be used.

  //     Notes
  //     -----
  //     Faces are labeled with integers 0-5 for the ``-x``, ``-y``, ``-z``,
  //     ``+x``, ``+y``, and ``+z`` face respectivly. The boundary face can
  //     also be determined by examining the particle position.

  //     Only a limited amount of injectors (typically only 1) are available for
  //     use in each boundary interaction. Do not attepnt to extend the injetor
  //     array, and always check the length of the injector array before using it.

  //     Custom particle boundary conditions will likely be very slow due to the
  //     substantial overhead in translating objects and performing function calls.
  //     If better performance is requried, consider creating custom boundary
  //     conditions in C++.
  //   )pydoc");

  //============================================================================
  // Specific Particle Boundary Conditions (VPIC C++)
  //============================================================================

  py::class_<ReflectPartricles>(m, "ReflectParticles")
    .def(py::init([](std::string name) {
      return ReflectPartricles([name](vpic_simulation * vsim) {
        vsim->define_particle_bc(reflecting_particle_bc(name.c_str()));
      });
    }),
    R"pydoc(
    Specular reflection boundary condition.

    Parameters
    ----------
    name : str
      Unique name for this boundary condition.
    )pydoc");

  py::class_<AbsorbParticles>(m, "AbsorbParticles")
    .def(py::init([](std::string name) {
      return AbsorbParticles([name](vpic_simulation * vsim) {
        vsim->define_particle_bc(absorbing_particle_bc(name.c_str()));
      });
    }),
    R"pydoc(
    Particle absorbing boundary condition.

    Parameters
    ----------
    name : str
      Unique name for this boundary condition.
    )pydoc");

  py::class_<MaxwellianReflux>(m, "MaxwellianReflux")
    .def(py::init([](
      std::string name,
      std::map<std::string, double> vth_para,
      std::map<std::string, double> vth_perp) {

        return MaxwellianReflux([name, vth_para, vth_perp](vpic_simulation * vsim) {

          if (num_species(vsim->species_list) == 0)
            throw std::runtime_error("SpeciesList is empty. Cannot create "
                                    "\'" + name + "\'");

          species_t * sp;
          particle_bc_t * mr = maxwellian_reflux(name.c_str(),
                                                vsim->species_list,
                                                vsim->entropy);

          LIST_FOR_EACH(sp, vsim->species_list) {
            const char * sp_name = get_species_name(sp);
            set_reflux_temp(mr, sp, vth_para.at(sp_name), vth_perp.at(sp_name));
          }

          vsim->define_particle_bc(mr);

        });

    }),
    R"pydoc(
    Thermal reflux boundary condition.

    Parameters
    ----------
    name : str
      Unique name for this boundary condition.
    vth_para : dict
      Dict of thermal velocities in the boundary normal direction. Each
      key should be a species name and each value the velocity.
    vth_perp : dict
      Dict of thermal velocities in the plane of the boundary. Each
      key should be a species name and each value the velocity.

    Notes
    -----
    Refluxing boundary condition on particles.  Calculate normalized
    momenta from the various perp and para temperatures.  Then, sample
    Maxwellian specra from Maxwellian distributsions that have the
    given normalized momenta.

    The perpendicular spectra are sampled from a bi-Maxwellian.  The
    parallel spectra are sampled from vpara = sqrt(2) vth sqrt(-log h)
    where h is uniformly distributed over range (0,1).  Strictly
    speaking, this method only works for non-relativistic distributions
    of edge particles.

    Due to the internal implementation, the species list should not be
    modifed after adding this boundary condition.
    )pydoc");

  py::class_<AbsorbTally>(m, "AbsorbTally")
    .def(py::init([](std::string name) {

        return AbsorbTally([name](vpic_simulation * vsim) {

          if (num_species(vsim->species_list) == 0)
            throw std::runtime_error("SpeciesList is empty. Cannot create "
                                    "\'" + name + "\'");

          vsim->define_particle_bc(
            absorb_tally(name.c_str(), vsim->species_list, vsim->field_array)
          );
        });

    }),
    R"pydoc(
    Special absorbing boundary condition that tallies the number of
    particles absorbed of each species.

    Parameters
    ----------
    name : str
      Unique name for this boundary condition.

    Notes
    -----
    Due to the internal implementation, the species list should not be
    modifed after adding this boundary condition.
    )pydoc");

}
