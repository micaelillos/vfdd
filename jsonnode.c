/*
 * jsonnode.c - json_node interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <jsonnode.h>
#include <strmem.h>
#include <duprintf.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 *** \brief Allocates memory for a new JsonNode object.
 */

JsonNode *json_node_new( const char *keyname, int type )
{
   JsonNode *node;

   node =  app_new0(JsonNode, 1);
   json_node_construct( node, keyname, type );
   app_class_overload_destroy( (AppClass *) node, json_node_destroy );
   return node;
}

/** \brief Constructor for the JsonNode object. */

void json_node_construct( JsonNode *node, const char *keyname, int type )
{
   app_class_construct( (AppClass *) node );
   json_node_dup_keyname(node, keyname );
   node->jsonType = type;
}

/** \brief Destructor for the JsonNode object. */

void json_node_destroy(void *node)
{
   JsonNode *this = (JsonNode *) node;

   if (node == NULL) {
      return;
   }
   app_free(this->keyname);
   if ( this->jsonType == JSON_STRING ){
      void *ptr = (void *) this->u.ptr;
      app_free(ptr);
   }
   dlist_delete_list(&this->child);

   app_class_destroy( node );
}

void json_node_set_type(JsonNode *node, int type)
{
   double d = 0.0;
   node->jsonType = type;
   if ( type == JSON_FALSE ){
      d = (double) 0;
   } else if ( type == JSON_TRUE ){
      d = (double) 1;
   }
   node->u.value = d; 
}

int json_node_get_type(JsonNode *node)
{
   return node->jsonType;
}

int json_node_is_null(JsonNode *node)
{
   return (node->jsonType == JSON_NULL) ;
}

int json_node_is_false(JsonNode *node)
{
   return (node->jsonType == JSON_FALSE) ;
}

int json_node_is_true(JsonNode *node)
{
   return (node->jsonType == JSON_TRUE) ;
}

void json_node_get_val_int(JsonNode *node, int *result )
{
   double res;
   json_node_get_val_double(node, &res );
   *result = (int) res;
}
void json_node_get_val_double(JsonNode *node, double *result )
{
   if ( node->jsonType != JSON_NUMBER && node->jsonType != JSON_TRUE &&
      node->jsonType != JSON_FALSE ){
      msg_error( "Request to non integer value 0x%x", node);
      *result = 0.0;
   }
   *result = node->u.value;
}

void json_node_get_val_string(JsonNode *node,  char **result )
{
   if ( node->jsonType != JSON_STRING ){
      msg_error( "Request to non integer value 0x%x", node);
      *result = NULL;
   }
   *result = node->u.ptr;
}

void json_node_set_val_number(JsonNode *node, double value)
{
   if ( node->jsonType != JSON_UNDEF &&
        node->jsonType != JSON_NUMBER ) {
      msg_error( "Node value is not a number 0x%x", node);
      return;
   }
   node->jsonType = JSON_NUMBER;
   node->u.value = value;
}

void json_node_dup_val_string(JsonNode *node, const char *string )
{
   if ( node->jsonType != JSON_UNDEF &&
        node->jsonType != JSON_STRING ) {
      msg_error( "Node value is not a string 0x%x", node);
      return;
   }
   node->jsonType = JSON_STRING;
   app_free(node->u.ptr);
   node->u.ptr = app_strdup( string );
}

void json_node_dup_keyname(JsonNode *node, const char *keyname )
{
   app_dup_str( &node->keyname, ( char *) keyname );
}

void json_node_set_keyname_from_value(JsonNode *node, int index)
{
   if ( node->jsonType != JSON_UNDEF && node->jsonType != JSON_STRING ){
      msg_error( "Node value is not a string 0x%x", node);
      return;
   }
   node->keyname = node->u.ptr;
   node->u.ptr = NULL;
   if ( node->keyname == NULL ){
      node->keyname = app_strdup_printf("%d", index );
   }
   node->jsonType = JSON_UNDEF;
}

char *json_node_get_keyname(JsonNode *node )
{
   return  node->keyname;
}

/*
 * return the node corresponding to path directly descending of current node.
 *   path may be something like ".*.path_tosearch"
 *   int this case path_tosearch is recursively searched.
 */

JsonNode *json_node_find_node(JsonNode *object, const char *path )
{
   if ( ! object || ! path ){
      return NULL;
   }
   JsonPath *jp = jsonpath_new( path );

   JsonNode *node = json_node_find( object, jp );

   jsonpath_destroy(jp);
   return node;
}

JsonNode *json_node_find(JsonNode *object, JsonPath *jp )
{
   int descend = 0;
   JsonNode *node;
   
   DList *child = object->child;
   if ( ! child ){
      return NULL;
   }
   DList *nodlist = child->next;

   while (nodlist != child) {
      node = (JsonNode *) nodlist->data;
      if ( node->keyname && app_strcmp( node->keyname , jp->tok ) == 0 ) {
         jsonpath_next_tok( jp );
         if ( ! jp->tok ){ /* path is found */
            return node;
         }
         descend = 1;
      }
         
      if ( jp->recur || descend ) {
         jp->level++;
         JsonNode *n = json_node_find( node, jp );
         jp->level--;
         if ( ! jp->tok || jp->recur == 0 ){
            return n;
         }
      }
      nodlist = nodlist->next;
   }
   return NULL;
}


JsonNode *json_node_get_nth_child(JsonNode *object, int n )
{
   JsonNode *node = (JsonNode *) object->child;
   if ( node ){
      node = (JsonNode *) dlist_get_ndata(object->child, n);
   }
   return node;
}

int json_node_keyname_cmp(AppClass *d1, AppClass *d2 )
{
   JsonNode *node = (JsonNode *) d1 ;
   char *name = (char *) d2 ;
   return app_strcmp( node->keyname, name );
}

int json_node_key_node_cmp(AppClass *d1, AppClass *d2 )
{
   JsonNode *node1 = (JsonNode *) d1 ;
   JsonNode *node2 = (JsonNode *) d2 ;
   return app_strcmp( node1->keyname, node2->keyname );
}

