/*
 * timerms.c - timerms run milli seconds.
 *             timer.c run seconds
 *          Before using timerms, verify the loop you are using can provide
 *          what you request and modify in consequence with
 *          loop_set_loop_timeout loop_set_timer_interval.
 * 
 * include LICENSE
 */
#define _GNU_SOURCE  /* sys/time.h */
#include <stdio.h>
#include <string.h>

#include <msglog.h>
#include <timer.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 *** \brief Allocates memory for a new Timer object.
 */

Timer *timer_new( AppClass *caller, int millisecs, Timer_Timeout_FP func, AppClass *data )
{
   Timer *timer;

   timer =  app_new0(Timer, 1);
   timer_construct( timer, caller, millisecs, func, data );
   app_class_overload_destroy( (AppClass *) timer, timer_destroy );
   return timer;
}

/** \brief Constructor for the Timer object. */

void timer_construct( Timer *timer, AppClass *caller, int millisecs,
		      Timer_Timeout_FP func, AppClass *data )
{
   if (timer == NULL) {
      msg_fatal( "passed null ptr" );
   }

   app_class_construct( (AppClass *) timer );
   timer->caller = caller;
   timer->func = func;
   timer->data = data;
   timer_update( timer, millisecs);
}

/** \brief Destructor for the Timer object. */

void timer_destroy(void *timer)
{
//   Timer *this = (Timer *) timer;

   if (timer == NULL) {
      return;
   }

   app_class_destroy( timer );
}


/*
 *  if millisecs < 0, timeout will trigger 1 second later
 */
void timer_update( Timer *timer, int millisecs )
{
   int millis = millisecs;
   if ( millisecs < 0) {
      millis = (- millisecs);
   }
   int sec = millis  / 1000;
   int usec = (millis % 1000) * 1000;

   gettimeofday(&timer->when, NULL);
   if ( millisecs < 0) {
      timer->when.tv_sec += 1;
   } else {
      timer->when.tv_sec += sec;
      timer->when.tv_usec += usec;
   }
   timer->millisecs.tv_sec = sec;
   timer->millisecs.tv_usec = usec;
}

void timer_modify( Timer *timer, int millisecs, Timer_Timeout_FP func )
{
   timer_update( timer, millisecs );
   timer->func = func;
}

/*
 *  run function for timers
 *    called by loop iterator
 */
int timer_iter_timer_func( AppClass *data, void *user_data )
{
   Timer *timer = (Timer *) data;
   struct timeval now;
   int ret = 0;
   
   gettimeofday(&now, NULL);
   if ( timercmp(&now, &timer->when, >= ) ){
      ret = timer->func ( timer->caller, timer->data );
      if (ret == 0 ){ /* reload the timer */
	 timeradd(&now, &timer->millisecs, &timer->when );
      }
   }
      
   return ret; /* if 0, don't destroy this node */
}
