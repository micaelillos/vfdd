/*
 * linux/drivers/input/vfd/vfd.c
 *
 * 7-segment LED display driver
 *
 * Copyright (c) 2011 tiejun_peng, Amlogic Corporation
 * Copyright (c) 2017 Andrew Zabolotny <zapparello@ya.ru>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <linux/major.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "vfd-priv.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
static void vfd_early_suspend (struct early_suspend *h);
#endif

static const char *skip_nspaces (const char *str, int *count)
{
   while (*count && isspace (*str)) {
      (*count)--;
      str++;
   }
   return str;
}

//***//***//***//***//***//***// sysfs support //***//***//***//***//***//***//

static ssize_t key_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	return sprintf(buf, "%u %u\n", vfd->last_scancode, vfd->last_keycode);
}

static ssize_t display_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	memcpy (buf, vfd->display, vfd->display_len);
	return vfd->display_len;
}

static void _display_store(struct vfd_t *vfd, const char *buf, size_t count)
{
	size_t n = (count > vfd->display_len) ? vfd->display_len : count;

	mutex_lock(&vfd->lock);
	/* pad with spaces */
	memset(vfd->display + n, 0, vfd->display_len - n);
	memcpy(vfd->display, buf, n);
	vfd->need_update = 1;
	mutex_unlock(&vfd->lock);
}

static ssize_t display_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);

	_display_store (vfd, buf, count);

	return count;
}

static ssize_t overlay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < ARRAY_SIZE (vfd->raw_overlay); i++)
		sprintf(buf + i * 5, "%04x ", vfd->raw_overlay [i]);
	buf [ARRAY_SIZE (vfd->raw_overlay) * 5 - 1] = 0;

	return ARRAY_SIZE (vfd->raw_overlay) * 5 - 1;
}

static ssize_t overlay_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);

	int i, n, left = count;
	char *endp;
	const char *cur = skip_nspaces (buf, &left);
	u16 raw_overlay [ARRAY_SIZE (vfd->raw_overlay)];

	for (i = 0; i < ARRAY_SIZE (vfd->raw_overlay); i++) {
		if (left <= 0)
			break;

		n = simple_strtoul (cur, &endp, 16);
		if (endp == cur)
			break;

		raw_overlay [i] = n;

		left -= (endp - cur);
		cur = skip_nspaces (endp, &left);
	}

	mutex_lock(&vfd->lock);
	memcpy (vfd->raw_overlay, raw_overlay, i * sizeof (u16));
	vfd->need_update = 1;
	mutex_unlock(&vfd->lock);

	return count;
}

static ssize_t enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", vfd->enabled);
}

static ssize_t _u8_store(struct vfd_t *vfd, const char *buf, size_t count, u8 *val, unsigned max)
{
	unsigned value;

	buf = skip_spaces (buf);

	if (kstrtouint(buf, 0, &value) != 0)
		return -EINVAL;

	if (value > max)
		value = max;

	mutex_lock(&vfd->lock);
	*val = value;
	mutex_unlock(&vfd->lock);

	return count;
}

static ssize_t enable_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	ssize_t ret = _u8_store (vfd, buf, count, &vfd->enabled, 1);
	if (ret > 0) {
		mutex_lock(&vfd->lock);
		hardware_update_brightness(vfd);
		mutex_unlock(&vfd->lock);
	}

	return ret;
}

static ssize_t brightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", vfd->brightness);
}

static ssize_t brightness_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	ssize_t ret = _u8_store (vfd, buf, count, &vfd->brightness, vfd->brightness_max);
	if (ret > 0) {
		mutex_lock(&vfd->lock);
		hardware_update_brightness (vfd);
		mutex_unlock(&vfd->lock);
	}

	return ret;
}

static ssize_t brightness_max_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", vfd->brightness_max);
}

static ssize_t brightness_suspend_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", vfd->brightness_suspend);
}

static ssize_t brightness_suspend_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	return _u8_store (vfd, buf, count, &vfd->brightness_suspend, vfd->brightness_max);
}

static ssize_t dotled_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	char *dst = buf;
	int i;

	for (i = 0; i < vfd->num_dotleds; i++) {
		struct vfd_dotled_t *dotled = &vfd->dotleds [i];
		int state = (vfd->raw_overlay [dotled->word] &
			(1 << dotled->bit)) ? 1 : 0;
		dst += sprintf(dst, "%s %d %d %d\n",
			dotled->name, state, dotled->word, dotled->bit);
	}

	return dst - buf;
}

static ssize_t dotled_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct vfd_t *vfd = dev_get_drvdata(dev);
	const char *cur = buf;
	int i, left = count;

	while (left) {
		cur = skip_nspaces (cur, &left);

		for (i = 0; i < vfd->num_dotleds; i++) {
			struct vfd_dotled_t *dotled = &vfd->dotleds [i];
			int n = strlen (dotled->name);
			if (n + 1 > left)
				continue;

			if ((strncmp (cur, dotled->name, n) == 0) &&
			    isspace (cur [n])) {
				unsigned ena;
				char *endp;

				left -= n;
				cur = skip_nspaces (cur + n, &left);
				ena = simple_strtoul(cur, &endp, 0);
				if (endp == cur)
					goto error;

				left -= (endp - cur);
				cur = endp;

				mutex_lock(&vfd->lock);
				if (ena)
					vfd->raw_overlay [dotled->word] |= (1 << dotled->bit);
				else
					vfd->raw_overlay [dotled->word] &= ~(1 << dotled->bit);
				vfd->need_update = 1;
				mutex_unlock(&vfd->lock);
				break;
			}
		}

		if (left && (i >= vfd->num_dotleds))
			goto error;
	}

	return count;

error:
	/* unknown garbage found, barf but swallow */
	dev_err(dev, "garbage encountered at pos %d: [NAME] [STATE] expected\n",
		(int)(cur - buf));
	return count;
}

static DEVICE_ATTR_RO(key);
static DEVICE_ATTR_RW(display);
static DEVICE_ATTR_RW(overlay);
static DEVICE_ATTR_RW(enable);
static DEVICE_ATTR_RW(brightness);
static DEVICE_ATTR_RO(brightness_max);
static DEVICE_ATTR_RW(brightness_suspend);
static DEVICE_ATTR_RW(dotled);

static const struct device_attribute *all_attrs [] = {
	&dev_attr_key, &dev_attr_display, &dev_attr_overlay, &dev_attr_enable,
	&dev_attr_brightness, &dev_attr_brightness_max, &dev_attr_brightness_suspend,
	&dev_attr_dotled,
};

//***//***//***//***//***//***// input support //***//***//***//***//***//***//

#ifndef CONFIG_VFD_NO_KEY_INPUT
static const u8 MultiplyDeBruijnBitPosition[32] = 
{
	0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
	31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

static void vfd_scan_keys(struct vfd_t *vfd)
{
	// key emitted flag
	int i, emit = 0;
	// key state bitvector
	u32 keys;
	// find out which key states have changed
	u32 keydiff;

	mutex_lock(&vfd->lock);
	keys = hardware_keys (vfd);
	mutex_unlock(&vfd->lock);

	keydiff = keys ^ vfd->keystate;

	// emit EV_KEY events for changed keys
	while (keydiff != 0) {
		// separate the lowest 1 bit
		u32 v = keydiff & -keydiff;
		// find the number of trailing zeros in a 32-bit integer
		// ref: http://supertech.csail.mit.edu/papers/debruijn.pdf
		u32 sc = MultiplyDeBruijnBitPosition[((u32)((v & -v) * 0x077CB531U)) >> 27];

		DBG_PRINT ("key scancode %d is now %s\n", sc, (keys & v) ? "down" : "up");
		vfd->last_scancode = sc;
		vfd->last_keycode = 0;

		// convert scancode to keycode
		for (i = 0; i < vfd->num_keys; i++)
			if (sc == vfd->keys [i].scancode) {
				// report key up/down
				vfd->last_keycode = vfd->keys [i].keycode;
				input_report_key(vfd->input, vfd->last_keycode, keys & v);
				emit = 1;
			}

		// drop this bit in keydiff
		keydiff ^= v;
	}
	if (emit)
		input_sync(vfd->input);

	vfd->keystate = keys;
}
#endif

//***//***//***//***//***//***// timer interrupt //***//***//***//***//***//***//

static char boot_anim [][4] = {
   "boot",
   "b__t",
   "boot",
   "b00t",
   "b**t",
   "b~~t",
};

//void vfd_timer_sr(unsigned long data)
void vfd_timer_sr(struct timer_list *t )
{
   struct vfd_t *vfd = from_timer(vfd, t, timer);

#ifndef CONFIG_VFD_NO_KEY_INPUT
   if (vfd->input)
      vfd_scan_keys(vfd);
#endif

   if (unlikely (vfd->boot_anim)) {
      vfd->boot_anim--;
      _display_store(vfd, boot_anim [vfd->boot_anim], 4);
   }

   if (unlikely (vfd->need_update)) {
      vfd->need_update = 0;
      hardware_update_display (vfd);
   }

   mod_timer(&vfd->timer, jiffies + msecs_to_jiffies(100));
}

//***//***//***//***// Platform device implementation //***//***//***//***//

/* Set up data bus GPIOs */
static int __setup_gpios (struct platform_device *pdev, struct vfd_t *vfd)
{
   int i, n, ret;
   struct gpio_desc *desc;

   for (i = 0; i < GPIO_MAX; i++) {
      desc = devm_gpiod_get_from_of_node(&pdev->dev, pdev->dev.of_node,
			      "gpios", i, OF_GPIO_ACTIVE_LOW, "hq");
      if (IS_ERR(desc)) {
	 dev_err(&pdev->dev, "%d bus signal GPIOs must be defined", GPIO_MAX);
	 return PTR_ERR (desc);
      }

      n = desc_to_gpio(desc);

      gpio_free(n);

      if ((ret = gpio_request( n, "vfd")) < 0) {
	 dev_info(&pdev->dev, "failed to request gpio %d\n", n);
	 return ret;
      }

      vfd->gpio_desc [i] = desc;
   }

   dev_info(&pdev->dev, "bus signals STB,CLK,DI/DO mapped to GPIOs %d,%d,%d\n",
	    desc_to_gpio(vfd->gpio_desc[GPIO_STB]),
	    desc_to_gpio(vfd->gpio_desc[GPIO_CLK]),
	    desc_to_gpio(vfd->gpio_desc[GPIO_DIDO]));

   return 0;
}

/* Set up digit number order */
static int __setup_grid_num(struct platform_device *pdev, struct vfd_t *vfd)
{
   int n;
   char tmpbuf[256];
   char *dst = tmpbuf;
   int i;

   n = of_property_read_variable_u8_array(pdev->dev.of_node, "grid_num",
					  vfd->grid_num, 0, RAW_DISPLAY_WORDS);

   for (i = 0; i < n ; i++) {
      dst += sprintf(dst, " %d", vfd->grid_num[i] );
   }
   dev_info(&pdev->dev, "grid num %d -> %s\n", n, tmpbuf );

   vfd->grid_len = n;
   return n;
}

/* Set up segment order */
static int __setup_segment_no(struct platform_device *pdev, struct vfd_t *vfd)
{
   int n;
   char tmpbuf[256];
   char *dst = tmpbuf;
   int i;

   n = of_property_read_variable_u8_array(pdev->dev.of_node, "segment_no",
					  vfd->segment_no, 0, 8 );

   for (i = 0; i < n ; i++) {
      dst += sprintf(dst, " %c", vfd->segment_no[i] + 'a' );
   }
   dev_info(&pdev->dev, "segment no %d -> %s\n", n, tmpbuf );

   vfd->segment_len = n;
   return n;
}

/* Set up dot LEDs */
static int __setup_dotled (struct platform_device *pdev, struct vfd_t *vfd)
{
   int i;
   struct property *prop;
   u8 *ptr;

   vfd->num_dotleds = of_property_count_strings(pdev->dev.of_node, "dot_names");
   if (vfd->num_dotleds <= 0)
      return 0;
   
   prop = of_find_property(pdev->dev.of_node, "dot_bits", NULL);
   if ( ! prop || ! prop->value) {
      dev_err(&pdev->dev, "dot_names defined, but dot_bits is empty!\n");
      return 0;
   }
   if (prop->length != (vfd->num_dotleds * 2 * sizeof (u8))) {
      dev_err(&pdev->dev, "Number of entries mismatch in dot_names (%d) and dot_bits (%d)!\n",
	      vfd->num_dotleds, (int)(prop->length / (2 * sizeof (u8))));
      return 0;
   }

   vfd->dotleds = kmalloc (vfd->num_dotleds * sizeof (struct vfd_dotled_t), GFP_KERNEL);
   ptr = (u8 *)prop->value;
   for (i = 0; i < vfd->num_dotleds; i++) {
      if (of_property_read_string_index(pdev->dev.of_node, "dot_names",
					i, &vfd->dotleds [i].name) != 0)
	 /* this should never happen */
	 vfd->dotleds [i].name = "*BUG*";
      vfd->dotleds [i].word = *ptr++;
      vfd->dotleds [i].bit  = *ptr++;
      DBG_PRINT ("dot led '%s', word %d, bit %d\n",
		 vfd->dotleds [i].name, vfd->dotleds [i].word, vfd->dotleds [i].bit);
   }

   return 0;
}

/* Set up input device */
static int __setup_input (struct platform_device *pdev, struct vfd_t *vfd)
{
   int i, ret;
   struct input_dev *input;
   struct property *prop;

   /* parse key description from DTS */
   prop = of_find_property(pdev->dev.of_node, "key_codes", NULL);
   if (prop && prop->value && (prop->length >= (2 * sizeof (u16)))) {
      u16 *cur_key = (u16 *)prop->value;
      vfd->num_keys = prop->length / (2 * sizeof (u16));
      vfd->keys = kmalloc(vfd->num_keys * sizeof(struct vfd_key_t), GFP_KERNEL);
      for (i = 0; i < vfd->num_keys; i++) {
	 vfd->keys[i].scancode = be16_to_cpup(cur_key++);
	 vfd->keys[i].keycode = be16_to_cpup(cur_key++);
	 DBG_PRINT ("key scan %d, code %d\n",
		    vfd->keys[i].scancode, vfd->keys[i].keycode);
      }
   }

   if (vfd->num_keys == 0)
      return 0;

   vfd->input = input = input_allocate_device();
   if (!input)
      return -ENOMEM;

   /* input device generates only EV_KEY's */
   set_bit(EV_KEY, input->evbit);
   for (i = 0; i < vfd->num_keys; i++)
      set_bit (vfd->keys[i].keycode, input->keybit);
   
   input->name = "vfd_keypad";
   input->phys = "vfd_keypad/input0";
   input->dev.parent = &pdev->dev;

   input->id.bustype = BUS_ISA;
   input->id.vendor = 0x0001;
   input->id.product = 0x0001;
   input->id.version = 0x0100;

   if ((ret = input_register_device(input)) < 0) {
      printk(KERN_ERR "Unable to register vfdkeypad input device\n");
      return ret;
   }

   return 0;
}

static int vfd_probe(struct platform_device *pdev)
{
   int i, ret;
   struct vfd_t *vfd;

   vfd = kzalloc(sizeof(struct vfd_t), GFP_KERNEL);
   if ( ! vfd){
      return -ENOMEM;
   }
   mutex_init(&vfd->lock);
   platform_set_drvdata(pdev, vfd);

   if ((ret = __setup_gpios (pdev, vfd)) < 0){
      goto err1;
   }
   if ((ret = __setup_grid_num (pdev, vfd)) < 0){
      goto err1;
   }
   if ((ret = __setup_segment_no (pdev, vfd)) < 0){
      goto err1;
   }

   if ((ret = hardware_init(vfd)) != 0) {
      dev_err(&pdev->dev, "vfd hardware init failed!\n");
      goto err1;
   }

   /* display boot animation */
   vfd->boot_anim = ARRAY_SIZE (boot_anim);

//	setup_timer(&vfd->timer, vfd_timer_sr, (unsigned long)vfd);
   timer_setup(&vfd->timer, vfd_timer_sr, 0);
   mod_timer(&vfd->timer, jiffies+msecs_to_jiffies(100));

   /* register sysfs attributes */
   for (i = 0; i < ARRAY_SIZE (all_attrs); i++){
      if ((ret = device_create_file(&pdev->dev, all_attrs [i])) < 0){
	 goto err2;
      }
   }

   /* create the dot-LED objects */
   if ((ret =  __setup_dotled (pdev, vfd)) < 0)
      goto err2;

   /* set up input device, if needed */
   if ((ret = __setup_input (pdev, vfd)) < 0)
      goto err2;

#ifdef CONFIG_HAS_EARLYSUSPEND
   vfd->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
   vfd->early_suspend.suspend = vfd_early_suspend;
   vfd->early_suspend.param = pdev;
   register_early_suspend (&vfd->early_suspend);
#endif

   return 0;

err2:
   for (i = ARRAY_SIZE (all_attrs) - 1; i >= 0; i--)
      device_remove_file (&pdev->dev, all_attrs [i]);
err1:
   if (vfd->input != NULL)
      input_free_device(vfd->input);
   kfree(vfd);

   return ret;
}

static int vfd_remove(struct platform_device *pdev)
{
   int i;
   struct vfd_t *vfd = platform_get_drvdata(pdev);

#ifdef CONFIG_HAS_EARLYSUSPEND
   unregister_early_suspend (&vfd->early_suspend);
#endif

   /* unregister everything */
   for (i = ARRAY_SIZE (all_attrs) - 1; i >= 0; i--)
      device_remove_file (&pdev->dev, all_attrs [i]);

   if (vfd->input != NULL)
      input_free_device(vfd->input);

   kfree(vfd);

   return 0;
}

static void uevent_suspend (struct platform_device *pdev, int suspend)
{
	char tmp [16];
	char *env [2];

	/* notify userspace of the device status,
	 * one could display something on display when device suspends
	 * (but don't forget to set brightness_suspend which is 0 by default)
	 */
	snprintf (tmp, sizeof (tmp), "SUSPEND=%d", suspend);
	env[0] = tmp;
	env[1] = NULL;
	kobject_uevent_env(&pdev->dev.kobj, KOBJ_CHANGE, env);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void vfd_early_suspend (struct early_suspend *h)
{
	struct platform_device *pdev = (struct platform_device *)h->param;
	DBG_TRACE;
	uevent_suspend (pdev, 1);
}
#endif

static int vfd_power_common (struct platform_device *pdev, int suspend)
{
	struct vfd_t *vfd = platform_get_drvdata(pdev);

	mutex_lock(&vfd->lock);
	hardware_suspend(vfd, suspend);
	mutex_unlock(&vfd->lock);

	/* we only care about resume here, suspend is handled in earlysuspend */
	if (!suspend)
		uevent_suspend (pdev, suspend);

	return 0;
}

static int vfd_suspend(struct platform_device *pdev, pm_message_t state)
{
	DBG_TRACE;
	return vfd_power_common (pdev, 1);
}

static int vfd_resume(struct platform_device *pdev)
{
	DBG_TRACE;
	return vfd_power_common (pdev, 0);
}

static void vfd_shutdown(struct platform_device *pdev)
{
	DBG_TRACE;
	vfd_power_common (pdev, 1);
}

static const struct of_device_id vfd_dt_match[] = {
   {
      .compatible     = "amlogic,aml_vfd",
   },
   {},
};
static struct platform_driver vfd_driver = {
   .probe      = vfd_probe,
   .remove     = vfd_remove,
   .suspend    = vfd_suspend,
   .resume     = vfd_resume,
   .shutdown   = vfd_shutdown,
   .driver     = {
      .name   = "m1-vfd.0",
      .of_match_table = vfd_dt_match,
   },
};

static int vfd_driver_init(void)
{
   return platform_driver_register(&vfd_driver);
}

static void __exit vfd_driver_exit(void)
{
   platform_driver_unregister(&vfd_driver);
}

module_exit(vfd_driver_exit);

#ifdef CONFIG_VFD_SUPPORT_MODULE
  module_init(vfd_driver_init);
#else
/* If statically linked, initialize as early as possible (but after gpiolib & input) */
  subsys_initcall_sync(vfd_driver_init);
#endif

MODULE_AUTHOR("tiejun_peng");
MODULE_DESCRIPTION("Amlogic VFD Driver");
MODULE_LICENSE("GPL");
