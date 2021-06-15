#define IN_boundary
#include "boundary_private.h"

/* Private interface *********************************************************/

void
checkpt_particle_bc_internal( const particle_bc_t * RESTRICT pbc ) {
  CHECKPT( pbc, 1 );
  CHECKPT_STR( pbc->name );
  CHECKPT_SYM( pbc->interact );
  CHECKPT_SYM( pbc->delete_pbc );
  CHECKPT_PTR( pbc->next );
}

particle_bc_t *
restore_particle_bc_internal( void * params ) {
  particle_bc_t * pbc;
  RESTORE( pbc );
  pbc->params = params;
  RESTORE_STR( pbc->name );
  RESTORE_SYM( pbc->interact );
  RESTORE_SYM( pbc->delete_pbc );
  RESTORE_PTR( pbc->next );
  return pbc;
}

particle_bc_t *
new_particle_bc_internal( const char * name,
                          void * params,
                          particle_bc_type type,
                          particle_bc_func_t interact,
                          delete_particle_bc_func_t delete_pbc,
                          checkpt_func_t checkpt,
                          restore_func_t restore,
                          reanimate_func_t reanimate ) {
  particle_bc_t * pbc;
  MALLOC( pbc, 1 );
  CLEAR( pbc, 1 );
  MALLOC( pbc->name, strlen(name)+1 );
  strcpy( pbc->name, name );
  pbc->type       = type;
  pbc->params     = params;
  pbc->interact   = interact;
  pbc->delete_pbc = delete_pbc;
  /* id, next set by append_particle_bc */
  REGISTER_OBJECT( pbc, checkpt, restore, reanimate );
  return pbc;
}

void
delete_particle_bc_internal( particle_bc_t * pbc ) {
  UNREGISTER_OBJECT( pbc );
  FREE( pbc->name );
  FREE( pbc );
}

/* Public interface **********************************************************/

int
num_particle_bc( const particle_bc_t * RESTRICT pbc_list ) {
  return pbc_list ? (-pbc_list->id-2) : 0;
}

void
delete_particle_bc_list( particle_bc_t * pbc_list ) {
  particle_bc_t * pbc;
  while( pbc_list ) {
    pbc = pbc_list;
    pbc_list = pbc_list->next;
    pbc->delete_pbc( pbc );
  }
}

particle_bc_t *
append_particle_bc( particle_bc_t * pbc,
                    particle_bc_t ** pbc_list ) {
  if( !pbc || !pbc_list ) ERROR(( "Bad args" ));
  if( pbc->next ) ERROR(( "Particle boundary condition already in a list" ));

  if      (pbc->type == absorb_particles_pbc_type)  pbc->id = absorb_particles;
  else if (pbc->type == reflect_particles_pbc_type) pbc->id = reflect_particles;
  else  {
    // Assumes reflective/absorbing are -1, -2
    pbc->id = -3-num_particle_bc( *pbc_list );
  }

  pbc->next = *pbc_list;
  *pbc_list = pbc;
  return pbc;
}

particle_bc_t *
next_particle_bc( particle_bc_t * pbc ) {
  return pbc ? pbc->next : NULL;
}

particle_bc_t *
find_particle_bc_id( int64_t         id,
                     particle_bc_t * pbc ) {
  particle_bc_t * p;
  LIST_FIND_FIRST(p, pbc, p->id == id);
  return p;
}

particle_bc_t *
find_particle_bc_name( const char    * name,
                       particle_bc_t * pbc ) {
  particle_bc_t * p;
  if(!name) ERROR(("Empty name"));
  LIST_FIND_FIRST(p, pbc, strcmp(p->name, name) == 0);
  return p;
}

int64_t
get_particle_bc_id( particle_bc_t * pbc ) {
  if( !pbc ) return 0;
  if( pbc==(particle_bc_t *) absorb_particles ) return  absorb_particles;
  if( pbc==(particle_bc_t *)reflect_particles ) return reflect_particles;
  return pbc->id;
}

const char *
get_particle_bc_name( particle_bc_t * pbc ) {
  if( !pbc ) return "";
  if( pbc==(particle_bc_t *) absorb_particles ) return  "absorb particles";
  if( pbc==(particle_bc_t *)reflect_particles ) return "reflect particles";
  return pbc->name;
}

particle_bc_type
get_particle_bc_type( particle_bc_t * pbc ) {
  if( !pbc ) return unknown_pbc_type;
  if( pbc==(particle_bc_t *) absorb_particles ) return absorb_particles_pbc_type;
  if( pbc==(particle_bc_t *)reflect_particles ) return reflect_particles_pbc_type;
  return pbc->type;
}

particle_bc_t *
reflecting_particle_bc( const char * name ) {
  return new_particle_bc_internal( name,
                                   NULL,
                                   reflect_particles_pbc_type,
                                   NULL,
                                   delete_particle_bc_internal,
                                   (checkpt_func_t)checkpt_particle_bc_internal,
                                   (restore_func_t)restore_particle_bc_internal,
                                   NULL );
}

particle_bc_t *
absorbing_particle_bc( const char * name ) {
  return new_particle_bc_internal( name,
                                   NULL,
                                   absorb_particles_pbc_type,
                                   NULL,
                                   delete_particle_bc_internal,
                                   (checkpt_func_t)checkpt_particle_bc_internal,
                                   (restore_func_t)restore_particle_bc_internal,
                                   NULL );
}
