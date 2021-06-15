#include "py_lists.h"
#include "../species/py_species.h"
#include "../boundary/py_boundary.h"
#include "../material/py_materials.h"

// Tempalte specializations for the various types of lists


#define DECLARE_VPIC_LIST_METHODS(T, LIST, KIND)                                              \
template<> std::function<void(T)> LIST::delete_vpic_list = &delete_##KIND##_list;             \
template<> std::function<int(T)> LIST::count_vpic_list = &num_##KIND;                         \
template<> std::function<std::string(T)> LIST::get_vpic_item_name = &get_##KIND##_name;       \
template<> std::function<T(T)> LIST::next_vpic_item = &next_##KIND;                           \
template<> std::function<T*(vpic_simulation*)>                                                \
LIST::get_vpic_list = [](vpic_simulation * vsim) { return &vsim->KIND##_list; }

//==============================================================================
// Species List : species_t * -> PySpecies

template<>
struct ListTranslator<species_t*> {
  using PyType = PySpecies;
  using VPICType = species_t*;

  static PyType wrap(vpic_simulation * vsim, VPICType item) { return PyType(vsim, item); }
  static VPICType unwrap(PyType& item) { return item.sp; }

};

DECLARE_VPIC_LIST_METHODS(species_t*, SpeciesList, species);

//==============================================================================
// Material List : material_t * -> material_t *

DECLARE_VPIC_LIST_METHODS(material_t*, MaterialList, material);

//==============================================================================
// Particle BC List : particle_bc_t * -> particle_bc_t *

DECLARE_VPIC_LIST_METHODS(particle_bc_t*, ParticleBCList, particle_bc);

//==============================================================================
// Emitter List : emitter_t * -> emitter_t *

DECLARE_VPIC_LIST_METHODS(emitter_t*, EmitterList, emitter);

//==============================================================================
// Colision Op List : collision_op_t * -> collision_op_t *

DECLARE_VPIC_LIST_METHODS(collision_op_t*, CollisionOpList, collision_op);

//==============================================================================
// Python entrypoint

template<typename T>
void declare_list(py::module &m, std::string name) {

    using List = VPICList<T>;

    // Lists contain constructed objects and can be examined.
    py::class_<List>(m, name.c_str())
      .def("__getitem__",  &List::at, py::return_value_policy::reference_internal)
      .def("__getitem__",  &List::get_name, py::return_value_policy::reference_internal)
      .def("__len__",      &List::size)
      .def("__str__",      &List::str)
      .def("__repr__",     &List::str)
      .def("__contains__", &List::contains)
      .def("__iter__",     &List::iteritems, py::keep_alive<0, 1>())
      .def("append",       &List::append)
      .def("extend",       &List::extend)
      .def("keys",         &List::get_keys);

}

void declare_lists(py::module &m_vpic) {

  // Create a list submodule
  py::module m = m_vpic.def_submodule("list",
    R"pydoc(
      Interface classes for interacting with VPIC lists.
    )pydoc");


  declare_list<species_t *>(m, "SpeciesList");
  declare_list<material_t *>(m, "MaterialList");
  declare_list<particle_bc_t *>(m, "ParticleBCList");
  // declare_list<emitter_t *>(m, "EmitterList");
  // declare_list<collision_op_t *>(m, "CollisionOpList");

}
