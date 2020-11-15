/*
 * jsonroot.c - json_root interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include <jsonroot.h>
#include <c2hex.h>
#include <utf8.h>
#include <duprintf.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 * local prototypes
 */
static char *json_root_parse_value(JsonRootNode *root, JsonNode *item );
static int json_root_print_value(JsonRootNode *root, JsonNode *item );
char *json_root_get_hex( JsonRootNode *root,  unsigned int *uc);
char *json_root_check_utf( JsonRootNode *root,  unsigned int *uc);
int json_root_iter_object( AppClass *data, void *user_data );
int json_root_iter_array( AppClass *data, void *user_data );

/*
 *** \brief Allocates memory for a new JsonRootNode object.
 */

JsonRootNode *json_root_new( DBuf *src, int type )
{
   JsonRootNode *root;

   root =  app_new0(JsonRootNode, 1);
   json_root_construct( root, src, type );
   app_class_overload_destroy( (AppClass *) root, json_root_destroy );
   return root;
}

/** \brief Constructor for the JsonRootNode object. */

void json_root_construct( JsonRootNode *root, DBuf *src, int type)
{
   json_node_construct( (JsonNode *) root, NULL, type );
   root->src = src;
   root->dbuf = dbuf_new( 1024, 0 );
}

/** \brief Destructor for the JsonRootNode object. */

void json_root_destroy(void *root)
{
   JsonRootNode *this = (JsonRootNode *) root;

   if (root == NULL) {
      return;
   }
   if ( this->dbuf ){
      app_class_unref((AppClass *) this->dbuf);
   }
   app_free(this->path);
   
   json_node_destroy( root );
}

void json_root_set_src(JsonRootNode *root, DBuf *src)
{
   root->src = src;
   root->expected = 0;
}


#define TEXTSIZE 16
void json_root_print_error(JsonRootNode *root)
{
   char *bufsrc = app_strndup(root->errptr, TEXTSIZE);
   char *expected = app_strdup("");
   if ( root->expected ){
      expected = app_strdup_printf(", expected '%c' or ','.", root->expected );
   }
   msg_error("Parse error at line %d, got '%s'... %s\n"
             "src pos %d",
             1 + dbuf_get_lineno(root->src),  bufsrc, expected,
             root->src->pos );
   app_free(bufsrc);
   app_free(expected);
}


void json_root_set_format(JsonRootNode *root, int format)
{
   root->format = format;
}

/*
 * skip whitespace on the input
 */
static char *json_root_skip_ws(JsonRootNode *root)
{
   char *p;
   
   while ( (p = dbuf_next_char(root->src)) && *p ) {
      if ( ! isspace((int) *p) ){
         root->curptr = p;
         return p;
      }
   }
   
   return NULL;
}

static void json_root_update_path(JsonRootNode *root, char *key)
{
   char *p;
   
   if ( key ){
      if ( ! root->path ){
         root->path = app_strdup("");
      }
      p = app_strdup_printf ("%s%s%s", root->path, PATH_SEP, key );
      app_free( root->path);
      root->path = p;
   } else {
      p = PATH_SEP;
      p = strrchr(root->path, *p );
      if ( p ){
         *p = 0;
      }
   }
}

/*
 * create an json number.
 */
static char *json_root_parse_number(JsonRootNode *root, JsonNode *item )
{
   char *p = root->curptr;
   double n = 0;
   char *endptr;

   /* numeric string to dbuf */   
   dbuf_clear(root->dbuf);
   do {
      if ( *p == '-' || (*p >= '0' && *p <= '9') || *p == '.' ||
          *p == '+' ||  *p == 'e' || *p == 'E'){
         dbuf_put_char( root->dbuf, *p );
      } else {
         dbuf_unget_char(root->src);
         break;
      }
   } while ( (p = dbuf_next_char(root->src))  );

   if ( ! p ){
      return NULL;
   }
   errno = 0; /* To distinguish success/failure after call */
   n = strtod(root->dbuf->s, &endptr);
   if ( errno != 0 || root->curptr == endptr ) {
      if ( errno != 0 ) {
         msg_error( "strtod - %s", strerror(errno));
      } else {
         msg_error( "strtod - no digits found" );
      }
      root->errptr = endptr;
      return NULL;
   }
   json_node_set_val_number( item, n);
   return p;
}

char *json_root_get_hex( JsonRootNode *root,  unsigned int *uc)
{
   char *p;
   int i;
   int c;
   
   for (i = 0;  i < 4 && (p = dbuf_next_char(root->src)); i++ ){
      if ( (c = c_to_hex(*p)) < 0 ) {
         p = NULL;
         break;
      }
      *uc <<= 4;
      *uc |= c;
   }
   return p;
}

char *json_root_check_utf( JsonRootNode *root,  unsigned int *uc)
{
   char *p;
   unsigned int uc1 = 0;
   unsigned int uc2 = 0;

   if ( (p = json_root_get_hex( root, &uc1 )) == NULL ){
      return NULL;
   }
   if ((uc1 >= 0xDC00 && uc1 <= 0xDFFF) || uc1 == 0){
      return NULL;      /* check for invalid.   */
   }
   *uc = uc1;
   
   if (uc1 >= 0xD800 && uc1 <= 0xDBFF) { /* UTF16 surrogate pairs. */
      p = dbuf_next_char(root->src);
      if ( p == NULL || *p != '\\' ){
         return NULL;
      }
      p = dbuf_next_char(root->src);
      if ( p == NULL || *p != 'u' ){
         return NULL;
      }
      if ( (p = json_root_get_hex( root, &uc2 )) == NULL ){
         return NULL;
      }
      if (uc2 < 0xDC00 || uc2 > 0xDFFF){
         return NULL;        /* invalid second-half of surrogate.    */
      }
      *uc = 0x10000 + (((uc1 & 0x3FF) << 10) | (uc2 & 0x3FF));
   }
   return p;
}

/*
 * create an json string.
 */
static char *json_root_parse_string(JsonRootNode *root, JsonNode *item )
{
   char *p = root->curptr;
   unsigned int uc = 0;

   if (*p != '"' ) {
      root->expected = '"';
      goto ret_error;
   }
   dbuf_clear(root->dbuf);
   while ( (p = dbuf_next_char(root->src)) && *p ){
      if ( *p == '"' ){ /* end of string */
         break;
      }
      if (*p != '\\'){
         dbuf_put_char( root->dbuf, *p );
      } else {
         if ((p = dbuf_next_char(root->src)) == NULL ){
            break;
         }
         switch (*p) {
          default:
            dbuf_put_char( root->dbuf, *p );
            break;
          case 'b':
            dbuf_put_char( root->dbuf, '\b');
            break;
          case 'f':
            dbuf_put_char( root->dbuf, '\f');
            break;
          case 'n':
            dbuf_put_char( root->dbuf, '\n');
            break;
          case 'r':
            dbuf_put_char( root->dbuf, '\r');
            break;
          case 't':
            dbuf_put_char( root->dbuf, '\t');
            break;
          case 'u':              /* transcode utf16 to utf8. */
            if ( json_root_check_utf( root, &uc) == NULL ){
               return NULL;
            }
            dbuf_strcat(root->dbuf, utf8_code2chars( uc ));
            break;
         }
      }
   }
   if (*p == '"'){
      json_node_dup_val_string( item, root->dbuf->s );
      return p + 1;        /* end of string */
   }
   root->expected = '"';

ret_error:
   root->errptr = p;
   return NULL;
}


/*
 * create an json array.
 */
static char *json_root_parse_array(JsonRootNode *root, JsonNode *item )
{
   char *p = root->curptr;
   int save_count;
   
   if (*p != '[') {
      root->expected = '[';
      goto ret_error;
   }
   json_node_set_type(item, JSON_ARRAY);
   if ( ! (p = json_root_skip_ws(root)) ){
      return NULL;
   }
   if (*p == ']'){
//      root->curptr = dbuf_next_char(root->src);
      return p + 1;		/* empty array. */
   }
   dbuf_unget_char(root->src);
   root->count = 0;
   
   do {
      JsonNode *child = json_node_new( NULL, 0 );

      json_node_set_keyname_from_value(child, root->count );
      json_root_update_path( root, json_node_get_keyname(child) );
      JsonNode *node = json_node_find_node( (JsonNode *) root, root->path );
      if ( node ){ /* node already exist - use it */
         json_node_destroy(child);
         child = node;
      } else {
         item->child = dlist_add_tail(item->child, (AppClass *) child );
      }

      save_count = root->count;
      p = json_root_parse_value(root, child);
      root->count = save_count;
      json_root_update_path( root, NULL );

      if ( ! p ){
         return NULL;
      }
      
      root->count++;
      if ( ! (p = json_root_skip_ws(root)) ){
         return NULL;
      }

   } while (*p == ',');

   if (*p == ']'){
      return p + 1;         /* end of array */
   }
   root->expected = ']';

ret_error:
   root->errptr = p;
   return NULL;			/* malformed. */
}

/*
 * create an json object.
 */
static char *json_root_parse_object(JsonRootNode *root, JsonNode *item )
{
   char *p = root->curptr;
   int save_count;
   
   if (*p != '{') {
      root->expected = '{';
      goto ret_error;
   }
   json_node_set_type(item, JSON_OBJECT);
   if ( ! (p = json_root_skip_ws(root)) ){
      return NULL;
   }
   if (*p == '}'){
//      root->curptr = dbuf_next_char(root->src);
      return p + 1;		/* empty object. */
   }
   root->count = 0;
   
   do {
      JsonNode *child = json_node_new( NULL, 0 );

      if ( *p == ',' ){
         p = json_root_skip_ws(root);
         if ( ! p ){
            return NULL;
         }
      }
      p = json_root_parse_string(root, child);
      if ( ! p ){
         return NULL;
      }
      json_node_set_keyname_from_value(child, root->count );
      json_root_update_path( root, json_node_get_keyname(child) );
      JsonNode *node = json_node_find_node( (JsonNode *) root, root->path );
      if ( node ){ /* node already exist - use it */
         json_node_destroy(child);
         child = node;
      } else {
         item->child = dlist_add_tail(item->child, (AppClass *) child );
      }
      if ( ! (p = json_root_skip_ws(root)) ){
         return NULL;
      }
      if (*p != ':') {
         root->expected = ':';
         goto ret_error;
      }
      save_count = root->count;
      p = json_root_parse_value(root, child);
      root->count = save_count;
      json_root_update_path( root, NULL );

      if ( ! p ){
         return NULL;
      }
      
      root->count++;
      if ( ! (p = json_root_skip_ws(root)) ){
         return NULL;
      }
      
   } while (*p == ',');

   if (*p == '}'){
      return p + 1;
   }
   root->expected = '}';
   
ret_error:
   root->errptr = p;
   return NULL;			/* malformed. */
}

/*
 * main Parser
 * after json_root_parse_value the dbuf pos should point to first not processed 
 */
static char *json_root_parse_value(JsonRootNode *root, JsonNode *item )
{
   char *p;
   if ( ! (p = json_root_skip_ws(root)) ){
      return NULL;
   }
   if ( ! app_strncmp(p, "null", 4)) {
      json_node_set_type(item, JSON_NULL);
      dbuf_set_pos(root->src, root->src->pos + 3);
      return p + 4;
   }
   if ( ! app_strncmp(p, "false", 5)) {
      json_node_set_type(item, JSON_FALSE);
      dbuf_set_pos(root->src, root->src->pos + 4);
      return p + 5;
   }
   if ( ! app_strncmp(p, "true", 4)) {
      json_node_set_type(item, JSON_TRUE);
      dbuf_set_pos(root->src, root->src->pos + 3);
      return  p + 4;
   }
   if (*p == '\"') {
      return json_root_parse_string(root, item);
   }
   if (*p == '-' || (*p >= '0' && *p <= '9')) {
      return json_root_parse_number(root, item);
   }
   if (*p == '[') {
      return json_root_parse_array(root, item);
   }
   if (*p == '{') {
      return json_root_parse_object(root, item);
   }

   /* unexpected char or end of string */
   root->errptr = p;
   return NULL;
}

/*
 * parse the json text
 * return NULL if error
 * return pointer on the src if OK
 */
char *json_root_parse( JsonRootNode *root, int check_end )
{
   app_dup_str(&root->path, NULL);
   char *p = json_root_parse_value(root, (JsonNode *) root );
   app_dup_str(&root->path, NULL);
   if ( ! p ) {
      return NULL;
   }

   if (check_end) {
      p = json_root_skip_ws(root);
      if ( *p ) {
         root->errptr = root->curptr;
	 return NULL;
      }
   }
   return p;
}

char *json_root_parse_str( JsonRootNode *root, char *text, int check_end )
{
   root->src = dbuf_new( 0, 0 );
   dbuf_strcpy(root->src, text);

   char *p = json_root_parse( root, check_end );   
   dbuf_destroy(root->src);
   return p;
}

static int json_root_print_number(JsonRootNode *root, JsonNode *item)
{
   double d;
   json_node_get_val_double(item, &d);
   int ii = (int) d;

   if (fabs( d - (double) ii) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN) {
      dbuf_printf(root->dest, "%d", ii);
   } else {
      if (fabs(floor(d) - d) <= DBL_EPSILON && fabs(d) < 1.0e60) {
         dbuf_printf(root->dest, "%.0f", d);
      } else if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9) {
         dbuf_printf(root->dest, "%e", d);
      } else {
         dbuf_printf(root->dest, "%f", d);
      }
   }
   return 1;
}

static int json_root_print_str(JsonRootNode *root, char *str)
{
   unsigned char *p;
   unsigned int uc;
   
   if ( ! str){
      return 1;
   }
   dbuf_put_char(root->dest, '"');
   p = (unsigned char *) str;
   while (*p) {
      if ( *p >= 0x20  && *p != '\"' && *p != '\\'){
         p++;
      } else {
         dbuf_strncat(root->dest, str, p - (unsigned char *) str );
         dbuf_put_char(root->dest, '\\');
         uc = *p;
         switch (*p++) {
          case '\\':
            dbuf_put_char(root->dest, '\\');
            break;
          case '\"':
            dbuf_put_char(root->dest, '"');
            break;
          case '\b':
            dbuf_put_char(root->dest, 'b');
            break;
          case '\f':
            dbuf_put_char(root->dest, 'f');
            break;
          case '\n':
            dbuf_put_char(root->dest, 'n');
            break;
          case '\r':
            dbuf_put_char(root->dest, 'r');
            break;
          case '\t':
            dbuf_put_char(root->dest, 't');
            break;
          default:
            dbuf_printf(root->dest, "u%04x", uc);
            break;              /* escape and print */
         }
         str = (char *) p;
      }
   }
   dbuf_strncat(root->dest, str, p - (unsigned char *) str );
   dbuf_put_char(root->dest, '"');
   return 1;
}

static int json_root_print_string(JsonRootNode *root, JsonNode *item)
{
   char *str;
   json_node_get_val_string(item, &str );
   return json_root_print_str(root, str );
}

int json_root_iter_array( AppClass *data, void *user_data )
{
   JsonNode *node = (JsonNode *) data;
   JsonRootNode *root = (JsonRootNode *) user_data;

   int save_count = root->count;
   json_root_print_value(root, node );
   root->count = save_count;

   root->count--;
   if ( root->count != 0 ){
      dbuf_put_char(root->dest, ',');
      if ( root->format ){
         dbuf_put_char(root->dest, ' ');
      }
   }
   
   return 0; /* do not remove the node */
}

static int json_root_print_array(JsonRootNode *root, JsonNode *item)
{
   dbuf_put_char(root->dest, '[');
   root->count = dlist_get_nelem(item->child);
   root->depth++;
   dlist_iterator(item->child, json_root_iter_array, root);
   root->depth--;
   dbuf_put_char(root->dest, ']');
   return 1;
}

int json_root_iter_object( AppClass *data, void *user_data )
{
   JsonNode *node = (JsonNode *) data;
   JsonRootNode *root = (JsonRootNode *) user_data;
   int i;

   if (root->format){
      for (i = 0; i < root->depth; i++){
         dbuf_strcat(root->dest, "    ");
      }
   }
   json_root_print_str(root, node->keyname );
   dbuf_put_char(root->dest, ':');
   if (root->format){
      dbuf_put_char(root->dest, '\t');
   }

   int save_count = root->count;
   json_root_print_value(root, node );
   root->count = save_count;
   
   root->count--;
   if ( root->count != 0 ){
      dbuf_put_char(root->dest, ',');
      if ( root->format ){
         dbuf_put_char(root->dest, ' ');
      }
   }
   if (root->format){
      dbuf_put_char(root->dest, '\n');
   }
   
   return 0; /* do not remove the node */
}

static int json_root_print_object(JsonRootNode *root, JsonNode *item)
{
   int i;
   
   dbuf_put_char(root->dest, '{');
   if (root->format){
      dbuf_put_char(root->dest, '\n');
   }

   root->count = dlist_get_nelem(item->child);
   root->depth++;
   dlist_iterator(item->child, json_root_iter_object, root);

   root->depth--;
   if (root->format){
      for (i = 0; i < root->depth; i++){
         dbuf_strcat(root->dest, "    ");
      }
   }
   dbuf_put_char(root->dest, '}');
   return 1;
}

static int json_root_print_value(JsonRootNode *root, JsonNode *item )
{
   int ret = 1;
   
   if ( ! item){
      return 0;
   }
   int type = json_node_get_type(item);
   switch (type) {
    case JSON_NULL:
      dbuf_strcat(root->dest, "null");
      break;
   case JSON_FALSE:
      dbuf_strcat(root->dest, "false");
      break;
   case JSON_TRUE:
      dbuf_strcat(root->dest, "true");
      break;
   case JSON_NUMBER:
      ret = json_root_print_number(root, item);
      break;
   case JSON_STRING:
      ret = json_root_print_string(root, item);
      break;
   case JSON_ARRAY:
      ret = json_root_print_array(root, item);
      break;
   case JSON_OBJECT:
      ret = json_root_print_object(root, item);
      break;
   }
   return ret;
}

/*
 * dump the json tree and append result in dbuf dest
 */
int json_root_dbuf_print(JsonRootNode *root, JsonNode *item, DBuf *dest, int format )
{
   root->dest = dest;
   root->format = format;
   root->depth = 0;
   int ret = json_root_print_value(root, item );
   if ( format ){
      dbuf_put_char(root->dest, '\n');
   }
   return ret;
}

/*
 * dump all the json tree in dbuf root->dbuf
 */
int json_root_print(JsonRootNode *root, int format )
{
   dbuf_clear(root->dbuf);
   return json_root_dbuf_print( root, (JsonNode *) root, root->dbuf, format );
}

/*
 * fonction to programmaticaly create a tree
 */
void json_root_add_item_2object(JsonNode *object, JsonNode *item)
{
   if ( ! item) {
      return;
   }
   object->child = dlist_add_tail(object->child, (AppClass *) item );
}

JsonNode *json_root_add_bool_2object(JsonNode *object, const char *key,
                                     int val )
{
   if ( val ) {
      return json_root_add_true_2object( object, key);
   } else {
      return json_root_add_false_2object( object, key);
   }
}

JsonNode *json_root_add_false_2object(JsonNode *object, const char *key)
{
   JsonNode *node = (JsonNode *) dlist_lookup(object->child, (AppClass *) key,
                                               json_node_keyname_cmp );
   if ( node ){
      json_node_set_type( node, JSON_FALSE );
   } else {
      node = json_node_new( key, JSON_FALSE );
      object->child = dlist_add_tail(object->child, (AppClass *) node );
   }
   return node;
}

JsonNode *json_root_add_true_2object(JsonNode *object, const char *key)
{
   JsonNode *node = (JsonNode *) dlist_lookup(object->child, (AppClass *) key,
                                               json_node_keyname_cmp );
   if ( node ){
      json_node_set_type( node, JSON_TRUE);
   } else {
      node = json_node_new( key, JSON_TRUE );
      object->child = dlist_add_tail(object->child, (AppClass *) node );
   }
   return node;
}

JsonNode *json_root_add_null_2object(JsonNode *object, const char *key)
{
   JsonNode *node = (JsonNode *) dlist_lookup(object->child, (AppClass *) key,
                                               json_node_keyname_cmp );
   if ( node ){
      json_node_set_type( node, JSON_NULL);
   } else {
      node = json_node_new( key, JSON_NULL );
      object->child = dlist_add_tail(object->child, (AppClass *) node );
   }
   return node;
}

JsonNode *json_root_add_number_2object(JsonNode *object, const char *key, double value)
{
   JsonNode *node = (JsonNode *) dlist_lookup(object->child, (AppClass *) key,
                                               json_node_keyname_cmp );
   if ( node ){
      json_node_set_val_number( node, value);
   } else {
      node = json_node_new( key, JSON_NUMBER );
      json_node_set_val_number( node, value);
      object->child = dlist_add_tail(object->child, (AppClass *) node );
   }
   return node;
}

/*
 * if dest node exist, modify it
 */
JsonNode *json_root_add_string_2object(JsonNode *object, const char *key,
                                  const char *string)
{
   JsonNode *node = (JsonNode *) dlist_lookup(object->child, (AppClass *) key,
                                               json_node_keyname_cmp );
   if ( node ){
      json_node_dup_val_string( node, string);
   } else {
      node = json_node_new( key, JSON_STRING );
      json_node_dup_val_string( node, string);
      object->child = dlist_add_tail(object->child, (AppClass *) node );
   }
   return node;
}

JsonNode *json_root_get_item_string(JsonNode *object, const char *path,
                                char **result )
{
   JsonNode *node = json_node_find_node(object, path );
   if ( node ){
      json_node_get_val_string(node, result );
   } else {
      *result = NULL;
   }
   return node;
}

JsonNode *json_root_dup_item_string(JsonNode *object, const char *path,
                                const char *string )
{
   JsonNode *node = json_node_find_node(object, path );
   if ( ! node ){
      return NULL;
   }
   json_node_dup_val_string(node, string );
   return node;
}

JsonNode *json_root_get_item_int(JsonNode *object, const char *path,
                                   int *result )
{
   JsonNode *node = json_node_find_node(object, path );
   if ( node ){
      json_node_get_val_int(node, result );
   } else {
      *result = 0;
   }
   return node;
}

JsonNode *json_root_get_item_double(JsonNode *object, const char *path,
                                   double *result )
{
   JsonNode *node = json_node_find_node(object, path );
   if ( node ){
      json_node_get_val_double(node, result );
   } else {
      *result = 0.0;
   }
   return node;
}

JsonNode *json_root_set_item_double(JsonNode *object, const char *path,
                                double value )
{
   JsonNode *node = json_node_find_node(object, path );
   if ( ! node ){
      return NULL;
   }
   json_node_set_val_number(node, value );
   return node;
}

JsonNode *json_root_get_item_bool(JsonNode *object, const char *path,
                                  int *result )
{
   JsonNode *node = json_node_find_node(object, path );
   if ( node ){
      if ( node->jsonType == JSON_FALSE || node->jsonType == JSON_TRUE ){
         json_node_get_val_int(node, result);
      } else {
         msg_error("node '%s' is not boolean", path);
         *result = 0;
      }
   } else {
      *result = 0;
   }
   return node;
}

JsonNode *json_root_set_item_bool(JsonNode *object, const char *path,
                                int value )
{
   int type = JSON_FALSE;
   JsonNode *node = json_node_find_node(object, path );
   if ( ! node ){
      return NULL;
   }
   if ( node->jsonType != JSON_FALSE || node->jsonType != JSON_TRUE ){
      msg_error("node '%s' is not boolean", path);
      return node;
   }
   if ( value ) {
      type = JSON_TRUE;
   }
   json_node_set_type(node, type );
   return node;
}

/*
 * get the number of elements in the node addressed by path
 */
JsonNode *json_root_get_item_nelem(JsonNode *object, const char *path, int *n )
{
   JsonNode *node = json_node_find_node(object, path );
   if ( node ){
      *n =  dlist_get_nelem(node->child);
   } else {
      *n = 0;
   }
   return node;
}

