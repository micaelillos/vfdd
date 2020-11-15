#ifndef DOTLED_H
#define DOTLED_H

/*
 * dotled.h - dotled protocol interface
 * 
 * include LICENSE
 */
#include <stdint.h>

#include <appclass.h>

typedef struct _DotLed DotLed;

struct _DotLed {
   AppClass parent;
   App_Run_FP test_func;         /* function to get the dot value */
   App_Run_FP cb_func;           /* function callback to get the dot value */
   void * cb_app;                /* callback app to get the dot value */
   char *name;                   /* dotled name */ 
   char *sysfile;                /* /sys file that give the info */ 
   char *tmpbuf;                 /* pointer to a temp buffer */ 
   int tmplen;                   /* len of data in tmpbuf */
   uint16_t * target;            /* ram memory address for dot */
   int bit;                      /* bit number in target */
};

/*
 * prototypes
 */
DotLed *dotled_new( AppClass *node, uint16_t * target );
void dotled_construct( DotLed *led, AppClass *node, uint16_t * target );
void dotled_destroy(void *led);

int dotled_name_str_cmp(AppClass *d1, AppClass *d2 );
void dotled_set_cb_func(DotLed *led, App_Run_FP func, void *user_data );
int dotled_iter_update(AppClass *data, void *user_data );
void dotled_set_test_func(DotLed *led, char *name);

#endif /* DOTLED_H */
