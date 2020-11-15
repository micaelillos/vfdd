/*
 * Private definitions and functions for VFD driver
 */

#ifndef __VFD_PRIV_H__
#define __VFD_PRIV_H__

#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_VFD_NO_DELAYS
#undef udelay
#undef ndelay
#define udelay(x)
#define ndelay(x)
#endif

//#define VFD_DEBUG
#ifdef VFD_DEBUG
#define DBG_PRINT(msg,...)	printk ("%s:%d (%s): " msg, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define DBG_TRACE		printk ("%s:%d (%s)\n", __FILE__, __LINE__, __FUNCTION__)
#else
#define DBG_PRINT(msg,...)
#define DBG_TRACE
#endif

#if defined CONFIG_VFD_PT6964_T95U
#  define COMPAT_FD620
#else
#  define COMPAT_PT6964
#endif

#if defined COMPAT_FD620
// 5 16-bit words of display RAM
#define RAW_DISPLAY_WORDS		5
#else
// 7 16-bit words of display RAM
#define RAW_DISPLAY_WORDS		7
#endif

struct vfd_key_t {
	/* linux key code */
	u16 keycode;
	/* key scan code */
	u16 scancode;
};

enum
{
	// STB signal index in gpio_xxx[] arrays
	GPIO_STB,
	// CLK signal index
	GPIO_CLK,
	// DI/DO signal index
	GPIO_DIDO,

	// number of GPIO entries in gpio_xxx[] arrays
	GPIO_MAX
};

struct vfd_t {
	/* the global device lock */
	struct mutex lock;

	struct input_dev *input;
	struct timer_list timer;

	/* bus gpio pin descriptors */
	struct gpio_desc *gpio_desc [GPIO_MAX];
	/* 1 if DI/DO pin is in output mode */
	int dido_gpio_out;

	/* number of keys defined in DTS */
	int num_keys;
	/* scancode -> keycode map */
	struct vfd_key_t *keys;
	/* The scancode of last key pressed */
	u8 last_scancode;
	/* The linux keycode of last key pressed */
	u16 last_keycode;

	/* Number of GLYPHS on the indicator */
	int display_len;
	/* the raw overlay data for additional bits to be set (extra LEDs) */
	u16 raw_overlay [RAW_DISPLAY_WORDS];
	/* The raw display content (device-dependent format) */
	u16 raw_display [RAW_DISPLAY_WORDS];

	/* Display brightness */
	u8 brightness;
	/* Maximal brightness */
	u8 brightness_max;
	/* Brightness in suspend mode */
	u8 brightness_suspend;
	/* Display enabled (1) or disabled (0) */
	u8 enabled;
	/* Operating system suspended (1) or resumed (0) */
	u8 suspended;
	/* Set to 1 if any variables affecting display have changed */
	u8 need_update;
	/* Boot animation stage */
	u8 boot_anim;
	/* the state of up to 20 keys */
	u32 keystate;

#ifdef CONFIG_HAS_EARLYSUSPEND
	/* early suspend structure */
	struct early_suspend early_suspend;
#endif
};

typedef int (*type_vfd_printk) (const char *fmt, ...);

extern int get_vfd_key_value (void);
extern int set_vfd_led_value (char *display_code);
extern void Led_Show_lockflg (bool lockflg);

/* These functions must be provided by the backend */

/*
 * Initialize the backend. Returns 0 or -errno.
 */
extern int hardware_init (struct vfd_t *vfd);

/*
 * Returns key pressed bitmap in the following order:
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
extern u32 hardware_keys(struct vfd_t *vfd);

/*
 * Update display brightness from vfd structure
 * @param vfd
 *      the platform device structure, defining the brightness
 */
extern void hardware_update_brightness(struct vfd_t *vfd);

/*
 * Suspend (enable=1) or resume (enable=0) the LCD.
 */
extern void hardware_suspend(struct vfd_t *vfd, int enable);

/*
 * Update display cells that were changed.
 * @param vfd
 *      the platform device structure
 */
extern void hardware_update_display(struct vfd_t *vfd);

#endif /* __VFD_PRIV_H__ */
