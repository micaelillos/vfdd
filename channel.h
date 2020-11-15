#ifndef CHANNEL_H
#define CHANNEL_H

/*
 * channel.c - data storage for an object, a descriptor socket or fd and
 *             callback func
 * 
 * include LICENSE
 */

#include <appclass.h>

/*
 * this info is placed in this file to serve all loop types.
 */
enum _ServerModeInfo {
   LM_CLIENT = 0,
   LM_STANDALONE,
   LM_INETD,
   LM_MODE_MASK  = 3,
   LM_IPV6       = 4,
};

typedef struct _Channel Channel;

typedef int (*Ready_Read_FP)( Channel *cha, AppClass *user_data );

struct _Channel {
   AppClass parent;
   AppClass *cnx;         /* the object */
   Ready_Read_FP rdfunc;  /* func to call when its descriptor is ready to read */
   AppClass *user_data;   /* some other user data */
   AppClass *poll_data;   /* data used by polloop */
   int fd;                /* fd associated with object */
};

/*
 * prototypes
 */
Channel *channel_new( AppClass *cnx, int fd, Ready_Read_FP read, AppClass *user_data );
void channel_construct( Channel *cha, AppClass *cnx, int fd,
                        Ready_Read_FP tdfunc, AppClass *user_data );
void channel_destroy(void *cha);

#endif /* CHANNEL_H */
