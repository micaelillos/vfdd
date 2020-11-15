/*
 * VFD backend for PT6964, SM1628, TM1623, FD268 LED driver chips.
 * Copyright (c) 2017 Andrew Zabolotny <zapparello@ya.ru>
 */

#ifndef __PT6964_H__
#define	__PT6964_H__

#include "vfd-priv.h"

// FD620 is somewhat compatible, but has some differences
#if defined COMPAT_FD620

// 4 digits, 8 segments
#define DISPLAY_MODE_4D8S		0x00
// 5 digits, 7 segments
#define DISPLAY_MODE_5D7S		0x01

#else

// 4 digits, 13 segments
#define DISPLAY_MODE_4D13S		0x00
// 5 digits, 12 segments
#define DISPLAY_MODE_5D12S		0x01
// 6 digits, 11 segments
#define DISPLAY_MODE_6D11S		0x02
// 7 digits, 10 segments
#define DISPLAY_MODE_7D10S		0x03

#endif

// Set display mode, 'mode' is one of DISPLAY_MODE_XXX constants
#define CMD_DISPLAY_MODE(mode)		(0x00 | (mode))
// Set up data input/output mode: 'inc' for auto-increment, 'read' for reading (otherwise write)
#define CMD_DATA_SETTING(inc, read)	(0x40 | ((inc) ? 0 : 4) | ((read) ? 2 : 0))
// Set display RAM address to read from/write to
#define CMD_ADDRESS_SET(addr)		(0xc0 | (addr))
// Enable display and set display brightness (0-7)
#define CMD_DISPLAY_CONTROL(en,bri)	(0x80 | ((en) ? 8 : 0) | (bri))

// 8 brighness levels 0..7 (0 is lowest brightness, not off)
#define BRIGHTNESS_MAX			7

#if defined CONFIG_VFD_PT6964_X92

// LED display brightness at startup
#define PLATFORM_BRIGHTNESS		3
// Display mode
#define PLATFORM_DISPLAY_MODE		DISPLAY_MODE_7D10S
// Number of displayed glyphs
#define PLATFORM_DISPLAY_LEN		4

// X92 uses a common-ANODE LED display, so the display RAM
// is organized quite perversely.

// (see vfd7s.c) a|b|c|d|e|f|g -> display grid number
#define PLATFORM_CELLNO			{ 0, 1, 2, 3, 4, 5, 6 }
// Bit number to set, depending on display cell
#define PLATFORM_CELLBIT		{ 7, 6, 5, 4 }

// alternatively, if you have a common-CATHODE display (the normal variant),
// define PLATFORM_SEGNO instead of PLATFORM_GRIDNO & PLATFORM_CELLNO
// (see vfd-glyphs.c) a|b|c|d|e|f|g -> cell bit number (0-15) translation table
//#define PLATFORM_SEGNO			{ 0, 1, 2, 3, 4, 5, 6 }

#elif defined CONFIG_VFD_PT6964_T95U

// LED display brightness at startup
#define PLATFORM_BRIGHTNESS		3
// Display mode
#define PLATFORM_DISPLAY_MODE		DISPLAY_MODE_5D7S
// Number of displayed glyphs
#define PLATFORM_DISPLAY_LEN		4
// T95U uses a common-CATHODE LED display
#define PLATFORM_SEGNO			{ 0, 1, 2, 3, 4, 5, 6 }

#else
#error "No display connection scheme defined!"
#endif

#endif // __PT6964_H__
