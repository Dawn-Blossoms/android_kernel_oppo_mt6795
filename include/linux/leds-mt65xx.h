/******************************************************************************
 * mt65xx_leds.h
 *
 * Copyright 2010 MediaTek Co.,Ltd.
 *
 ******************************************************************************/
#ifndef _MT65XX_LEDS_H
#define _MT65XX_LEDS_H

#include <linux/leds.h>
#include <cust_leds.h>

extern int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness value);
extern int backlight_brightness_set(int level);

#ifdef VENDOR_EDIT
extern void mt_step_set_pmic(int step);
extern void mt_duty_set_pmic(int duty);
extern int  mt_step_get_pmic(void);
extern int mt_duty_get_pmic(void);
#endif /* VENDOR_EDIT */

#endif
