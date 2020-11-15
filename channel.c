/*
 * channel.c - data storage for an object, a descriptor socket or fd and
 *             callback func
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <channel.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif


/*
 *** \brief Allocates memory for a new Channel object.
 */

Channel *channel_new( AppClass *cnx, int fd, Ready_Read_FP rdfunc,
                      AppClass *user_data )
{
   Channel *cha;

   cha =  app_new0(Channel, 1);
   channel_construct( cha, cnx, fd, rdfunc, user_data );
   app_class_overload_destroy( (AppClass *) cha, channel_destroy );
   return cha;
}

/** \brief Constructor for the Channel object. */

void channel_construct( Channel *cha, AppClass *cnx, int fd,
                        Ready_Read_FP rdfunc, AppClass *user_data )
{
   app_class_construct( (AppClass *) cha );

   cha->cnx = cnx;
   cha->fd = fd;
   cha->rdfunc = rdfunc;
   cha->user_data = user_data;
}

/** \brief Destructor for the Channel object. */

void channel_destroy(void *cha)
{
//   Channel *this = (Channel *) cha;

   if (cha == NULL) {
      return;
   }

   app_class_destroy( cha );
}
