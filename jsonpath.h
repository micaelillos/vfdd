#ifndef JSONPATH_H
#define JSONPATH_H

/*
 * jsonpath.h - json data struct to find records in the jsaon tree
 * 
 * include LICENSE
 */

#include <appclass.h>

/*
 * PATH_SEP is internal representation of a nodes hierarchy.
 * When looking for a specific node it is convenient to use a string
 * ".string1.string2.string3" as a hierarchy to search.
 * jsonpath consider the first character to be used as separator.
 * It can be "./!#$%,:;="
 * a beginning '.*' means ignore the first conponents in the object path.
 * "*.string3" will match ".string1.string2.string3".
 */

#define PATH_SEP  "."

typedef struct _JsonPath JsonPath;

struct _JsonPath {
   AppClass parent;
   char *sn;        /* copy of the path */
   char **tbl;      /* table of pointers on members of the path */
   char *tok;       /* pointer on the current member to search */
   int count;       /* count of members of the path */
   int i;           /* current index in tbl */
   int recur;       /* flag indicating a recursive search */
   int level;       /* recursion level for debug */
};

/*
 * prototypes
 */
JsonPath *jsonpath_new(  const char *path );
void jsonpath_construct( JsonPath *jp,  const char *path );
void jsonpath_destroy(void *jp);

char *jsonpath_next_tok( JsonPath *jp);

#endif /* JSONPATH_H */
