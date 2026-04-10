#ifndef MT6320_BATTERY_H
#define MT6320_BATTERY_H

#include <linux/ioctl.h>

/*****************************************************************************
 *  BATTERY VOLTAGE
 ****************************************************************************/
#define PRE_CHARGE_VOLTAGE                  3200

#ifndef VENDOR_EDIT
//Fanhong.Kong@ProDrv.CHG, 2012/03/13, Modify for  power off vol
#define SYSTEM_OFF_VOLTAGE                  3400      
#else /* VENDOR_EDIT */
#define SYSTEM_OFF_VOLTAGE                  3490    
#define SYSTEM_OFF_VOLTAGE_HIGH                3550    
#endif /* VENDOR_EDIT */


#define CONSTANT_CURRENT_CHARGE_VOLTAGE     4100  
#define CONSTANT_VOLTAGE_CHARGE_VOLTAGE     4200  
#define CV_DROPDOWN_VOLTAGE                 4000
#define CHARGER_THRESH_HOLD                 4300
#define BATTERY_UVLO_VOLTAGE                2700

/*****************************************************************************
 *  BATTERY TIMER
 ****************************************************************************/

//#define MAX_CHARGING_TIME                   6*60*60 //battery_common.h
#ifdef VENDOR_EDIT//Fanhong.Kong@ProDrv.CHG, 2012.6.29 for 12015 
#define MAX_CHARGING_TIME_12015             10*60*60
#endif/*VENDOR_EDIT*/

#define MAX_POSTFULL_SAFETY_TIME       		1*30*60 	
#define MAX_PreCC_CHARGING_TIME         	1*30*60  	
#define MAX_CV_CHARGING_TIME              	3*60*60 	
#define MUTEX_TIMEOUT                       5000
//#define BAT_TASK_PERIOD                     (D_BAT_THREAD_CB_TIME/1000)	 //battery_common.h	
#define g_free_bat_temp 					1000 	

#define D_MAX_TOPOFF_TIME					1*60*60


/*****************************************************************************
 *  BATTERY Protection
 ****************************************************************************/
#define Battery_Percent_100    100
#define charger_OVER_VOL	    1
#define BATTERY_UNDER_VOL		2
#define BATTERY_OVER_TEMP		3
#define ADC_SAMPLE_TIMES        5

#ifdef VENDOR_EDIT//Fanhong.Kong@ProDrv.CHG, add 2012.1.3 for charging status
#define     Notify_Charger_Over_Vol                   	1 
#define     Notify_Charger_Low_Vol                    	2 
#define     Notify_Bat_Over_Temp                      	3
#define     Notify_Bat_Low_Temp                       	4
#define     Notify_Bat_Not_Connect                    	5
#define     Notify_Bat_Over_Vol                       	6
#define     Notify_Bat_Full                           	7
#define     Notify_Chging_Current                     	8
#define		Notify_Chging_OverTime					  	9
#define		Notify_Bat_Full_Pre_High_Temp			  	10
#define		Notify_Bat_Full_Pre_Low_Temp2			  	11
#define		Notify_Bat_Full_THIRD_BATTERY			  	14


enum{
	HW_VERSION__UNKNOWN,
	HW_VERSION__EVT, 	
	HW_VERSION__DVT, 	
	HW_VERSION__MCU, 	
	HW_VERSION__15041, 	
};

#define HW_ID_0							GPIO152
#define HW_ID_1							GPIO153
#define HW_ID_2							GPIO151
#define HW_ID_3							GPIO138


#endif/*VENDOR_EDIT*/

/*****************************************************************************
 *  Pulse Charging State
 ****************************************************************************/




/*****************************************************************************
 *  Type define
 ****************************************************************************/
typedef unsigned int       WORD;

/*****************************************************************************
 *  Extern Function
 ****************************************************************************/
extern void do_chrdet_int_task(void);
#ifdef CONFIG_MTK_SMART_BATTERY

extern void wake_up_bat(void);
extern unsigned long BAT_Get_Battery_Voltage(int polling_mode);

#else

#define wake_up_bat()			do {} while (0)
#define BAT_Get_Battery_Voltage()	({ 0; })

#endif

#endif

