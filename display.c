/*
 * display.c - display functions : date time temp.
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <display.h>
#include <strmem.h>
#include <jsonroot.h>
#include <fileutil.h>
#include <vfdd.h>

/*
 * local prototypes
 */
int display_get_temp(VfddDisplay *dis, Vfdd *vf );
/* */

/*
 *** \brief Allocates memory for a new VfddDisplay object.
 */

VfddDisplay *display_new( AppClass *xnode, AppClass * xvf )
{
   VfddDisplay *dis;

   dis =  app_new0(VfddDisplay, 1);
   display_construct( dis, xnode, xvf );
   app_class_overload_destroy( (AppClass *) dis, display_destroy );
   return dis;
}

/** \brief Constructor for the VfddDisplay object. */

void display_construct( VfddDisplay *dis, AppClass *xnode, AppClass * xvf  )
{
   JsonNode *node = (JsonNode *) xnode;
   char *name;
   
   app_class_construct( (AppClass *) dis );
   dis->xvf = xvf;

   dis->name = app_strdup(node->keyname);
   JsonNode *n = json_root_get_item_string(node, "sysfile",  &name );
   if ( n ){
      dis->sysfile = app_strdup(name);
   }
   n = json_root_get_item_string(node, "format",  &name );
   if ( n ){
      dis->format = app_strdup(name);
   }
   json_root_get_item_int(node, "order",  &dis->order );

}

/** \brief Destructor for the VfddDisplay object. */

void display_destroy(void *dis)
{
   VfddDisplay *this = (VfddDisplay *) dis;

   if (dis == NULL) {
      return;
   }
   app_free(this->name);
   app_free(this->sysfile);
   app_free(this->format);

   app_class_destroy( dis );
}

int display_get_temp(VfddDisplay *dis, Vfdd *vf )
{
   char tmpbuf[256];
   FILE *fd = NULL;
   int val = 0;
   
   if ( dis->sysfile ){
      if ( ! file_exists( dis->sysfile ) ){
	 return 0;
      }
      fd = fopen( dis->sysfile, "r");
      if ( ! fd ) {
	 msg_error("Failed to open file '%s' - %s", dis->sysfile, strerror(errno) );
	 return 0;
      }
      fread( tmpbuf, 1, sizeof(tmpbuf), fd);
      fclose (fd);
      val = strtoul( tmpbuf, NULL, 10 );
   }
   return val;
}

/* call back */
int display_iter_update_cb(AppClass *xdis, void *user_data )
{
   char buff[32];
   
   VfddDisplay *dis = (VfddDisplay *) xdis;
   Vfdd *vf = (Vfdd *) dis->xvf;

   switch (dis->order) {
    case DIS_DATE:
      if ( vf->vftm->tm_sec < 5 || vf->vftm->tm_sec >= 10 ){
	 return 0;
      }
    case DIS_TIME:
      strftime( buff, sizeof(buff), dis->format, vf->vftm);
      break;
    case DIS_TEMP:
      if ( vf->vftm->tm_sec < 15 || vf->vftm->tm_sec >= 20 ){
	 return 0;
      }
      int val = display_get_temp( dis, vf );
      snprintf( buff, sizeof(buff), dis->format, val / 1000 );
      break;
   }
   app_dup_str(&vf->display_str, buff );
  // vf->nocolon = dis->order;
   return 0;
}
