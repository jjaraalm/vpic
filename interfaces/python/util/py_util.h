#ifndef _py_util_h_
#define _py_util_h_

#include <set>
#include <map>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <vpic/vpic.h>

namespace py = pybind11;

//==============================================================================
// Helper macros to checkpoint/restore pickle-able objects

// #define CHECKPT_PICKLE(obj) do {                          \
//   std::string __data = py::module::import("pickle")       \
//                                   .attr("dumps")(obj)     \
//                                   .cast<std::string>();   \
//   CHECKPT_STR(__data.c_str());                            \
// } while(0)

// #define RESTORE_PICKLE(obj, type) do {                    \
//   char * __data;                                          \
//   RESTORE_STR(__data);                                    \
//   (obj) = py::module::import("pickle")                    \
//                      .attr("loads")(__data)               \
//                      .cast<type>();                       \
//   FREE(__data);                                           \
// } while(0)

//==============================================================================
// Helper macros for tuple packing/unpacking

#define X(A) std::get<0>(A)
#define Y(A) std::get<1>(A)
#define Z(A) std::get<2>(A)

#define MAKE_TUPLE(T, A) [](T * x) {                                    \
    return std::tuple<float, float, float>(x->A##x, x->A##y, x->A##z);  \
}

//==============================================================================
// Helper macro to flag an array as non-copying

#define NOCOPY(x) py::capsule(x, [](void*){})

//==============================================================================
// Helper template to reshape VPIC grid data into a numpy array. The returned array
// is fortran-indexed (x, y, z) and is non-copying. This is the best for
// performance, but requries care on the python side to avoid subtle issues.

template<class T>
py::array_t<T> numpy_view(grid_t * grid, T * data) {
  return py::array_t<T, py::array::f_style | py::array::forcecast>(
    {grid->nx+2, grid->ny+2, grid->nz+2},
    data,
    NOCOPY(data)
  );
};

//==============================================================================
// Support for classmethods - currently missing in pybind11

template<class Func, typename ... Args>
py::object classmethod(Func f, Args ... args) {
  py::object cppfunc = py::cpp_function(f, args...);
  return py::object(PyClassMethod_New(cppfunc.ptr()), true);
}

//==============================================================================
// Stream redirection

#define DEVNULL py::module::import("os").attr("devnull").cast<std::string>()

#define PYOPEN(OUT) py::module::import("__main__").attr("__builtins__").attr("open")(OUT, "a")

#define SCOPED_ESTREAM_CAPTURE(OUT) \
  py::scoped_estream_redirect _estream(std::cerr, PYOPEN(OUT))

#define SCOPED_OSTREAM_CAPTURE(OUT) \
  py::scoped_ostream_redirect _ostream(std::cout, PYOPEN(OUT))

#define SCOPED_STREAM_CAPTURE(OUT)  \
  SCOPED_OSTREAM_CAPTURE(OUT);      \
  SCOPED_ESTREAM_CAPTURE(OUT)

//==============================================================================
// Helper for formating multiline docstrings
// std::string
// unindent_docstring(const char* p) {
//     std::string result;
//     if (*p == '\n') ++p;
//     const char* p_leading = p;
//     while (std::isspace(*p) && *p != '\n')
//         ++p;
//     size_t leading_len = p - p_leading;
//     while (*p)
//     {
//         result += *p;
//         if (*p++ == '\n')
//         {
//             for (size_t i = 0; i < leading_len; ++i)
//                 if (p[i] != p_leading[i])
//                     goto dont_skip_leading;
//             p += leading_len;
//         }
//       dont_skip_leading: ;
//     }
//     return result;
// }

#endif
