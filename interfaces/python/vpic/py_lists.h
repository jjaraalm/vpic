#ifndef _py_lists_h_
#define _py_lists_h_

#include <algorithm>
#include "py_proto.h"
#include "py_simulation.h"

// Translation between VPIC and Python types for lists
// Default is no translation, specializations are located in py_lists.cc
template<class VPICType>
struct ListTranslator {
  using PyType = VPICType;

  static PyType wrap(vpic_simulation * vsim, VPICType item) { return item; }
  static VPICType unwrap(PyType& item) { return item; }

};


// Interface to VPIC lists. Lists are "alive" and are always treated to
// references to the underlying VPIC list. The underlying list is assumed
// to be mutable, and so we rebuild a local cache on every access.

template<class T>
class VPICList {
public:

  using PyType = typename ListTranslator<T>::PyType;

  //============================================================================
  // Pybind entrypoints
  //============================================================================

  static VPICList<T>
  get_list(Simulation &simulation) {
    return VPICList<T>(simulation.vsim);
  }

  static void
  set_list(Simulation &simulation, std::vector<VPICPrototypeObject<T>> value) {
    VPICList<T> list = VPICList<T>::get_list(simulation);
    list.delete_list();
    list.extend(value);
  }

  //============================================================================
  // Member functions
  //============================================================================

  int size() {
    rebuild_items();
    return keys.size();
  }

  PyType at(int index) {
    rebuild_items();
    if (index >= items.size())
      throw py::index_error("Index out of range.");
    return items[index];
  }

  PyType get_name(std::string key) {
    rebuild_items();
    auto loc = std::find(keys.begin(), keys.end(), key);
    if (loc == keys.end())
      throw py::key_error("Item \"" + key + "\" not found.");
    return items[std::distance(keys.begin(), loc)];
  }

  PyType head() {
    rebuild_items();
    return keys.size() > 0 ? items[keys.size()-1] : nullptr;
  }

  bool contains(std::string key) {
    rebuild_items();
    return std::find(keys.begin(), keys.end(), key) != keys.end();
  }

  std::vector<std::string> get_keys() {
    rebuild_items();
    return keys;
  }

  std::vector<PyType> get_values() {
    rebuild_items();
    return items;
  }

  py::object iterkeys() {
    rebuild_items();
    return py::make_iterator(keys.begin(), keys.end());
  }

  py::object iteritems() {
    rebuild_items();
    return py::make_iterator(items.begin(), items.end());
  }

  void extend(std::vector<VPICPrototypeObject<T>> other) {
    for(auto item : other)
      append(item);
  }

  void append(VPICPrototypeObject<T>& proto) {
    set_active_registry(vsim->checkpt_registry_id);
    proto(vsim);
  }

  std::string str() {
    rebuild_items();
    std::string out = "[";
    for(auto key : keys)
      out += "\'" + key + "\', ";
    out.pop_back();
    out.pop_back();
    return out + "]";
  }

private:
  VPICList(vpic_simulation * vsim)
  : vsim(vsim) {
   rebuild_items();
  }

  void delete_list() {
    set_active_registry(vsim->checkpt_registry_id);
    delete_vpic_list(*get_vpic_list(vsim));
    *get_vpic_list(vsim) = nullptr;
  }

  void rebuild_items() {
    if ( count_vpic_list(*get_vpic_list(vsim)) != items.size() ){
      keys.clear();
      items.clear();

      for(T item = *get_vpic_list(vsim) ; item ; item = next_vpic_item(item)) {
        keys.emplace_back(get_vpic_item_name(item));
        items.emplace_back(ListTranslator<T>::wrap(vsim, item));
      }

      std::reverse(keys.begin(), keys.end());
      std::reverse(items.begin(), items.end());
    }
  }

  // Entrypoints to VPIC
  vpic_simulation * vsim;
  static std::function<T*(vpic_simulation*)> get_vpic_list;
  static std::function<void(T)> delete_vpic_list;
  static std::function<int(const T)> count_vpic_list;
  static std::function<std::string(const T)> get_vpic_item_name;
  static std::function<T(T)> next_vpic_item;

  // Internal cache
  std::vector<std::string> keys;
  std::vector<PyType> items;

};

typedef VPICList<species_t*> SpeciesList;
typedef VPICList<emitter_t*> EmitterList;
typedef VPICList<material_t*> MaterialList;
typedef VPICList<particle_bc_t*> ParticleBCList;
typedef VPICList<collision_op_t*> CollisionOpList;

// Entrypoint for python declarations.
void declare_lists(py::module &m);

#endif // _lists_h_
