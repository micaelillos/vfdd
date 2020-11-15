#ifndef SELLOOP_H
#define SELLOOP_H

/*
 * selloop.h - the select main loop for servers interface
 * 
 * include LICENSE
 */
#include <sys/select.h>

#include <channel.h>
#include <timer.h>
#include <dlist.h>

#define LOOP_NB_CHANNEL 20   /* default max connections */

enum _LoopFdsInfo {
   FDS_READ,
      FDS_WRITE,
      FDS_EXCEPT,
};


typedef struct _Loop Loop;

typedef int (*Loop_Accept_FP)( AppClass *cnx, Loop *loop );

struct _Loop {
   AppClass  parent;
   DList *channels;   /* list of SockCon connected objects to watch */
   DList *timers;     /* list of timer object to run */
   fd_set fdsmsk;     /* table of fds_set 0, read  */
   fd_set fdscopy;    /* copy of fds */
   struct timeval loop_timeout;    /* loop timeout value  */
   struct timeval timer_interval;  /* timer increment  */
   int width;         /* current  number of file descriptors */
   int max_width;     /* max number of file descriptors */
   int endRequest;    /* run the the loop while endRequest is 0 */
   void *user_data;   /* a user data pointer */
};

/*
 * prototypes
 */
Loop *loop_new( int max_cnx, void *user_data );
void loop_construct( Loop *loop, int max_cnx, void *user_data );
void loop_destroy(void *loop);

void loop_set_fds(Loop *loop, int s );
void loop_clr_fds(Loop *loop, int s );
void loop_channel_add(Loop *loop, Channel *cha );
void loop_channel_remove_fd(Loop *loop, Channel *cha );
void loop_channel_remove(Loop *loop, Channel *cha );
void loop_timer_add(Loop *loop, Timer *timer );
void loop_timer_remove(Loop *loop, Timer *timer );

void loop_set_loop_timeout(Loop *loop, int ms );
void loop_set_timer_interval(Loop *loop, int ms );
void loop_run(Loop *loop );
void loop_quit(Loop *loop );
void loop_alarm_handler(int sig);

#endif /* SELLOOP_H */
