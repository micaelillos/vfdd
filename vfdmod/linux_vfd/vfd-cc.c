/*
 * 7-segment common CATHODE display glyph support
 * Copyright (c) 2017 Andrew Zabolotny <zapparello@ya.ru>
 *
 * The way how the display driver IC is connected to the real
 * 7-segment LED display may vary from platform to platform.
 * This is a collection of helpers which provides glyph drawing
 * support in a more or less platform-independent way.
 *
 * This file provides support for the common-cathode displays.
 * In this case every 16-bit word of the on-chip display RAM
 * encodes a single glyph.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include "vfd-priv.h"

struct vfd_glyph_render_t {
   u16 cellcode ['~' - ' ' + 1];
};

/* Convert a string of 8-bit characters into a string of 16-bit
 * display bitmasks */
static void vfd_display_to_raw_cc (struct vfd_t *vfd, u8 *display, u16 *raw)
{
   int i;
   struct vfd_glyph_render_t *g = (struct vfd_glyph_render_t *) vfd->glyph_render_data;

   for (i = 0; i < vfd->display_len; i++) {
      u8 ch;
      if ( vfd->grid_len ){
	 ch = display [vfd->grid_num[i]] - ' ';
      } else {
	 ch = display [i] - ' ';
      }
      if (ch >= ARRAY_SIZE (g->cellcode)){
	 ch = 0;
      }
      raw [i] = g->cellcode [ch];
   }
   for (  ; i < ARRAY_SIZE(vfd->raw_display); i++) {
      raw [i] = 0;
   }
}

/* Initialize glyph images for current platform from the
 * platform-independent representation */
void vfd_init_glyphs_cc (struct vfd_t *vfd, const u8 *segno)
{
   int i, j;
   u8 code;
   struct vfd_glyph_render_t *g = kzalloc (sizeof (struct vfd_glyph_render_t), GFP_KERNEL);

   vfd->glyph_render_data = g;
   vfd->display_to_raw = vfd_display_to_raw_cc;

   if ( vfd->segment_len ){
      segno = vfd->segment_no;
   }
   for (i = 0; (code = vfd_glyphs [i].code) != 0; i++) {
      u8 src;
      u16 dst;

      code -= ' ';
      if (code >= ARRAY_SIZE (g->cellcode)){
	 continue;
      }
      src = vfd_glyphs [i].image;
      dst = 0;
      // for every segment a,b,c,d,e,f,g (7 total)...
      for (j = 0; j < 7; j++){
	 if (src & (1 << j)){
	    dst |= (1U << segno [j]);
	 }
      }
      g->cellcode [code] = dst;
   }
}
