/*
 * c2hex.c - convert string to hex
 *
 * 
 * include LICENSE
 */

#include <c2hex.h>
/*
 * return hex value of a char 0-9, A-F
 * or return -1
 */

int8_t c_to_hex(int8_t c)
{
   if ( c < 0x30 ) {
      return -1;
   }
   if ( c < 0x3A ) {
      return c - '0';
   }
   c &= ~ 0x20;
   c = c - '7';
   if ( c < 0x0A || c > 0x0F ){
      return -1;
   }
   return c;
}
