#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg)

//#define DEBUG_CAMERA_HW_K
//#ifdef DEBUG_CAMERA_HW_K
#if 1
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         xlog_printk(ANDROID_LOG_ERR, PFX , fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
                do {    \
                   xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg); \
                } while(0)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#define PK_XLOG_INFO(fmt, args...)
#endif

extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);
extern void ISP_MCLK3_EN(BOOL En);
#ifndef VENDOR_EDIT
//jindian.guan@camera 20150527 modify for flash
int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{

u32 pinSetIdx = 0;//default main sensor

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4
#define IDX_PS_VIO 8
#define IDX_PS_VDIG 12
#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3


u32 pinSet[3][16] = {
                        //for main sensor
                     {
                        CAMERA_CMRST_PIN,
                        CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        CAMERA_CMPDN_PIN,
                        CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,

                        CAMERA_MAIN_VIO_PIN,
                        CAMERA_MAIN_VIO_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        CAMERA_MAIN_VDIG_PIN,
                        CAMERA_MAIN_VDIG_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
						
                     },
                     //for sub sensor
                     {
                        CAMERA_CMRST1_PIN,
                        CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        CAMERA_CMPDN1_PIN,
                        CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,

                        CAMERA_SUB_VIO_PIN,
                        CAMERA_SUB_VIO_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     },
                     //for main_2 sensor
                     {
                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,

                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     }
                   };


#if 0
    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx){
        pinSetIdx = 0;
		PK_DBG_FUNC("[CAMERA SENSOR] 1!! \n");
    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        pinSetIdx = 1;
		PK_DBG_FUNC("[CAMERA SENSOR] 2!! \n");
    }
    else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
        pinSetIdx = 2;
		PK_DBG_FUNC("[CAMERA SENSOR] 3!! \n");
    }
#else
    if ((DUAL_CAMERA_MAIN_SENSOR == SensorIdx)&&currSensorName && (0 == strcmp(SENSOR_DRVNAME_IMX278_MIPI_RAW, currSensorName)))
	{
        pinSetIdx = 0;
		PK_DBG_FUNC("[CAMERA SENSOR] 21!! \n");
    }
	else if ((DUAL_CAMERA_SUB_SENSOR == SensorIdx)&&currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV8858_MIPI_RAW, currSensorName)))
	{
        pinSetIdx = 1;
		PK_DBG_FUNC("[CAMERA SENSOR] 22!! \n");
    }
	else
		{
		PK_DBG_FUNC("[CAMERA SENSOR] 23!! \n");
		return 0;}
#endif
   
    //power ON
    if (On) {
        if(pinSetIdx == 0 ) {
            ISP_MCLK1_EN(1);
			PK_DBG("[CAMERA SENSOR] 4!! \n");
        }
        else if (pinSetIdx == 1) {
//			if(mt_set_gpio_mode(GPIO39,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
//            if(mt_set_gpio_dir(GPIO39,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
//            if(mt_set_gpio_out(GPIO39,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
//            mdelay(3);
 //           if(mt_set_gpio_out(GPIO39,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
 //           mdelay(3);

			if(mt_set_gpio_mode(GPIO39,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
	           mdelay(3);		
			
 //           ISP_MCLK3_EN(1);
			ISP_MCLK2_EN(1); //add lxl 
			PK_DBG("[CAMERA SENSOR] 5!! \n");
        }
        else if (pinSetIdx == 2) {
            ISP_MCLK2_EN(1);
			PK_DBG("[CAMERA SENSOR] 6!! \n");
        }


        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VIO]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VIO],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
			PK_DBG_FUNC("[CAMERA SENSOR] 7!! \n");
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
        }
  

        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VDIG]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VDIG],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
			PK_DBG_FUNC("[CAMERA SENSOR] 8!! \n");
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
        }


        //VCAM_IO

		 
        //VCAM_A
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
        {
            PK_DBG_FUNC("[CAMERA SENSOR] Fail to enable analog power\n");
            goto _kdCISModulePowerOn_exit_;
        }
 
        //DVDD

 if(pinSetIdx == 1 ) {
 	PK_DBG_FUNC("[CAMERA SENSOR] 88!! \n");
            if(TRUE != hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1200,mode_name))
            {
            PK_DBG_FUNC("[CAMERA SENSOR] 87!! \n");
                 PK_DBG_FUNC("[CAMERA SENSOR] Fail to enable digital power\n");
                 goto _kdCISModulePowerOn_exit_;
            }
 	}

 if(pinSetIdx == 0 ){
        //AF_VCC
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable af power\n");
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }
}

        //enable active sensor
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(3);
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
			PK_DBG_FUNC("[CAMERA SENSOR] 10!! \n");

            //PDN pin
            if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV16825_MIPI_RAW, currSensorName))) 
            {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                PK_DBG("[CAMERA SENSOR] SENSOR_DRVNAME_OV16825_MIPI_RAW Set IDX_PS_CMPDN low \n");

            }
            else
            {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
PK_DBG_FUNC("[CAMERA SENSOR] 11!! \n");
            }
        }
    }
    else {//power OFF
        if(pinSetIdx == 0 ) {
            ISP_MCLK1_EN(0);
        }
        else if (pinSetIdx == 1) {
 //           ISP_MCLK3_EN(0);
			ISP_MCLK2_EN(0); //add
        }
        else if (pinSetIdx == 2) {
            ISP_MCLK2_EN(0);
        }

        //PK_DBG("[OFF]sensorIdx:%d \n",SensorIdx);
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}

            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor

            if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV16825_MIPI_RAW, currSensorName))) 
            {
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ONE)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            }
            else
            {
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            }
        }

     if(pinSetIdx == 1 ) {	
                if(TRUE != hwPowerDown(SUB_CAMERA_POWER_VCAM_D, mode_name))
                {
                     PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                     goto _kdCISModulePowerOn_exit_;
                }
     	}

        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {
            PK_DBG("[CAMERA SENSOR] Fail to OFF analog power(%d)\n",CAMERA_POWER_VCAM_A);
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }

 if(pinSetIdx == 0 ){
        //AF_VCC
        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to disable af power\n");
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }
}

        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VIO]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VIO],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
        }
  

        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VDIG]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VDIG],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        }
	
    }//

    return 0;

_kdCISModulePowerOn_exit_:
    return -EIO;
    
}

EXPORT_SYMBOL(kdCISModulePowerOn);
#else
#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4
#define IDX_PS_VIO 8
#define IDX_PS_VDIG 12
#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3


u32 pinSet[3][16] = {
                        //for main sensor
                     {
                        CAMERA_CMRST_PIN,
                        CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        CAMERA_CMPDN_PIN,
                        CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,

                        CAMERA_MAIN_VIO_PIN,
                        CAMERA_MAIN_VIO_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        CAMERA_MAIN_VDIG_PIN,
                        CAMERA_MAIN_VDIG_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
						
                     },
                     //for sub sensor
                     {
                        CAMERA_CMRST1_PIN,
                        CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        CAMERA_CMPDN1_PIN,
                        CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,

                        CAMERA_SUB_VIO_PIN,
                        CAMERA_SUB_VIO_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     },
                     //for main_2 sensor
                     {
                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,

                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        GPIO_CAMERA_INVALID,
                        GPIO_MODE_00,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     }
                   };

int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{

u32 pinSetIdx = 0;//default main sensor


#if 0
    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx){
        pinSetIdx = 0;
		PK_DBG_FUNC("[CAMERA SENSOR] 1!! \n");
    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        pinSetIdx = 1;
		PK_DBG_FUNC("[CAMERA SENSOR] 2!! \n");
    }
    else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
        pinSetIdx = 2;
		PK_DBG_FUNC("[CAMERA SENSOR] 3!! \n");
    }
#else
    if ((DUAL_CAMERA_MAIN_SENSOR == SensorIdx)&&currSensorName && (0 == strcmp(SENSOR_DRVNAME_IMX278_MIPI_RAW, currSensorName)))
	{
        pinSetIdx = 0;
		PK_DBG_FUNC("[CAMERA SENSOR] 21!! \n");
    }
	else if ((DUAL_CAMERA_SUB_SENSOR == SensorIdx)&&currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV8858_MIPI_RAW, currSensorName)))
	{
        pinSetIdx = 1;
		PK_DBG_FUNC("[CAMERA SENSOR] 22!! \n");
    }
	else
		{
		PK_DBG_FUNC("[CAMERA SENSOR] 23!! \n");
		return 0;}
#endif
   
    //power ON
    if (On) {
        if(pinSetIdx == 0 ) {
            ISP_MCLK1_EN(1);
			PK_DBG("[CAMERA SENSOR] 4!! \n");
        }
        else if (pinSetIdx == 1) {
//			if(mt_set_gpio_mode(GPIO39,GPIO_MODE_00)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
//            if(mt_set_gpio_dir(GPIO39,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
//            if(mt_set_gpio_out(GPIO39,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
//            mdelay(3);
 //           if(mt_set_gpio_out(GPIO39,GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
 //           mdelay(3);

			if(mt_set_gpio_mode(GPIO39,GPIO_MODE_01)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
	           mdelay(3);		
			
 //           ISP_MCLK3_EN(1);
			ISP_MCLK2_EN(1); //add lxl 
			PK_DBG("[CAMERA SENSOR] 5!! \n");
        }
        else if (pinSetIdx == 2) {
            ISP_MCLK2_EN(1);
			PK_DBG("[CAMERA SENSOR] 6!! \n");
        }


        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VIO]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VIO],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
			PK_DBG_FUNC("[CAMERA SENSOR] 7!! \n");
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
        }
  
  if(pinSetIdx == 0 ){
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VDIG]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VDIG],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
			PK_DBG_FUNC("[CAMERA SENSOR] 8!! \n");
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
        }
    }

        //VCAM_IO

		 
        //VCAM_A
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
        {
            PK_DBG_FUNC("[CAMERA SENSOR] Fail to enable analog power\n");
            goto _kdCISModulePowerOn_exit_;
        }
 
        //DVDD

 if(pinSetIdx == 1 ) {
 	PK_DBG_FUNC("[CAMERA SENSOR] 88!! \n");
            if(TRUE != hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1200,mode_name))
            {
            PK_DBG_FUNC("[CAMERA SENSOR] 87!! \n");
                 PK_DBG_FUNC("[CAMERA SENSOR] Fail to enable digital power\n");
                 goto _kdCISModulePowerOn_exit_;
            }
 	}

 if(pinSetIdx == 0 ){
        //AF_VCC
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable af power\n");
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }
}

        //enable active sensor
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(3);
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
			PK_DBG_FUNC("[CAMERA SENSOR] 10!! \n");

            //PDN pin
            if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV16825_MIPI_RAW, currSensorName))) 
            {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
                PK_DBG("[CAMERA SENSOR] SENSOR_DRVNAME_OV16825_MIPI_RAW Set IDX_PS_CMPDN low \n");

            }
            else
            {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
PK_DBG_FUNC("[CAMERA SENSOR] 11!! \n");
            }
        }
    }
    else {//power OFF
        if(pinSetIdx == 0 ) {
            ISP_MCLK1_EN(0);
        }
        else if (pinSetIdx == 1) {
 //           ISP_MCLK3_EN(0);
			ISP_MCLK2_EN(0); //add
        }
        else if (pinSetIdx == 2) {
            ISP_MCLK2_EN(0);
        }

        //PK_DBG("[OFF]sensorIdx:%d \n",SensorIdx);
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}

            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor

            if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV16825_MIPI_RAW, currSensorName))) 
            {
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ONE)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            }
            else
            {
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            }
        }

     if(pinSetIdx == 1 ) {	
                if(TRUE != hwPowerDown(SUB_CAMERA_POWER_VCAM_D, mode_name))
                {
                     PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
                     goto _kdCISModulePowerOn_exit_;
                }
     	}

        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {
            PK_DBG("[CAMERA SENSOR] Fail to OFF analog power(%d)\n",CAMERA_POWER_VCAM_A);
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }

 if(pinSetIdx == 0 ){
        //AF_VCC
        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to disable af power\n");
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }
}

        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VIO]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VIO],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
        }
  
  if(pinSetIdx == 0 ){
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VDIG]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VDIG],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VDIG],pinSet[pinSetIdx][IDX_PS_VDIG+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        }
    }
  }//

    return 0;

_kdCISModulePowerOn_exit_:
    return -EIO;
    
}

EXPORT_SYMBOL(kdCISModulePowerOn);

int kdVIOPowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
    u32 pinSetIdx = 0;

    if (On) {
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VIO]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VIO],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
			PK_DBG_FUNC("[CAMERA SENSOR] 7!! \n");
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
           }

        }
    else{
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_VIO]) {
           if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
           if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_VIO],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
           if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_VIO],pinSet[pinSetIdx][IDX_PS_VIO+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
           mdelay(1);
           }
        }

}
EXPORT_SYMBOL(kdVIOPowerOn);

#endif
//!--
//


