#ifndef TIMERMS_H
#define TIMERMS_H

/*
 * timerms.h - timer run milli seconds interface
 * 
 * include LICENSE
 */
#include <sys/time.h>

#include <appclass.h>

typedef int (*Timer_Timeout_FP)( AppClass *data, AppClass *user_data );

typedef struct _Timer Timer;

struct _Timer {
   AppClass parent;
   AppClass *caller;
   AppClass *data;
   Timer_Timeout_FP func;  /* function to be called on timeout */
   struct timeval when;
   struct timeval millisecs;           /* timer duration */
};

/*
 * prototypes
 */
Timer *timer_new( AppClass *caller, int milisecs, Timer_Timeout_FP func, AppClass *data);
void timer_construct( Timer *timer, AppClass *caller, int milisecs,
		      Timer_Timeout_FP func, AppClass *data );
void timer_destroy(void *timer);

void timer_update( Timer *timer, int seconds );
void timer_modify( Timer *timer, int seconds, Timer_Timeout_FP func );

int timer_iter_timer_func( AppClass *data, void *user_data );

#endif /* TIMERMS_H */
