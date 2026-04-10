/*************************************************************
 ** Copyright (C), 2008-2013, OPPO Mobile Comm Corp., Ltd
 ** VENDOR_EDIT
 ** File : cust_mag.c
 ** Description : MAG Driver Custom Para
 ** Date : 2013-12-05 15:56
 ** Author : Prd.SenDrv
 **
 ** ------------------ Revision History: ---------------------
 ** <author> <date> <desc>
 ** Prd.SenDrv 2013/12/05 NULL
 *************************************************************/ 
#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_mag.h>

#ifdef VENDOR_EDIT//Shaoyu.Huang@Prd.BasicDrv.Sensor,add 2012/5/28 for gsensor power
#include <linux/delay.h>
#include <mach/mt_gpio.h>
/*static int power(struct mag_hw *hw, unsigned int on, char *devname)
{
	static unsigned int status = 0;
	if (on == status){
		return 0;
	}
	if (on){
		hwPowerOn(MT6323_POWER_LDO_VGP2,VOL_2800,"msensor_2800");
		msleep(1);
		hwPowerOn(MT6323_POWER_LDO_VGP1,VOL_1800,"msensor_1800");
		msleep(1);		
	}
	else{

		hwPowerDown(MT6323_POWER_LDO_VGP1,"msensor_1800");
		msleep(1);	
		hwPowerDown(MT6323_POWER_LDO_VGP2,"msensor_2800");
	}

	status = on;
	return 0;
}*/

static int power(struct mag_hw *hw, unsigned int on, char *devname)
{
	static unsigned int status = 0;
	if (on == status){
		return 0;
	}
	if (on){
		//hwPowerOn(MT6325_POWER_LDO_VGP3, VOL_1800, devname);
		//hwPowerOn(MT6325_POWER_LDO_VIO28, VOL_2800, devname);
		msleep(1);		
	}
	else{
		//hwPowerDown(MT6325_POWER_LDO_VGP3,  devname);
		//hwPowerDown(MT6325_POWER_LDO_VIO28,  devname);
		msleep(1);
	}

	status = on;
	return 0;
}
#endif/*VENDOR_EDIT*/

static struct mag_hw cust_mag_hw = {
    .i2c_num = 3,//i2c-0
    .direction = 0,
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
#ifdef VENDOR_EDIT//mingqiang.guo@Prd.BasicDrv.Sensor, add 2012/8/13 for msensor power
    .power = power,
#endif/*VENDOR_EDIT*/
};

struct mag_hw* mmc3416x_get_cust_mag_hw(void) 
{
    return &cust_mag_hw;
}


