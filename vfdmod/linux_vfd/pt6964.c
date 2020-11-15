/*
 * VFD backend for PT6964, SM1628, TM1623, FD268 LED driver chips.
 * Copyright (c) 2017 Andrew Zabolotny <zapparello@ya.ru>
 */

#include <linux/kernel.h>
#include <linux/gpio/consumer.h>

#include "pt6964.h"

static inline void CLK(struct vfd_t *vfd, int value)
{
   gpiod_set_value(vfd->gpio_desc[GPIO_CLK], value);
}

static inline void STB(struct vfd_t *vfd, int value)
{
   gpiod_set_value(vfd->gpio_desc[GPIO_STB], value);
}

static inline void DO(struct vfd_t *vfd, int value)
{
   if (vfd->dido_gpio_out){
      gpiod_set_value(vfd->gpio_desc[GPIO_DIDO], value);
   } else {
      vfd->dido_gpio_out = 1;
      gpiod_direction_output(vfd->gpio_desc[GPIO_DIDO], value);
   }
}

static inline int DI(struct vfd_t *vfd)
{
   if (vfd->dido_gpio_out) {
      /* switch to input mode */
      vfd->dido_gpio_out = 0;
      gpiod_direction_input(vfd->gpio_desc[GPIO_DIDO]);
   }

   return gpiod_get_value(vfd->gpio_desc[GPIO_DIDO]);
}

// Send a byte to chip; assumes STB & CLK high
static void pt6964_send(struct vfd_t *vfd, u8 data)
{
   int i;
   for (i = 8; i != 0; i--, data >>= 1) {
      CLK(vfd, 0);
      DO(vfd, data & 1);
      ndelay(400);
      CLK(vfd, 1);
      ndelay(400);
   }
}

static void pt6964_cmd(struct vfd_t *vfd, u8 cmd)
{
   STB(vfd, 0);
   pt6964_send(vfd, cmd);
   STB(vfd, 1);
   udelay(1);
}

// Clear display RAM
static void pt6964_clear_dram(struct vfd_t *vfd)
{
   int i;

   DBG_TRACE;

   pt6964_cmd(vfd, CMD_DATA_SETTING(1, 0)); /* inc, write */

   STB(vfd, 0);
   pt6964_send(vfd, CMD_ADDRESS_SET(0));
   for (i = 0; i < RAW_DISPLAY_WORDS * 2; i++){
      pt6964_send(vfd, 0);
   }
   STB(vfd, 1);
   udelay(1);
}

#ifndef CONFIG_VFD_NO_KEY_INPUT

static u8 pt6964_read(struct vfd_t *vfd)
{
   int i;
   u8 d = 0;

   for (i = 1; i < 0x100; i <<= 1) {
      CLK(vfd, 0);
      ndelay(400);
      if (DI(vfd)) {
	 d |= i;
      }
      CLK(vfd, 1);
      ndelay(400);
   }

   return d;
}

/*
 * Returns the whole key state bitmap in a single 32-bit word
 */
u32 hardware_keys(struct vfd_t *vfd)
{
   int i;
   u32 keys = 0;

   STB (vfd, 0);

   // read data command
   pt6964_send(vfd, CMD_DATA_SETTING (0, 1)); /* inc = 0, read */
   udelay (1);

#if defined COMPAT_FD620
   /*
    * The order of keys in key state bitmap:
    * bit 0  - SEG1/KS1
    * bit 1  - SEG2/KS2
    * bit 2  - SEG3/KS3
    * bit 3  - SEG4/KS4
    * bit 4  - SEG5/KS5
    * bit 5  - SEG6/KS6
    * bit 6  - SEG7/KS7
    */

   for (i = 0; i < 8; i += 2) {
      u32 x = pt6964_read (vfd);
      x = (x & 0x01) | ((x & 8) >> 2);
      keys |= (x << i);
   }
#else
   /*
    * The order of keys in key state bitmap:
    * bit 0  - KS1+K1
    * bit 1  - KS1+K2
    * bit 2  - KS2+K1
    * bit 3  - KS2+K2
    * bit 4  - KS3+K1
    * bit 5  - KS3+K2
    * bit 6  - KS4+K1
    * bit 7  - KS4+K2
    * ...
    * bit 18 - KS10+K1
    * bit 19 - KS10+K2
    */

   for (i = 0; i < 20; i += 4) {
      u32 x = pt6964_read (vfd);
      x = (x & 0x03) | ((x & 18) >> 1);
      keys |= (x << i);
   }
#endif

   STB (vfd, 1);
   udelay(1);

   return keys;
}

#endif

void hardware_update_brightness(struct vfd_t *vfd)
{
   int bri = ! vfd->enabled ? 0 :
	       vfd->suspended ? vfd->brightness_suspend :
	       vfd->brightness;

   DBG_PRINT ("%d\n", bri);

   if (bri == 0){
      pt6964_cmd(vfd, CMD_DISPLAY_CONTROL(0, 0)); /* inc=0, write */
   } else {
      pt6964_cmd(vfd, CMD_DISPLAY_CONTROL(1, bri - 1));
   }
}

void hardware_suspend(struct vfd_t *vfd, int enable)
{
   DBG_TRACE;
   vfd->suspended = enable;
   hardware_update_brightness(vfd);
}

void hardware_update_display(struct vfd_t *vfd)
{
   unsigned i, pofs = 0x6964;
   u16 raw [ARRAY_SIZE(vfd->raw_display)];

   //DBG_TRACE;

   if (vfd->display_to_raw){
      vfd->display_to_raw (vfd, vfd->display, raw);
   }

   for (i = 0; i < ARRAY_SIZE(vfd->raw_display); i++){
      raw [i] |= vfd->raw_overlay [i];
   }

   // update on-chip display RAM
   for (i = 0; i < ARRAY_SIZE(vfd->raw_display); i++) {
      u16 r = raw [i];

      if (r == vfd->raw_display [i]){
	 continue;
      }

      vfd->raw_display [i] = r;

      if (pofs == 0x6964) {
	 // initialize write mode with auto-increment
	 pt6964_cmd(vfd, CMD_DATA_SETTING(1, 0)); /* inc, write */
      } else if (pofs != (i - 1)) {
	 // terminate previous command
	 STB(vfd, 1);
	 udelay(1);
      }

      if (pofs != (i - 1)) {
	 STB(vfd, 0);
	 pt6964_send(vfd, CMD_ADDRESS_SET(i * 2));
      }
      pofs = i;

      pt6964_send(vfd, r & 0xff);
      pt6964_send(vfd, r >> 8);
   }

   if (pofs != 0x6964) {
      STB(vfd, 1);
      udelay(1);
   }
}

#if defined PLATFORM_SEGNO
// the array that maps LCD segments a,b,c,d,e,f,g to bit numbers
static const u8 platform_segno [7] = PLATFORM_SEGNO;
#elif defined PLATFORM_CELLNO
// alternatively, for reverse-polarity displays
static const u8 platform_cellno [7] = PLATFORM_CELLNO;
static const u8 platform_cellbit [PLATFORM_DISPLAY_LEN] = PLATFORM_CELLBIT;
#endif

int hardware_init(struct vfd_t *vfd)
{
   DBG_TRACE;

   vfd->display_len = PLATFORM_DISPLAY_LEN;
   vfd->brightness = 1 + PLATFORM_BRIGHTNESS;
   vfd->brightness_max = 1 + BRIGHTNESS_MAX;
   vfd->brightness_suspend = 0;
   vfd->enabled = 1;
   vfd->dido_gpio_out = 0;

#ifdef PLATFORM_SEGNO
   vfd_init_glyphs_cc (vfd, platform_segno);
#elif defined PLATFORM_CELLNO
   vfd_init_glyphs_ca (vfd, platform_cellno, platform_cellbit);
#endif

   // set up GPIO modes
   gpiod_direction_output(vfd->gpio_desc [GPIO_STB], 1);
   gpiod_direction_output(vfd->gpio_desc [GPIO_CLK], 1);
   gpiod_direction_input(vfd->gpio_desc [GPIO_DIDO]);

   udelay(10);

   pt6964_clear_dram(vfd);
   pt6964_cmd(vfd, CMD_DISPLAY_MODE(PLATFORM_DISPLAY_MODE));
   hardware_update_brightness(vfd);

   return 0;
}
