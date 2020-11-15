/*
 * dotled.c - set or clear a bit corresponding to a led in memory.
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <dotled.h>
#include <jsonroot.h>
#include <fileutil.h>
#include <stutil.h>

typedef struct _TestDotval TestDotval;

struct _TestDotval {
   const char *name;
   App_Run_FP func;
};

/*
 * local prototypes
 */
int dotled_test_net(AppClass *data, void *user_data );
int dotled_test_hdmi(AppClass *data, void *user_data );
int dotled_test_colon(AppClass *data, void *user_data );
int dotled_test_usb(AppClass *data, void *user_data );
int dotled_test_bluetooth(AppClass *data, void *user_data );
int dotled_test_alarm(AppClass *data, void *user_data );
/* */

/*
 *** \brief Allocates memory for a new DotLed object.
 *  node : configuration node
 *  target: address of ram to modify
 */

DotLed *dotled_new( AppClass *xnode, uint16_t * target )
{
   DotLed *led;

   led =  app_new0(DotLed, 1);
   dotled_construct( led, xnode, target );
   app_class_overload_destroy( (AppClass *) led, dotled_destroy );
   return led;
}

/** \brief Constructor for the DotLed object. */

void dotled_construct( DotLed *led, AppClass *xnode,  uint16_t * target )
{
   char *name;
   app_class_construct( (AppClass *) led );

   JsonNode *node = (JsonNode *) xnode;
   led->target = target;
   led->name = app_strdup(node->keyname);

   JsonNode *n = json_root_get_item_string(node, "sysfile",  &name );
   if ( n ){
      led->sysfile = app_strdup(name);
   }
   n = json_root_get_item_string(node, "driver",  &name );
   if ( n ){
      dotled_set_test_func(led, name );
   }
   n = json_root_get_item_int(node, "bit",  &led->bit );
   if ( ! n ){
      msg_error( "dotled bit not defined" );
   }
}

/** \brief Destructor for the DotLed object. */

void dotled_destroy(void *led)
{
   DotLed *this = (DotLed *) led;

   if (led == NULL) {
      return;
   }
   app_free(this->name);
   app_free(this->sysfile);

   app_class_destroy( led );
}

void dotled_set_cb_func(DotLed *led, App_Run_FP func, void *user_data )
{
   led->cb_func = func;
   led->cb_app = user_data;
}

int dotled_name_str_cmp(AppClass *d1, AppClass *d2 )
{
   DotLed *led = (DotLed *) d1 ;
   char *name = (char *) d2 ;
   return app_strcmp( led->name, name );
}

int dotled_iter_update(AppClass *data, void *user_data )
{
   DotLed *led = (DotLed *) data;
   char tmpbuf[256];
   FILE *fd = NULL;
   
   if ( led->sysfile ){
      if ( ! file_exists( led->sysfile ) ){
	 return 0;
      }
      fd = fopen( led->sysfile, "r");
      if ( ! fd ) {
	 msg_error("Failed to open file '%s' - %s", led->sysfile, strerror(errno) );
	 return 0;
      }
      led->tmplen = fread( tmpbuf, 1, sizeof(tmpbuf), fd);
      fclose (fd);
      led->tmpbuf = tmpbuf;
   }
   if ( led->test_func ) {
      int val = led->test_func(data, user_data);
      if ( val ){
	 *led->target |= (1 << led->bit);
      }
   }
   return 0;
}


int dotled_test_net(AppClass *data, void *user_data )
{
   int ret = 0;
   DotLed *led = (DotLed *) data;
   int val = strtoul( led->tmpbuf, NULL, 16 );
   if ( val & 1 ){
      ret = 1;
   }
   
   return 1;
}

int dotled_test_hdmi(AppClass *data, void *user_data )
{
   int ret = 0;
   DotLed *led = (DotLed *) data;
   char *msg = "connected";

   if ( app_strncmp( led->tmpbuf, msg, strlen(msg)) == 0 ) {
      ret = 1;
   }
   return 0;
}

int dotled_test_usb(AppClass *data, void *user_data )
{
   int ret = 0;
   DotLed *led = (DotLed *) data;
   int i;
   char *sn = led->tmpbuf;
   char *tok;

   for ( i = 0 ; i < 11 ; i++ ){
      tok = stu_token_next(&sn, " ", " ");
   }
   int val = strtoul( tok, NULL, 10 );
   if ( val > 0 ){
      ret = 1;
   }
   return 0;
}

int dotled_test_alarm(AppClass *data, void *user_data )
{
   int ret = 0;
   DotLed *led = (DotLed *) data;

   if ( *led->tmpbuf == '1' ){
      ret = 1;
   }
   return ret;
}

int dotled_test_colon(AppClass *data, void *user_data )
{
   int ret = 0;
   DotLed *led = (DotLed *) data;

   if ( led->cb_func ) {
      ret = led->cb_func ( led->cb_app, data );
   }
   return ret;
}

static TestDotval test_fun_tbl[] = {
   { "net", dotled_test_net            },
   { "hdmi", 0            },
   { "colon", dotled_test_colon          },
   { "bluetooth", 0  },
   { "usb", 0              },
   { "alarm", 0          },
   { NULL, NULL                          },
};

void dotled_set_test_func(DotLed *led, char *name)
{
   TestDotval *fp = test_fun_tbl;

   while ( fp->name ) {
      if ( app_strcmp( fp->name, name ) == 0 ){
         led->test_func = fp->func;
         return;
      }
      fp++;
   }
   msg_warning( "driver for  '%s' not found\n", name );
}
