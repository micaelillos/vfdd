/*
 * jsonpath.c -  json data struct to find records in the json tree.
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <jsonpath.h>
#include <strmem.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 *** \brief Allocates memory for a new JsonPath object.
 */

JsonPath *jsonpath_new( const char *path )
{
   JsonPath *jp;

   jp =  app_new0(JsonPath, 1);
   jsonpath_construct( jp, path );
   app_class_overload_destroy( (AppClass *) jp, jsonpath_destroy );
   return jp;
}

/** \brief Constructor for the JsonPath object. */

void jsonpath_construct( JsonPath *jp, const char *path )
{
   app_class_construct( (AppClass *) jp );

   char *sep =  PATH_SEP; 
   char *p = (char *) path;
   int i;

   if ( strspn( p, "./!#$%,:;=" ) ){
      sep = p++;
   }

   if ( *p == '*' && *(p + 1) == *sep ) {
      p += 2;
      jp->recur = 1;
   } else if ( *p == *sep ) {
      p += 1;
   }
   jp->sn = app_strdup(p);
   for ( p = jp->sn, i = 0 ; *p ; p++ ){
      if ( *p == *sep ){
         i++;
      }
   }
   i++;
   jp->count = i ;

   jp->tbl = (char **) app_new0(char **, i + 1);
   for ( i = 0, p = jp->sn ; i < jp->count ; i++ ){
      jp->tbl[i] = p;
      while ( *p && *p != *sep ){
         p++;
      }
      *p++ = 0;
   }
   jsonpath_next_tok( jp );
}

/** \brief Destructor for the JsonPath object. */

void jsonpath_destroy(void *jp)
{
   JsonPath *this = (JsonPath *) jp;

   if (jp == NULL) {
      return;
   }
   app_free( this->tbl );
   app_free( this->sn );

   app_class_destroy( jp );
}

char *jsonpath_next_tok( JsonPath *jp)
{
   if ( jp->i <= jp->count ){
      jp->tok = jp->tbl[jp->i++];
   }
   return  jp->tok;
}
