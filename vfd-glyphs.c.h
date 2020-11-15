/*
 * vfd-glyphs.c.h - Glyph bitmap definitions
 */


/*
 * Platform-independent glyph images
 *     a
 *    ---
 * f |   | b
 *    -g-
 * e |   | c
 *    ---
 *     d
 */
#define a 0x08
#define b 0x10
#define c 0x20
#define d 0x01
#define e 0x02
#define f 0x04
#define g 0x40

struct vfd_glyph_t {
   char code;
   uint8_t image;
};


struct vfd_glyph_t vfd_glyphs [] = {
   { '0', a|b|c|d|e|f },
   { '1', b|c },
   { '2', a|b|g|e|d },
   { '3', a|b|g|c|d },
   { '4', f|g|b|c },
   { '5', a|f|g|c|d },
   { '6', a|f|e|d|c|g },
   { '7', a|b|c },
   { '8', a|b|c|d|e|f|g },
   { '9', g|f|a|b|c|d },
   { ' ', 0 },
   { '~', a },
   { '-', g },
   { '_', d },
   { '`', f },
   { '\'', b },
   { '"', b|f },
   { '/', e|g|b },
   { '\\', f|g|c },
   { '*', a|b|f|g },	// degree sign
   { 'A', e|f|a|b|c|g },
   { 'B', a|b|c|d|e|f|g },
   { 'C', a|f|e|d },
   { 'D', a|b|c|d|e|f },
   { 'E', a|f|g|e|d },
   { 'F', e|f|g|f|a },
   { 'G', a|f|e|d|c|g },
   { 'H', f|b|e|c|g },
   { 'I', b|c },
   { 'J', b|c|d },
   { 'K', f|e|g|b|c },	// bad
   { 'L', f|e|d },
   { 'M', f|e|a|b|c },	// bad
   { 'N', f|e|a|b|c },
   { 'O', a|b|c|d|e|f },
   { 'P', e|f|a|b|g },
   { 'Q', e|f|a|b|c|d },	// bad
   { 'R', e|f|a|b|g|c },	// bad
   { 'S', a|f|g|c|d },
   { 'T', f|e|a },		// bad
   { 'U', f|e|d|c|b },
   { 'V', f|e|d|c|b },	// bad
   { 'W', f|e|d|c|b },	// bad
   { 'X', f|g|c|b|e },
   { 'Y', f|g|b|c|d },
   { 'Z', a|b|g|e|d },
   { 'a', a|b|c|d|e|g },
   { 'b', f|g|c|d|e },
   { 'c', g|e|d },
   { 'd', g|e|d|c|b },
   { 'e', d|e|f|a|b|g },
   { 'f', e|f|a|g },
   { 'g', f|a|b|g|c|d },
   { 'h', f|g|c|e },
   { 'i', c },
   { 'j', c|d },
   { 'k', f|g|c|e|b },	// bad
   { 'l', f|e },
   { 'm', e|g|c },		// bad
   { 'n', e|g|c },
   { 'o', e|g|c|d },
   { 'p', e|f|a|b|g },
   { 'q', g|f|a|b|c },
   { 'r', e|g },
   { 's', a|f|g|c|d },
   { 't', f|g|e|d },
   { 'u', e|d|c },
   { 'v', e|d|c },		// bad
   { 'w', e|d|c },		// bad
   { 'x', f|g|c|b|e },
   { 'y', f|g|b|c|d },
   { 'z', a|b|g|e|d },
   { 0, 0 }
};
