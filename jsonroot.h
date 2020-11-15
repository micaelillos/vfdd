#ifndef JSONROOT_H
#define JSONROOT_H

/*
 * jsonroot.h - json root node interface
 * 
 * include LICENSE
 */

#include <jsonnode.h>
#include <dbuf.h>

#define json_root_set_item_int(object, path, value)  \
                (int) json_root_set_item_double(object, path, (double) value)  

typedef struct _JsonRootNode JsonRootNode;

struct _JsonRootNode {
   JsonNode parent;
   DBuf *src;        /* the json text buffer to parse */
   char *curptr;     /* current pointer to the source */
   char *errptr;     /* error pointer to the source */
   char *path;       /* current element path */
   DBuf *dbuf;       /* buffer to store the coding decoding */
   DBuf *dest;       /* pointer to a dbuf */
   int count;        /* temp value of element number in array object */
   int depth;        /* recursion depth for indentation */
   int format;       /* if set, format the output */
   char expected;    /* set when a character is expected */
};

/*
 * prototypes
 */
JsonRootNode *json_root_new( DBuf *src, int type );
void json_root_construct( JsonRootNode *root, DBuf *src, int type );
void json_root_destroy(void *root);

void json_root_print_error(JsonRootNode *root);
void json_root_set_src( JsonRootNode *root, DBuf *src);
void json_root_set_format( JsonRootNode *root, int format);

char *json_root_parse( JsonRootNode *root, int check_end );
char *json_root_parse_str( JsonRootNode *root, char *text, int check_end );

int json_root_dbuf_print(JsonRootNode *root, JsonNode *item, DBuf *dest, int format );
int json_root_print(JsonRootNode *root, int format );


JsonNode *json_root_add_string_2object(JsonNode *object, const char *key,
                                  const char *string);
JsonNode *json_root_add_number_2object(JsonNode *object, const char *key, double value);
JsonNode *json_root_add_null_2object(JsonNode *object, const char *key);
JsonNode *json_root_add_true_2object(JsonNode *object, const char *key);
JsonNode *json_root_add_false_2object(JsonNode *object, const char *key);
JsonNode *json_root_add_bool_2object(JsonNode *object, const char *key,
                                     int val);

void json_root_add_item_2object(JsonNode *object, JsonNode *item);

JsonNode *json_root_get_item_string(JsonNode *object, const char *path,
                                   char ** result );
JsonNode *json_root_dup_item_string(JsonNode *object, const char *path,
                                const char *string );
JsonNode *json_root_get_item_bool(JsonNode *object, const char *path,
                                  int *result );
JsonNode *json_root_get_item_int(JsonNode *object, const char *path,
                                    int *result );
JsonNode *json_root_set_item_bool(JsonNode *object, const char *path,
                                int value );
JsonNode *json_root_get_item_double(JsonNode *object, const char *path,
                                    double *result );
JsonNode *json_root_set_item_double(JsonNode *object, const char *path,
                                double value );
JsonNode *json_root_get_item_nelem(JsonNode *object, const char *path, int *n );

#endif /* JSONROOT_H */
