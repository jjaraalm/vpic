#define IN_collision
#include "collision_private.h"

/* Private interface *********************************************************/

void
checkpt_collision_op_internal( const collision_op_t * RESTRICT cop ) {
  CHECKPT( cop, 1 );
  CHECKPT_STR( cop->name );
  CHECKPT_SYM( cop->apply );
  CHECKPT_SYM( cop->delete_cop );
  CHECKPT_PTR( cop->next );
}

collision_op_t *
restore_collision_op_internal( void * params ) {
  collision_op_t * cop;
  RESTORE( cop );
  cop->params = params;
  RESTORE_STR( cop->name );
  RESTORE_SYM( cop->apply );
  RESTORE_SYM( cop->delete_cop );
  RESTORE_PTR( cop->next );
  return cop;
}

collision_op_t *
new_collision_op_internal( const char * name,
                           void * params,
                           collision_op_type type,
                           collision_op_func_t apply,
                           delete_collision_op_func_t delete_cop,
                           checkpt_func_t checkpt,
                           restore_func_t restore,
                           reanimate_func_t reanimate ) {
  collision_op_t * cop;
  MALLOC( cop, 1 );
  MALLOC( cop->name, strlen(name)+1 );
  strcpy( cop->name, name );
  cop->type       = type;
  cop->params     = params;
  cop->apply      = apply;
  cop->delete_cop = delete_cop;
  cop->next       = NULL; /* Set by append_collision_op */
  REGISTER_OBJECT( cop, checkpt, restore, reanimate );
  return cop;
}

void
delete_collision_op_internal( collision_op_t * cop ) {
  UNREGISTER_OBJECT( cop );
  FREE( cop );
}

/* Public interface **********************************************************/

int
num_collision_op( const collision_op_t * RESTRICT cop_list ) {
  const collision_op_t * RESTRICT cop;
  int n = 0;
  LIST_FOR_EACH( cop, cop_list ) n++;
  return n;
}

collision_op_t *
next_collision_op( collision_op_t * cop_list ) {
  return cop_list ? cop_list->next : NULL;
}

collision_op_t *
find_collision_op_name( const char     * name,
                        collision_op_t * cop_list ) {
  collision_op_t * cop;
  if (!name) return NULL;
  LIST_FIND_FIRST(cop, cop_list, strcmp(cop->name, name) == 0);
  return cop;
}

collision_op_t *
find_collision_op_id( int              id,
                      collision_op_t * cop_list ){
  collision_op_t * cop;
  LIST_FIND_FIRST(cop, cop_list, cop->id == id);
  return cop;
}

const char *
get_collision_op_name( const collision_op_t * cop ) {
  return cop ? cop->name : "";
}

int
get_collision_op_id( const collision_op_t * cop ) {
  return cop ? cop->id : -1;
}

collision_op_type
get_collision_op_type( const collision_op_t * cop ) {
  return cop ? cop->type : unknown_collision_type;
}


void
apply_collision_op_list( collision_op_t * cop_list ) {
  collision_op_t * cop;
  LIST_FOR_EACH( cop, cop_list ) cop->apply( cop->params );
}

void
delete_collision_op_list( collision_op_t * cop_list ) {
  collision_op_t * cop;
  while( cop_list ) {
    cop = cop_list;
    cop_list = cop_list->next;
    cop->delete_cop( cop );
  }
}

collision_op_t *
append_collision_op( collision_op_t * cop,
                     collision_op_t ** cop_list ) {
  if( !cop || !cop_list || cop->next ) ERROR(( "Bad args" ));
  cop->id = num_collision_op( *cop_list );
  cop->next = *cop_list;
  *cop_list = cop;
  return cop;
}
