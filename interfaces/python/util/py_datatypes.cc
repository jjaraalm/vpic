#include "py_datatypes.h"

void declare_datatypes(py::module &m_vpic) {

  // Create a datatypes submodule
  py::module m = m_vpic.def_submodule("datatypes",
    R"pydoc(
      Type definitions for the various data structures used in VPIC.
    )pydoc");

    // Particle data types.
  PYBIND11_NUMPY_DTYPE(particle_t,
                        dx, dy, dz, i,
                        ux, uy, uz, w);


  // Particle movers and injectors are needed for custom particle_bcs.
  PYBIND11_NUMPY_DTYPE(particle_mover_t,
                        dispx, dispy, dispz, i);

  PYBIND11_NUMPY_DTYPE(particle_injector_t,
                        dx, dy, dz, i,
                        ux, uy, uz, w,
                        dispx, dispy, dispz, sp_id);

  // Hydro data type.
  // FIXME: Programatically adjust for padding?
  PYBIND11_NUMPY_DTYPE(hydro_t,
                        jx, jy, jz, rho,
                        px, py, pz, ke,
                        txx, tyy, tzz,
                        tyz, tzx, txy, _pad);

  // Field data typt
  PYBIND11_NUMPY_DTYPE(field_t,
                        ex,    ey,    ez,    div_e_err,
                        cbx,   cby,   cbz,   div_b_err,
                        tcax,  tcay,  tcaz,  rhob,
                        jfx,   jfy,   jfz,   rhof,
                        ematx, ematy, ematz, nmat,
                        fmatx, fmaty, fmatz, cmat);

}
