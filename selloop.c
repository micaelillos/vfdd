/*
 * loop.c - the select main loop for servers functions
 * 
 * include LICENSE
 */
#define _GNU_SOURCE  /* sys/time.h */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#include <selloop.h>
#include <timer.h>
#include <msglog.h>

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

/*
 * local prototypes
 */
int loop_iter_channel_func( AppClass *channel, void *user_data );

/*
 *** \brief Allocates memory for a new Loop object.
 */

Loop *loop_new( int max_cnx, void *user_data )
{
   Loop *loop;

   loop =  app_new0(Loop, 1);
   loop_construct( loop, max_cnx, user_data);
   app_class_overload_destroy( (AppClass *) loop, loop_destroy );
   return loop;
}

/** \brief Constructor for the Loop object. */

void loop_construct( Loop *loop, int max_cnx, void *user_data )
{
   app_class_construct( (AppClass *) loop );
 
   loop->max_width = max_cnx;
   loop->user_data = user_data;
   /* default values */
   loop->loop_timeout.tv_sec = 0;
   loop->loop_timeout.tv_usec = 250000;
   loop->timer_interval.tv_sec = 0;
   loop->timer_interval.tv_usec = 500000;
}

/** \brief Destructor for the Loop object. */

void loop_destroy(void *loop)
{
   Loop *this = (Loop *) loop;

   if (loop == NULL) {
      return;
   }
   if ( this->channels ){
      dlist_delete_all( this->channels );
   }
   if ( this->timers ){
      dlist_delete_all( this->timers );
   }
   app_class_destroy( loop );
}

void loop_set_loop_timeout(Loop *loop, int ms )
{
   loop->loop_timeout.tv_sec = ms  / 1000;
   loop->loop_timeout.tv_usec =  (ms % 1000) * 1000;
}

void loop_set_timer_interval(Loop *loop, int ms )
{
   loop->timer_interval.tv_sec = ms / 1000;
   loop->timer_interval.tv_usec = (ms % 1000) * 1000;
}

void loop_set_fds(Loop *loop, int s )
{
   loop->width = MAX(loop->width, s + 1);
   if ( loop->width >= loop->max_width ){
      msg_error( "number of fds exhausted %d >= %d", loop->width,
                 loop->max_width ) ;
   }
   FD_SET( s, &loop->fdsmsk ) ;
}

void loop_clr_fds(Loop *loop, int s )
{
    FD_CLR( s, &loop->fdsmsk ) ;
}

void loop_channel_add(Loop *loop, Channel *cha )
{
   loop->channels = dlist_add_tail ( loop->channels, (AppClass *) cha );
   loop_set_fds(loop, cha->fd );
}

/*
 * remove the desc only
 * the list will be updated in dlist_iterator with appropriate ret code
 */
void loop_channel_remove_fd(Loop *loop, Channel *cha )
{
   if ( ! cha || cha->fd < 0 ) {
      return;
   }
   FD_CLR( cha->fd, &loop->fdsmsk ) ;
   cha->fd = -1; /* mark fd as removed */
}

void loop_channel_remove(Loop *loop, Channel *cha )
{
   if ( ! cha ) {
      return;
   }
   loop_channel_remove_fd(loop, cha );
   loop->channels = dlist_delete ( loop->channels, (AppClass *) cha, dlist_iterator_cmp );
}

void loop_timer_add(Loop *loop, Timer *timer )
{
   loop->timers = dlist_add_tail ( loop->timers, (AppClass *) timer );
}

void loop_timer_remove(Loop *loop, Timer *timer )
{
   loop->timers = dlist_delete ( loop->timers, (AppClass *) timer, dlist_iterator_cmp );
}

int loop_iter_channel_func( AppClass *channel, void *user_data )
{
   Channel *cha = (Channel *) channel ;
   Loop *loop = (Loop *) user_data ;
   int ret = 0;
  
   if ( FD_ISSET( cha->fd, &loop->fdscopy) ){
      /* client ready for write to , loop ready for read from */
      ret = cha->rdfunc( cha, user_data) ;
   }
      
   return ret;  /* return 0 to avoid deleting the channel node */
}

void loop_quit(Loop *loop )
{
   loop->endRequest = 1;
}

void loop_run(Loop *loop )
{
   int do_timers = 0;
   int j;
   int ret = 0;
   struct timeval sel_timeout ;
   struct timeval run_timers ;
   struct timeval time_now ;

   timerclear(&run_timers);
   
   for ( ; ! loop->endRequest ; ) {
      msg_infol( 6, "Main loop - %d active connections\n", ret) ;

      loop->fdscopy = loop->fdsmsk;

      sel_timeout.tv_sec = loop->loop_timeout.tv_sec;
      sel_timeout.tv_usec = loop->loop_timeout.tv_usec ;
      
      j = select(loop->width, &loop->fdscopy, NULL, NULL, &sel_timeout );

      if ( j <= 0 ){
         if ( j < 0 ) {
            msg_error( "select readfds - err %s", strerror(errno)) ;
            continue ;
         }
         /* j == 0 , timeout */
         gettimeofday(&time_now, NULL );
         if ( timercmp(&time_now, &run_timers, >= ) ){
             do_timers = 1;
         }
      }
      if ( do_timers ){
         do_timers = 0;
//         fprintf(stderr, "doing timers\n");
         dlist_iterator( loop->timers, timer_iter_timer_func, loop );
         timeradd(&time_now, &loop->timer_interval, &run_timers);
      }
      if ( j == 0 ) {
         continue ;
      }
      dlist_iterator( loop->channels, loop_iter_channel_func, loop );

       
   }
}
