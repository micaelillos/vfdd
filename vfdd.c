/*
 * vfdd.c - vfdd interface functions
 * 
 * include LICENSE
 */
#define _GNU_SOURCE  /* time.h */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <vfdd.h>
#include <mdbuf.h>
#include <dotled.h>
#include <display.h>
#include <duprintf.h>


#define RENDER_TBL_SIZ (0x7E - 0x20)
#include <vfd-glyphs.c.h>

/*
 *** \brief Allocates memory for a new Vfdd object.
 */

Vfdd *vfdd_new( char *conffile )
{
   Vfdd *vf;

   vf =  app_new0(Vfdd, 1);
   vfdd_construct( vf, conffile);
   app_class_overload_destroy( (AppClass *) vf, vfdd_destroy );
   return vf;
}

/** \brief Constructor for the Vfdd object. */

void vfdd_construct( Vfdd *vf, char *conffile )
{
   app_class_construct( (AppClass *) vf );

   vf->conffile = conffile;
   if ( vfdd_read_conf (vf ) < 0 ){
      msg_fatal("Error reading configuration file '%s'", vf->conffile);
   }
   vf->interval = 500;
   vf->vftm = app_new0(struct tm, 1 );

   vf->loop = loop_new( LOOP_NB_CHANNEL, vf );
   /* timer must exist for timer_update */
   vf->timer = timer_new( (AppClass *) vf, vf->interval,
                                vfdd_timer_cb, NULL );

   /* the -ud->interval will run the callback  1 second later */
   loop_timer_add(vf->loop, vf->timer );
   
   DotLed *led = (DotLed *) dlist_lookup( vf->dots, (AppClass *) "colon",
					  dotled_name_str_cmp );
   if ( led ) {
      dotled_set_cb_func( led, vfdd_get_colon, vf );
   }
}

/** \brief Destructor for the Vfdd object. */

void vfdd_destroy(void *vf)
{
   Vfdd *this = (Vfdd *) vf;

   if (vf == NULL) {
      return;
   }
   loop_destroy( this->loop ); /* this should remove the timer */
   dlist_delete_all( this->dots );
   dlist_delete_all( this->listCbs );
   app_free(this->device);
   app_free(this->overlay);
   app_free(this->render_tbl);
   app_free(this->display_raw);
   app_free(this->digit_map);
   app_free(this->vftm);
   app_free(this->display_str);
   
   app_class_destroy( vf );
}

int vfdd_iter_dotled_funcs( AppClass *data, void *user_data )
{
   JsonNode *node = (JsonNode *) data;
   Vfdd *vf = (Vfdd *) user_data;
   int result;
   
   JsonNode *n = json_root_get_item_bool(node, "enable",  &result );
   if ( n == NULL || result == 0 ){
      return 0;
   }

   DotLed *led = dotled_new( (AppClass *) node, &vf->display_raw[vf->dotled_map] );
   vf->dots = dlist_add_tail(vf->dots, (AppClass *) led );

   msg_dbg( "dotled '%s' %d \n", node->keyname, vf->dotled_map );
   return 0;
}

int vfdd_iter_vfdd_funcs( AppClass *data, void *user_data )
{
   JsonNode *node = (JsonNode *) data;
   Vfdd *vf = (Vfdd *) user_data;
   int result;
   
   JsonNode *n = json_root_get_item_bool(node, "enable",  &result );
   if ( n == NULL || result == 0 ){
      return 0;
   }

   VfddDisplay *dis = display_new( (AppClass *) node, (AppClass *) vf );
   vf->listCbs = dlist_add_tail(vf->listCbs, (AppClass *) dis );

   msg_dbg( "vfdd_funcd '%s'\n", node->keyname );
   return 0;
}

int vfdd_read_conf(Vfdd *vf)
{
   char *end;
   JsonRootNode *root = NULL;
   JsonNode *node;
   char *name;
   int count;
   int i;
   int ret = -1;

   MDbuf *mdbuf = mdbuf_new( vf->conffile, O_RDONLY );
   if ( mdbuf->status ){
      vf->status = mdbuf->status;
      goto enderr;
   }
   root = json_root_new((DBuf *) mdbuf, 0);
   end = json_root_parse( root, 0 );

   if ( ! end ) {
      json_root_print_error(root);
      goto enderr;
   }

   if ( msg_get_dbg_msk() & DBG_1 ){
      json_root_print(root, 1);
      printf("%s\n", root->dbuf->s);
   }
   
   node = json_node_find_node((JsonNode *) root, "display" );
   if ( node ) {

      json_root_get_item_string(node, "device", &name );
      if ( name ){
         vf->device = app_strdup(name);
      }
      json_root_get_item_string(node, "sysfile", &name );
      if ( name ){
         vf->overlay = app_strdup_printf("%s/%s", vf->device, name);
      }

      json_root_get_item_int( node, "grid_num", &vf->grid_num );
      vf->display_raw = app_new0(uint16_t, vf->grid_num );

      JsonNode *array = json_root_get_item_nelem( node, "segment_no", &count);
      if ( array ){
	 /* segno : bit num for segment a b c ... */
	 uint8_t *segno = app_new0( uint8_t, count );
	 for ( i = 0 ; i < count ; i++ ){
	    JsonNode *item = json_node_get_nth_child( array, i );
	    if ( ! item ){
	       continue;
	    }
	    int val;
	    json_node_get_val_int(item, &val );
	    segno[i] = val;
   	 }
	 vfdd_init_render_tbl ( vf, segno);
	 app_free (segno);
      }

      array = json_root_get_item_nelem( node, "digit_map", &count);
      if ( array ){
	 /* segno : bit num for segment a b c ... */
	 vf->digit_num = count ;
	 vf->digit_map = app_new0( uint8_t, count );
	 for ( i = 0 ; i < count ; i++ ){
	    JsonNode *item = json_node_get_nth_child( array, i );
	    if ( ! item ){
	       continue;
	    }
	    int val;
	    json_node_get_val_int(item, &val );
	    vf->digit_map[i] = val;
   	 }
      }

      json_root_get_item_int( node, "brightness", &vf->brightness );

      JsonNode *object = json_node_find_node( node, "functions");
      if ( object ){
	 dlist_iterator(object->child, vfdd_iter_vfdd_funcs, vf );
      }
   }
   
   node = json_node_find_node((JsonNode *) root, "dotleds" );
   if ( node ) {

      json_root_get_item_int( node, "grid_map", &vf->dotled_map );

      JsonNode *object = json_node_find_node( node, "functions" );
      if ( object ){
	 dlist_iterator(object->child, vfdd_iter_dotled_funcs, vf );
      }

   }
   ret = 0;

enderr:
   mdbuf_destroy(mdbuf);
   json_root_destroy(root);
   return ret;
}

int vfdd_get_colon( AppClass *xvf, void *user_data )
{
   Vfdd *vf = (Vfdd  *) xvf;
   return vf->nocolon;
   
}

uint8_t getMyGlyph(char letter) {
for(int i =0; i< sizeof(vfd_glyphs); i++){
   if(letter == vfd_glyphs[i].code)
   return vfd_glyphs[i].image;
}
return vfd_glyphs[0].image;
}
/*
 * Convert a string of 8-bit characters into a string of 16-bit
 * display bitmasks
 */
void vfdd_update_display (Vfdd *vf )
{
   int i;
   int k;
   char *dstr = vf->display_str;
   msg_dbgl( DBG_2, "converting '%s' colon %d", dstr, !! (vf->display_raw[vf->dotled_map] & 4) );

   for (i = 0; i < vf->digit_num; i++) {
      uint8_t ch = dstr[i] - ' ';
      /* digit_map says : ch[0] go to mem digit_map[0] , ... */
      k = vf->digit_map[i];

      if ( ch >= RENDER_TBL_SIZ ){
         ch = 0;
      }
      msg_dbgl(  DBG_2, "k %d ch 0x%X  render 0x%X", k, ch, vf->render_tbl[ch] );
     
//      vf->display_raw[k] = vf->render_tbl[ch];
      // ! GO 1
       // vf->display_raw[3] = getMyGlyph('d');
       

// ! GO 2
      //   vf->display_raw[3] =  vfd_glyphs[63].image;
      //   vf->display_raw[2] = vfd_glyphs[66].image;
      //   vf->display_raw[1] = vfd_glyphs[59].image;


   }
    printf("Printing %s\n",vf->word);
   // ! done
        vf->display_raw[3] =  getMyGlyph(vf->word[0]);
         vf->display_raw[2] =getMyGlyph(vf->word[1]);
         vf->display_raw[1] = getMyGlyph(vf->word[2]);
        vf->display_raw[0] = getMyGlyph(vf->word[3]);

      
 
}




void vfdd_overlay_store (Vfdd *vf )
{
   int i;

   FILE *fd = fopen( vf->overlay, "w");
   if ( ! fd ){
      msg_error("Failed to open file '%s' - %s", vf->overlay, strerror(errno) );
      return;
   }
   msg_dbg( "writing to '%s'", vf->overlay );
   for (i = 0; i < vf->grid_num; i++) {
      fprintf(fd, "%04X ", vf->display_raw[i] ) ;
   }
   fclose(fd);
}

int vfdd_timer_cb(AppClass *xvf, AppClass *user_data )
{
   Vfdd *vf = (Vfdd  *) xvf;
   vf->timer_count++;

   memset( vf->display_raw, 0, vf->grid_num * 2 );
   time(&vf->curtime);
   localtime_r(&vf->curtime, vf->vftm );
   dlist_iterator(vf->listCbs, display_iter_update_cb, vf );
   dlist_iterator(vf->dots, dotled_iter_update, vf );

   vfdd_update_display ( vf );
   vfdd_overlay_store ( vf );
    exit(1); //! Exit 
   return 0;   
   
}


/*
 * Initialize glyph images for current platform from the
 * platform-independent representation
 */
void vfdd_init_render_tbl ( Vfdd *vf, const uint8_t *segno)
{
  
}
