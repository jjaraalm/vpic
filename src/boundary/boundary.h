#ifndef _boundary_h_
#define _boundary_h_

#include "../species_advance/species_advance.h"

struct particle_bc;
typedef struct particle_bc particle_bc_t;

// may have been moved by Kevin
typedef struct link_boundary {
char fbase[256];	// base of file name to contain link info
double n_out;		// number of writes so far on this node (double to
						// accomodate long long runs)
} link_boundary_t;

enum particle_bc_type {
  unknown_pbc_type            = 0,
  reflect_particles_pbc_type  = 1,
  absorb_particles_pbc_type   = 2,
  maxwellian_reflux_pbc_type  = 3,
  absorb_tally_pbc_type       = 4
};

BEGIN_C_DECLS

/* In boundary.c */

int
num_particle_bc( const particle_bc_t * RESTRICT pbc_list );

void
delete_particle_bc_list( particle_bc_t * RESTRICT pbc_list );

particle_bc_t *
append_particle_bc( particle_bc_t * pbc,
                    particle_bc_t ** pbc_list );

particle_bc_t *
next_particle_bc( particle_bc_t * pbc );

particle_bc_t *
find_particle_bc_id( int64_t         id,
                     particle_bc_t * pbc );

particle_bc_t *
find_particle_bc_name( const char    * name,
                       particle_bc_t * pbc );

int64_t
get_particle_bc_id( particle_bc_t * pbc );

const char *
get_particle_bc_name( particle_bc_t * pbc );

particle_bc_type
get_particle_bc_type( particle_bc_t * pbc );

/* In boundary_p.cxx */

void
boundary_p( particle_bc_t       * RESTRICT pbc_list,
            species_t           * RESTRICT sp_list,
            field_array_t       * RESTRICT fa,
            accumulator_array_t * RESTRICT aa );

// Elevates builtin particle boundary conditions into the particle_bc_t API.

particle_bc_t *
reflecting_particle_bc( const char * name );

particle_bc_t *
absorbing_particle_bc( const char * name );

/* In maxwellian_reflux.c */

particle_bc_t *
maxwellian_reflux( const char *          name,
                   species_t  * RESTRICT sp_list,
                   rng_pool_t * RESTRICT rp );

void
set_reflux_temp( /**/  particle_bc_t * RESTRICT mr,
                 const species_t     * RESTRICT sp,
                 float ut_para,
                 float ut_perp );

/* In absorb_tally.c */

particle_bc_t *
absorb_tally( const char           *          name,
              /**/  species_t      * RESTRICT sp_list,
              const field_array_t  * RESTRICT fa );

int *
get_absorb_tally( particle_bc_t * pbc );

END_C_DECLS

#endif /* _boundary_h_ */
