#include "py_materials.h"

void declare_materials(py::module &m_vpic) {

  // Create a boundary submodule
  py::module m = m_vpic.def_submodule("material",
    R"pydoc(
      VPIC material definitions.

      Materials define the bulk properties of voxels are are used in the field
      advance. Materials should be created using the ``material`` method and
      then attached to a simulation's material list. Once attached, the material
      can be applied to voxels.
    )pydoc");

  //============================================================================
  // Material List and Class
  //============================================================================

  py::class_<material_t>(m, "RegisteredMaterial")
    .def_property_readonly("name", &get_material_name)
    .def_property_readonly("id", &get_material_id)
    .def_property_readonly("eps",   MAKE_TUPLE(material_t, eps))
    .def_property_readonly("mu",    MAKE_TUPLE(material_t, mu))
    .def_property_readonly("sigma", MAKE_TUPLE(material_t, sigma))
    .def_property_readonly("zeta",  MAKE_TUPLE(material_t, zeta));

  //============================================================================
  // Material creation, isotropic and non-isotropic
  //============================================================================

  py::class_<PrototypeMaterial>(m, "Material")
    .def(py::init([](
        std::string name,
        std::tuple<float, float, float> eps,
        std::tuple<float, float, float> mu,
        std::tuple<float, float, float> sigma,
        std::tuple<float, float, float> zeta) {

          return PrototypeMaterial([name, eps, mu, sigma, zeta] (
            vpic_simulation * vsim
          ) {
              vsim->define_material(name.c_str(),
                                    X(eps),   Y(eps),   Z(eps),
                                    X(mu),    Y(mu),    Z(mu),
                                    X(sigma), Y(sigma), Z(sigma),
                                    X(zeta),  Y(zeta),  Z(zeta));
          });

      }),
      R"pydoc(
        Create a new anisotropic material

        Parameters
        ----------
        name : str
          Unique name for the material
        eps : array-like
          Relative permitivity for the material in the x, y, and z directions.
        mu : array-like
          Relative permeability for the material in the x, y, and z directions.
        sigma : array-like
          Electrical conductivity for the material in the x, y, and z directions.
        zeta : array-like
          Magnetic conductivity for the material in the x, y, and z directions.

        Returns
        -------
        mat : PrototypeMaterial
          A prototype for the material. Must be added to a material list to be
          used.
      )pydoc",
      py::arg("name").none(false),
      py::arg("eps")   = std::tuple<float, float, float>(1., 1., 1.),
      py::arg("mu")    = std::tuple<float, float, float>(1., 1., 1.),
      py::arg("sigma") = std::tuple<float, float, float>(0., 0., 0.),
      py::arg("zeta")  = std::tuple<float, float, float>(0., 0., 0.)
    )

    .def(py::init([](
      std::string name,
      float eps,
      float mu,
      float sigma,
      float zeta) {

        return PrototypeMaterial([name, eps, mu, sigma, zeta] (
          vpic_simulation * vsim
        ) {
            vsim->define_material(name.c_str(), eps, mu, sigma, zeta);
        });

     }),
      R"pydoc(
        Create a new isotropic material

        Parameters
        ----------
        name : str
          Unique name for the material
        eps : float
          Relative permitivity for the material
        mu : float
          Relative permeability for the material
        sigma : float
          Electrical conductivity for the material
        zeta : float
          Magnetic conductivity for the material

        Returns
        -------
        mat : PrototypeMaterial
          A prototype for the material. Must be added to a material list to be
          used.
      )pydoc",
     py::arg("name").none(false),
     py::arg("eps")   = 1,
     py::arg("mu")    = 1,
     py::arg("sigma") = 0,
     py::arg("zeta")  = 0
   );

}
