#ifndef DISPLAY_H
#define DISPLAY_H

/*
 * display.h - display functions : date time temp.
 * 
 * include LICENSE
 */

#include <appclass.h>

typedef struct _VfddDisplay VfddDisplay;

enum _DisplayVfddInfo {
   DIS_TIME,
   DIS_DATE,
   DIS_TEMP,
};

struct _VfddDisplay {
   AppClass parent;
   AppClass *xvf;               /* caller object */
   char *name;                  /* object name */
   char *sysfile;               /* /sys file that give the info */
   char *format;                /* object display format */
   int order;                   /* 0 time, 1 date, 2 temp,... */
};

/*
 * prototypes
 */
VfddDisplay *display_new( AppClass *xnode, AppClass * xvf );
void display_construct( VfddDisplay *dis, AppClass *xnode, AppClass * xvf );
void display_destroy(void *dis);

int display_iter_update_cb(AppClass *xdis, void *user_data );

#endif /* DISPLAY_H */
