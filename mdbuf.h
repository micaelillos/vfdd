#ifndef MDBUF_H
#define MDBUF_H

/*
 * mdbuf.h - a dbuf like memory mapped object functions.
 * 
 * include LICENSE
 */

#include <dbuf.h>
#include <mutil.h>

typedef struct _MDbuf MDbuf;

struct _MDbuf {
   DBuf parent;
   char *filename;             /* the name of the file to open */
   char *addr;                 /* addr returned by mmap */
   size_t size;                /* size of the data file */
   int status;                 /* op status, 0 = OK  */
};

/*
 * prototypes
 */
MDbuf *mdbuf_new( const char *filename, int openflags );
void mdbuf_construct( MDbuf *dbuf, const char *filename, int openflags );
void mdbuf_destroy(void *dbuf);


#endif /* MDBUF_H */
