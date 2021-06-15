#include "py_grid.h"

#define GET(FIELD) [](SimulationGrid &g){ return g.g->FIELD; }
#define GET_AS(FIELD, TYPE) [](SimulationGrid &g){ return (TYPE) g.g->FIELD; }
#define SET(FIELD, TYPE) [](SimulationGrid &g, TYPE value){g.g->FIELD = value;}

void declare_grid(py::module &m) {

  py::class_<SimulationGrid>(m, "Grid")

    .def_property_readonly("nx", GET(nx))
    .def_property_readonly("ny", GET(ny))
    .def_property_readonly("nz", GET(nz))

    .def_property_readonly("dx", GET(dx))
    .def_property_readonly("dy", GET(dy))
    .def_property_readonly("dz", GET(dz))
    .def_property_readonly("dt", GET(dt))

    .def_property_readonly("x0", GET(x0))
    .def_property_readonly("y0", GET(y0))
    .def_property_readonly("z0", GET(z0))
    .def_property_readonly("t0", GET(t0))

    .def_property_readonly("x1", GET(x1))
    .def_property_readonly("y1", GET(y1))
    .def_property_readonly("z1", GET(z1))

    .def_property_readonly("step", GET(step))
    .def_property_readonly("shape", [](SimulationGrid &g) {
      return std::make_tuple(g.g->nx, g.g->ny, g.g->nz);
    })

    .def_property_readonly("dt_cfl", [](SimulationGrid &g) {

        if (!g.g->nv)
          return std::numeric_limits<double>::infinity();

        return g.simulation.vsim->courant_length(g.g->x1 - g.g->x0,
                                                 g.g->y1 - g.g->y0,
                                                 g.g->z1 - g.g->z0,
                                                 g.g->nx,
                                                 g.g->ny,
                                                 g.g->nz) / g.g->cvac;

      }, "The maximum permissible timestep due to CFL restrictions.")

    .def("partition_box", [](SimulationGrid &g,
                             std::tuple<int, int, int> grid_shape,
                             std::tuple<int, int, int> rank_shape,
                             std::tuple<double, double, double> lower,
                             std::tuple<double, double, double> upper) {

          partition_periodic_box(g.g,
                                 X(lower),      Y(lower),      Z(lower),
                                 X(upper),      Y(upper),      Z(upper),
                                 X(grid_shape), Y(grid_shape), Z(grid_shape),
                                 X(rank_shape), Y(rank_shape), Z(rank_shape));

       }, R"pydoc(
         Partitions a rectilinear grid

         Parameters
         ----------
         grid_shape : (nx, ny, nz)
           The size of the global grid
         rank_shape : (px, py, pz)
           The number of domains to decompose each direction into. ``px*py*pz``
           should be exactly equal to the number of MPI ranks.
         lower : (x0, y0, z0)
           The global coordinate of the lower grid point.
         upper : (x1, y1, z1)
           The global coordinate of the upper grid point.

         Notes
         -----
         This method is the simplest way to decompose a problem across ranks.
         Here, the global coordinate space is rectilinear and each rank recieves
         an equal sized chunk of the domain. Fully periodic boundary condtions
         are used by default, but can be changed using the
         :py:func:``set_field_boundary_conditions`` and
         :py:func:``set_particle_boundary_conditions`` methods.

         See Also
         --------
         set_field_boundary_conditions
           Set the local field boundary conditions
         set_particle_boundary_conditions
           Set the local particle boundary conditions

       )pydoc",
      py::arg("grid_shape"),
      py::arg("rank_shape"),
      py::arg("lower") = std::tuple<double, double, double>(0, 0, 0),
      py::arg("upper") = std::tuple<double, double, double>(1, 1, 1))

    .def("create_local", [](SimulationGrid &g,
                            std::tuple<int, int, int> shape,
                            std::tuple<double, double, double> lower,
                            std::tuple<double, double, double> upper) {

        // Need to manually set internal variables.
        double dx = (X(upper) - X(lower))/((double) X(shape));
        double dy = (Y(upper) - Y(lower))/((double) Y(shape));
        double dz = (Z(upper) - Z(lower))/((double) Z(shape));

        g.g->x0 = X(lower);
        g.g->x1 = X(upper);
        g.g->y0 = Y(lower);
        g.g->y1 = Y(upper);
        g.g->z0 = Z(lower);
        g.g->z1 = Z(upper);
        g.g->dx = dx;
        g.g->dy = dy;
        g.g->dz = dz;
        g.g->dV = dx*dy*dz;
        g.g->rdx = 1./dx;
        g.g->rdy = 1./dy;
        g.g->rdz = 1./dz;
        g.g->r8V = (1./dx)*(1./dy)*(1./dz)*0.125;

        size_grid(g.g, X(shape), Y(shape), Z(shape));

      }, R"pydoc(
        Creates a local grid

        Parameters
        ----------
        shape : (nx, ny, nz)
          The size of the local grid
        lower : (x0, y0, z0)
          The position of the lower corner in global coordinates
        upper : (x1, y1, z1)
          The position of the upper corner in global coordinates

        Notes
        -----
        This method is intended for users who need to create complex,
        non-rectilinear domain decompositions. It is the users' responsibility
        to ensure that domain connectivity is consistent across domains and that
        all boundary condtions are appropriately set.

        See Also
        --------
        partition_box
          Simplified method for creating a rectilinear domain decomposition

      )pydoc")

    .def("join_domains", [](SimulationGrid &g,
                    int x_upper, int x_lower,
                    int y_upper, int y_lower,
                    int z_upper, int z_lower) {

        if (x_upper > 0) join_grid(g.g, BOUNDARY( 1,  0,  0), x_upper);
        if (x_lower > 0) join_grid(g.g, BOUNDARY(-1,  0,  0), x_lower);
        if (y_upper > 0) join_grid(g.g, BOUNDARY( 0,  1,  0), y_upper);
        if (y_lower > 0) join_grid(g.g, BOUNDARY( 0, -1,  0), y_lower);
        if (z_upper > 0) join_grid(g.g, BOUNDARY( 0,  0,  1), z_upper);
        if (z_lower > 0) join_grid(g.g, BOUNDARY( 0,  0, -1), z_lower);

      }, R"pydoc(
        Join domains together.

        Parameters
        ----------
        x_upper : int
          The domain to attach on the upper x-boundry.
        x_lower : int
          The domain to attach on the lower x-boundry.
        y_upper : int
          The domain to attach on the upper y-boundry.
        y_lower : int
          The domain to attach on the lower y-boundry.
        z_upper : int
          The domain to attach on the upper z-boundry.
        z_lower : int
          The domain to attach on the lower z-boundry.

        Notes
        -----
        If any parameter is negative, then no domain will be attached. This
        method is intended for users who need to create complex, non-rectilinear
        domain decompositions. It is the users' responsibility to ensure that
        domain connectivity is consistent across domains and that all boundary
        condtions are appropriately set.

        See Also
        --------
        partition_box
          Simplified method for creating a rectilinear domain decomposition

      )pydoc",
      py::arg("x_upper") = -1,
      py::arg("x_lower") = -1,
      py::arg("y_upper") = -1,
      py::arg("y_lower") = -1,
      py::arg("z_upper") = -1,
      py::arg("z_lower") = -1)

    .def("set_field_boundary_conditions",
         [](SimulationGrid &g,
            grid_enums * x_upper, grid_enums * x_lower,
            grid_enums * y_upper, grid_enums * y_lower,
            grid_enums * z_upper, grid_enums * z_lower) {

        if (x_upper) set_fbc(g.g, BOUNDARY( 1,  0,  0), *x_upper);
        if (x_lower) set_fbc(g.g, BOUNDARY(-1,  0,  0), *x_lower);
        if (y_upper) set_fbc(g.g, BOUNDARY( 0,  1,  0), *y_upper);
        if (y_lower) set_fbc(g.g, BOUNDARY( 0, -1,  0), *y_lower);
        if (z_upper) set_fbc(g.g, BOUNDARY( 0,  0,  1), *z_upper);
        if (z_lower) set_fbc(g.g, BOUNDARY( 0,  0, -1), *z_lower);

      }, R"pydoc(
        Set field boundary conditions.

        Parameters
        ----------
        x_upper : BoundaryCondition or None
          The boundary condition to set on the upper x-boundary.
        x_lower : BoundaryCondition or None
          The boundary condition to set on the lower x-boundary
        y_upper : BoundaryCondition or None
          The boundary condition to set on the upper y-boundary
        y_lower : BoundaryCondition or None
          The boundary condition to set on the lower y-boundary
        z_upper : BoundaryCondition or None
          The boundary condition to set on the upper z-boundary
        z_lower : BoundaryCondition or None
          The boundary condition to set on the lower z-boundary

        Notes
        -----
        This sets the boundary conditions on the local domain. Generally
        boundary conditions will want to be applied on the global domain,
        in which case not all ranks will need to use this.

      )pydoc",
      py::arg("x_upper") = nullptr,
      py::arg("x_lower") = nullptr,
      py::arg("y_upper") = nullptr,
      py::arg("y_lower") = nullptr,
      py::arg("z_upper") = nullptr,
      py::arg("z_lower") = nullptr)

    .def("set_particle_boundary_conditions",
         [](SimulationGrid &g,
            grid_enums * x_upper, grid_enums * x_lower,
            grid_enums * y_upper, grid_enums * y_lower,
            grid_enums * z_upper, grid_enums * z_lower) {

        if (x_upper) set_pbc(g.g, BOUNDARY( 1,  0,  0), *x_upper);
        if (x_lower) set_pbc(g.g, BOUNDARY(-1,  0,  0), *x_lower);
        if (y_upper) set_pbc(g.g, BOUNDARY( 0,  1,  0), *y_upper);
        if (y_lower) set_pbc(g.g, BOUNDARY( 0, -1,  0), *y_lower);
        if (z_upper) set_pbc(g.g, BOUNDARY( 0,  0,  1), *z_upper);
        if (z_lower) set_pbc(g.g, BOUNDARY( 0,  0, -1), *z_lower);

      }, R"pydoc(
        Set particle boundary conditions.

        Parameters
        ----------
        x_upper : BoundaryCondition or None
          The boundary condition to set on the upper x-boundary.
        x_lower : BoundaryCondition or None
          The boundary condition to set on the lower x-boundary
        y_upper : BoundaryCondition or None
          The boundary condition to set on the upper y-boundary
        y_lower : BoundaryCondition or None
          The boundary condition to set on the lower y-boundary
        z_upper : BoundaryCondition or None
          The boundary condition to set on the upper z-boundary
        z_lower : BoundaryCondition or None
          The boundary condition to set on the lower z-boundary

        Notes
        -----
        This sets the boundary conditions on the local domain. Generally
        boundary conditions will want to be applied on the global domain,
        in which case not all ranks will need to use this.

      )pydoc",
      py::arg("x_upper") = nullptr,
      py::arg("x_lower") = nullptr,
      py::arg("y_upper") = nullptr,
      py::arg("y_lower") = nullptr,
      py::arg("z_upper") = nullptr,
      py::arg("z_lower") = nullptr);

}
