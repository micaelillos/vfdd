/*
 * mdbuf.c - a dbuf like memory mapped object functions.
 *           mmap a file and access it like a dbuf
 * MDbuf *mdbuf = (DBuf *) mdbuf_new( absfile, O_RDONLY);
 *        if ( mdbuf->status ) {
 *           mdbuf_destroy(mdbuf);
 *           return
 *        }
 *        while (( c = dbug_get_char((DBuf *) mdbuf)) >= 0 ){
 *            ...
 *        }
 * mdbuf_destroy(mdbuf);
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <mdbuf.h>

/*
 *** \brief Allocates memory for a new MDbuf object.
 */

MDbuf *mdbuf_new( const char *filename, int openflags )
{
   MDbuf *mdbuf;

   mdbuf =  app_new0(MDbuf, 1);
   mdbuf_construct( mdbuf, filename, openflags );
   app_class_overload_destroy( (AppClass *) mdbuf, mdbuf_destroy );
   return mdbuf;
}

/** \brief Constructor for the MDbuf object. */

void mdbuf_construct( MDbuf *mdbuf, const char *filename, int openflags )
{
   mdbuf->addr = mu_mmap_open(filename, openflags, &mdbuf->size, 0);
   if (mdbuf->addr == MAP_FAILED ){
      msg_error("mmap failed");
      mdbuf->status = -1;
      return;
   }
   dbuf_construct( (DBuf *) mdbuf, mdbuf->size, mdbuf->addr );
}

/** \brief Destructor for the MDbuf object. */

void mdbuf_destroy(void *mdbuf)
{
   MDbuf *this = (MDbuf *) mdbuf;

   if (mdbuf == NULL) {
      return;
   }
   mu_mmap_close(this->addr, this->size);
   
   dbuf_destroy( mdbuf );
}
