#ifndef JSONNODE_H
#define JSONNODE_H

/*
 * jsonnode.h - json data node protocol interface
 * 
 * include LICENSE
 */

#include <dlist.h>
#include <jsonpath.h>

enum _JsonTypeInfo {
   JSON_UNDEF = 0,    /* 0 */
   JSON_FALSE,        /* 1 */
   JSON_TRUE,         /* 2 */
   JSON_NULL,         /* 3 */
   JSON_NUMBER,       /* 4 */
   JSON_STRING,       /* 5 */
   JSON_ARRAY,        /* 6 */
   JSON_OBJECT,       /* 7 */
};

typedef struct _JsonNode JsonNode;

struct _JsonNode {
   AppClass parent;
   DList *child;    /* A list of object items (children JsonNode) . */
   char *keyname;   /* the key associated to the value */
   union {
      double value;    /* 8 bytes: The value of item:  string, int, double */
      char *ptr;
   } u;
   int jsonType;    /* The type of the item. */
};

/*
 * prototypes
 */
JsonNode *json_node_new( const char *keyname, int type );
void json_node_construct( JsonNode *node, const char *keyname, int type );
void json_node_destroy(void *node);

void json_node_set_type(JsonNode *node, int type);
int json_node_get_type(JsonNode *node);

int json_node_is_null(JsonNode *node);
int json_node_is_false(JsonNode *node);
int json_node_is_true(JsonNode *node);

void json_node_get_val_double(JsonNode *node, double *result );
void json_node_get_val_int(JsonNode *node, int *result );
void json_node_get_val_string(JsonNode *node, char **result );
void json_node_set_val_number(JsonNode *node, double value);
void json_node_dup_val_string(JsonNode *node, const char *string );

void json_node_dup_keyname(JsonNode *node, const char *keyname );
char *json_node_get_keyname(JsonNode *node );
void json_node_set_keyname_from_value(JsonNode *node, int index );

int json_node_keyname_cmp(AppClass *d1, AppClass *d2 );
int json_node_key_node_cmp(AppClass *d1, AppClass *d2 );

JsonNode *json_node_find(JsonNode *object, JsonPath *jp );
JsonNode *json_node_find_node(JsonNode *object, const char *path );
JsonNode *json_node_get_nth_child(JsonNode *object, int n );

#endif /* JSONNODE_H */
