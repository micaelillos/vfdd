#ifndef UTF8_H
#define UTF8_H

/*
 * utf8.h - encode or decode an escaped utf8 code point
 *
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

/*
 * prototypes
 */
int utf8_is_multibyte( unsigned char *str );
char *utf8_code2chars( unsigned int code);
unsigned int utf8_chars2code( unsigned char **str );

#endif /* UTF8_H */
