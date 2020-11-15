/*
 * 7-segment common ANODE display glyph support
 * Copyright (c) 2017 Andrew Zabolotny <zapparello@ya.ru>
 *
 * The way how the display driver IC is connected to the real
 * 7-segment LED display may vary from platform to platform.
 * This is a collection of helpers which provides glyph drawing
 * support in a more or less platform-independent way.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include "vfd-priv.h"

struct vfd_glyph_render_t {
	const u8 *cellno;
	const u8 *cellbit;
	u8 bitmap ['~' - ' ' + 1];
};

/* Convert a string of 8-bit characters into a string of 16-bit
 * display bitmasks */
static void vfd_display_to_raw_ca (struct vfd_t *vfd, u8 *display, u16 *raw)
{
	unsigned i, j;
	struct vfd_glyph_render_t *g = (struct vfd_glyph_render_t *)vfd->glyph_render_data;

	memset (raw, 0, sizeof (vfd->raw_display));

	for (i = 0; i < vfd->display_len; i++) {
		u16 bit;
		u8 bitmap, ch = display [i] - ' ';
		if (ch >= ARRAY_SIZE (g->bitmap))
			ch = 0;
		bitmap = g->bitmap [ch];

		// stuff bits a-g to respective slots in respective display cells...
		bit = (1U << g->cellbit [i]);
		for (j = 0; j < 7; j++)
			if (bitmap & (1 << j))
				raw [g->cellno [j]] |= bit;
	}
}

void vfd_init_glyphs_ca (struct vfd_t *vfd, const u8 *cellno, const u8 *cellbit)
{
	unsigned i;
	u8 code;
	struct vfd_glyph_render_t *g = kzalloc (sizeof (struct vfd_glyph_render_t), GFP_KERNEL);
	vfd->display_to_raw = vfd_display_to_raw_ca;
	vfd->glyph_render_data = g;
	g->cellno = cellno;
	g->cellbit = cellbit;

	for (i = 0; (code = vfd_glyphs [i].code) != 0; i++) {
		code -= ' ';
		if (code >= ARRAY_SIZE (g->bitmap))
			continue;

		g->bitmap [code] = vfd_glyphs [i].image;
	}
}
