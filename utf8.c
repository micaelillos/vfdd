/*
 * utf8.c - encode or decode an escaped utf8 code point
 * 
 * include LICENSE
 */
#include <utf8.h>

static const unsigned char firstMark[7] = {
    0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC,
};
static unsigned int utf8_boundary[] = {
   0x80, 0x800, 0x10000, 0x200000,
};

int utf8_is_multibyte( unsigned char *str )
{
   if (*str >= 0x80 ){
      return 1;
   }
   return 0;
}

char *utf8_code2chars( unsigned int code)
{
   static char buf[8];
   int i;
   int len = 0;

   for ( i = 0 ; i < 4 ; i++ ){
      len++;
      if ( code < utf8_boundary[i] ){
         break;
      }
   }
   buf[len] = 0;
   for ( i = len - 1 ; i >= 0 ; i-- ){
      if ( i == 0 ){
         buf[i] = (code | firstMark[len]);
      } else {
         buf[i] = ((code | 0x80) & 0xBF);
         code >>= 6;
      }
   }
   return buf;
}

/*
 * on return set str at next after the multibyte sequence
 */ 
unsigned int utf8_chars2code( unsigned char **str )
{
   unsigned int code = 0;
   unsigned char *p;
   int mask = 0x80;
   int i;
   int len;

   p = *str;
   for ( len = 0 ; len < 7 ; len++ ){
      if ( (*p & mask) == 0 ){
         break;
      }
      mask >>= 1;
   }
   p = *str + len - 1;
   for ( i = len - 1 ; i >= 0 ; i--, p-- ){
      if ( i == 0 ){
         code |= (*p & ~firstMark[len]);
      } else {
         code |= (*p & 0x3f);
         code <<= 6;
      }
   }

   *str += len;
   return code;
}
