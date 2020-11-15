#ifndef VFDD_H
#define VFDD_H

/*
 * vfdd.h - vfdd protocol interface
 * 
 * include LICENSE
 */

#include <stdint.h>

#include <loop.h>
#include <timerms.h>
#include <jsonroot.h>

typedef struct _Vfdd Vfdd;

struct _Vfdd {
   AppClass parent;
   Loop *loop;             /* the main loop */
   Timer *timer;           /* the main timer */
   uint16_t *display_raw;  /* data to be transmitted to display */
   DList *dots;            /* list of dotled object */
   DList *listCbs;         /* list of vfdd funcs callback */
   uint8_t *render_tbl;    /* table to convert an ascii symbol to a display image */
   uint8_t *digit_map;     /* table of digit address */
   char *conffile;         /* pointer to configuration filename  */
   char *device;           /* vfd device name */
   char *overlay;          /* name of the overlay sys file */
   char *display_str;      /* sting to be displayed */
   unsigned long timer_count;  /* count timer interrupt every 500 ms */
   time_t curtime;         /* current time */
   int digit_num;          /* number of digit in display */
   int grid_num;           /* number of ram address in display */
   int dotled_map;         /* ram address for dotleds */
   int brightness;         /* default led brightness (0 to 100%) */
   int interval;           /* time to wait milisecs before next callback */
   int nocolon;            /* set to 1 if date, temp is displayed */
   int status;             /* operation status */
   struct tm *vftm;        /* structure tm */
   char *word; /*the word to print*/
};

/*
 * prototypes
 */
Vfdd *vfdd_new( char *conffile );
void vfdd_construct( Vfdd *vf, char *conffile  );
void vfdd_destroy(void *vf);

int vfdd_get_colon( AppClass *xvf, void *user_data );
int vfdd_read_conf(Vfdd *vf);
int vfdd_timer_cb(AppClass *xvf, AppClass *user_data );
void vfdd_init_render_tbl ( Vfdd *vf, const uint8_t *segno);
int vfdd_iter_dotled_funcs( AppClass *data, void *user_data );
int vfdd_iter_vfdd_funcs( AppClass *data, void *user_data );
void vfdd_update_display (Vfdd *vf );
uint8_t getMyGlyph(char letter);
void vfdd_overlay_store (Vfdd *vf );

#endif /* VFDD_H */
