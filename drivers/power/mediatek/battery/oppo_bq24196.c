/************************************************************************************
** File:  \\192.168.144.3\Linux_Share\12015\ics2\development\mediatek\custom\oppo77_12015\kernel\battery\battery
** VENDOR_EDIT
** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
** 
** Description: 
**      for dc-dc sn111008 charg
** 
** Version: 1.0
** Date created: 21:03:46,05/04/2012
** Author: Fanhong.Kong@ProDrv.CHG
** 
** --------------------------- Revision History: ------------------------------------------------------------
** 	<author>	<data>			<desc>
************************************************************************************************************/


#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#include <linux/xlog.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>


#include <oppo_bq24196.h>
#include <mach/battery_meter.h>	//modified by PengNan
#include "mt65xx_battery.h"
#include <mach/charging.h>

#include <mach/battery_common.h>


typedef enum
{
	BATTERY_STATUS__GOOD = 0,
	BATTERY_STATUS__REMOVED,
	BATTERY_STATUS__LOW_TEMP,
	BATTERY_STATUS__HIGH_TEMP,
	BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0,
	BATTERY_STATUS__PRE_LOW_TEMP_0_5,
	BATTERY_STATUS__PRE_LOW_TEMP_5_12,
//	BATTERY_STATUS__PRE_LOW_TEMP_0_12,
	BATTERY_STATUS__PRE_LOW_TEMP_12_16,
	BATTERY_STATUS__PRE_HIGH_TEMP,
	BATTERY_STATUS__INVALID
}TBatStatus;

bool bq24196_stop_charging(void);
bool bq24196_start_charging(void);
u8 bq24196_get_suspend_status(void);
void bq24196_unsuspend_charger(void);
void bq24196_suspend_charger(void);

extern TBatStatus get_battery_temp_status(void);
extern CHARGER_TYPE get_battery_charger_type(void);
extern void pchr_turn_off_charging_bq24196(void);

extern kal_bool oppo_high_battery_status;
extern kal_uint8 oppo_check_ID_status;


static u8 bq24196_charging_mode_get(void);

/*macores defined begin*/
#define IRQ_POWER_SUPPLY_UPDATE_DELAY	50//ms
#define CHG_POWER_UPPLY_UPDATE_DELAY 	50//ms
#define POWER_SUPPLY_UPDATE_PERIOD		5000//ms
#define START_CHG_DELAY					1000//ms

#define BAT_TEMP__INVALID			-273
#define BAT_TEMP__TEN_BELOW_ZERO	-10
#define BAT_TEMP__ZERO				0
#define BAT_TEMP__TEN				10
#define BAT_TEMP__FORTY_FIVE		45
#define BAT_TEMP__FIFTY_FIVE		55
#define BAT_TEMP__BOUNCE			3

/*initial charger and battery data*/
#define CHARGER_VOL__DEFAULT		5000
#define BAT_VOL__DEFAULT			3800//MV
#define BAT_TEMP__DEFAULT			25
#define BAT_TEMP_REGION__DEFAULT	BATTERY_TEMP_REGION__NORMAL
#define BAT_CAPACITY__DEFAULT		30

/*charging current set*/
#define STANDARD__PRE_CHG__LITTLE_COLD_CURRNET		PRE_CHG_CURRENT__450MA
#define STANDARD__PRE_CHG__COOL_CURRNET				PRE_CHG_CURRENT__450MA
#define STANDARD__PRE_CHG__NORMAL_CURRNET			PRE_CHG_CURRENT__450MA
#define STANDARD__PRE_CHG__WARM_CURRNET			PRE_CHG_CURRENT__450MA
#define STANDARD__FAST_CHG__LITTLE_COLD_CURRNET		256
#define STANDARD__FAST_CHG__COOL_CURRNET			512
#define STANDARD__FAST_CHG__WARM_CURRNET			512
#define STANDARD__TAPER_CHG__LITTLE_COLD_CURRNET	FAST_CHG_CURRENT__2000MA
#define STANDARD__TAPER_CHG__COOL_CURRNET			FAST_CHG_CURRENT__2000MA
#define STANDARD__TAPER_CHG__NORMAL_CURRNET		FAST_CHG_CURRENT__2000MA
#define STANDARD__TAPER_CHG__WARM_CURRNET			FAST_CHG_CURRENT__2000MA

#define STANDARD__FAST_CHG__PRE_COLD_CURRNET_NEG3_0			300	
#define STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12			900
#define STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12_PRE			500	//from 900 for new stadard 2015/03/18
#define STANDARD__FAST_CHG__PRE_COLD_CURRNET_12_16			1800 
//#define STANDARD__FAST_CHG__PRE_WARM_CURRNET				1500
#define STANDARD__FAST_CHG__PRE_WARM_CURRNET				1000	
#define STANDARD__FAST_CHG__NORMAL_CURRNET					2000
#define STANDARD__FAST_CHG__NORMAL_LED_ON_CURRNET			1200

#define NON_STANDARD__PRE_CHG__LITTLE_COLD_CURRNET		PRE_CHG_CURRENT__450MA
#define NON_STANDARD__PRE_CHG__COOL_CURRNET			PRE_CHG_CURRENT__450MA
#define NON_STANDARD__PRE_CHG__NORMAL_CURRNET			PRE_CHG_CURRENT__450MA
#define NON_STANDARD__PRE_CHG__WARM_CURRNET			PRE_CHG_CURRENT__450MA
#define NON_STANDARD__FAST_CHG__LITTLE_COLD_CURRNET	FAST_CHG_CURRENT__200MA
#define NON_STANDARD__FAST_CHG__COOL_CURRNET			FAST_CHG_CURRENT__600MA
#define NON_STANDARD__FAST_CHG__NORMAL_CURRNET		FAST_CHG_CURRENT__600MA
#define NON_STANDARD__FAST_CHG__WARM_CURRNET			FAST_CHG_CURRENT__600MA
#define NON_STANDARD__TAPER_CHG__LITTLE_COLD_CURRNET	FAST_CHG_CURRENT__600MA
#define NON_STANDARD__TAPER_CHG__COOL_CURRNET			FAST_CHG_CURRENT__600MA
#define NON_STANDARD__TAPER_CHG__NORMAL_CURRNET		FAST_CHG_CURRENT__600MA
#define NON_STANDARD__TAPER_CHG__WARM_CURRNET			FAST_CHG_CURRENT__600MA

#define USB__PRE_CHG__LITTLE_COLD_CURRNET		PRE_CHG_CURRENT__450MA
#define USB__PRE_CHG__COOL_CURRNET				PRE_CHG_CURRENT__450MA
#define USB__PRE_CHG__NORMAL_CURRNET			PRE_CHG_CURRENT__450MA
#define USB__PRE_CHG__WARM_CURRNET				PRE_CHG_CURRENT__450MA
#define USB__FAST_CHG__LITTLE_COLD_CURRNET		256

#define USB__FAST_CHG__PRE_COLD_CURRNET_NEG3_0		300
#define USB__FAST_CHG__PRE_COLD_CURRNET_0_12		1000
#define USB__FAST_CHG__PRE_COLD_CURRNET_12_16		1800
#define USB__FAST_CHG__PRE_WARM_CURRNET				1000
#define USB__FAST_CHG__NORMAL_CURRNET				2000



#define USB__FAST_CHG__COOL_CURRNET				512
#define USB__FAST_CHG__WARM_CURRNET				512
#define USB__TAPER_CHG__LITTLE_COLD_CURRNET		256
#define USB__TAPER_CHG__COOL_CURRNET				FAST_CHG_CURRENT__600MA
#define USB__TAPER_CHG__NORMAL_CURRNET			FAST_CHG_CURRENT__600MA
#define USB__TAPER_CHG__WARM_CURRNET			FAST_CHG_CURRENT__600MA

#define MAX_INPUT_CURRENT_LIMIT__STANDARD		MAX_CHG_CURRENT__2000MA		
#define MAX_INPUT_CURRENT_LIMIT__NON_STANDARD	MAX_CHG_CURRENT__500MA
#define MAX_INPUT_CURRENT_LIMIT__USB				MAX_CHG_CURRENT__500MA

/*irq flags bgein*/
#define IRQ_FLAGS__BAT_MISSING				1
#define IRQ_FLAGS__CHG_COMPLETE			2
#define IRQ_FLAGS__CHG_RESUME				3
#define IRQ_FLAGS__NO_CHG					4
#define IRQ_FLAGS__CHG_MODE_PRE			5
#define IRQ_FLAGS__CHG_MODE_FAST			6
#define IRQ_FLAGS__CHG_MODE_TAPER		7
#define IRQ_FLAGS__CHG_TYPE__INVALID		8
#define IRQ_FLAGS__CHG_TYPE__SDP			9
#define IRQ_FLAGS__CHG_TYPE__DCP			10
#define IRQ_FLAGS__CHG_TYPE__NON_DCP		11
/*irq flags end*/

/*charging complete  begin*/
#define CHG_COMPLETE_CHECK_COUNT			12
#define CHG_COMPLETE_CHECK__CURRENT		100//ma

#define CHG_COMPLETE_CHECK_VOLTAGE__NON_HI_BATTERY			4200//mv

#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__LITTLE_COLD		4000//mv
//modified by PengNan  for the NEW_CHARGER_STARDARD_V2_8 2015.12.3
//#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__COOL				4200//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__COOL				4320//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__NORMAL			4320//mv

#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__PRE_LITTLE_COOL	3960//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__PRE_COOL			4160//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__PRE_HIGH			4330//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__PRE_WARM			4060//mv


#define CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__WARM			4100//mv

#define CHG_COMPLETE_CHECK_VOLTAGE__NON_STANDARD__LITTLE_COLD	4000//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__NON_STANDARD__COOL			4300//4200//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__NON_STANDARD__NORMAL		4350//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__NON_STANDARD__WARM			4100//mv

#define CHG_COMPLETE_CHECK_VOLTAGE__USB__LITTLE_COLD			4000//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__USB__COOL					4300//4200//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__USB__NORMAL				4350//mv
#define CHG_COMPLETE_CHECK_VOLTAGE__USB__WARM					4100//mv
/*charging complete end*/

/*chargign resume begin*/
#define CHG_RESUME_CHECK_COUNT	12

#define CHG_RESUME_CHECK_VOLTAGE__STANDARD__LITTLE_COLD		3800//mv
#define CHG_RESUME_CHECK_VOLTAGE__STANDARD__COOL				4100//4000//mv
#define CHG_RESUME_CHECK_VOLTAGE__STANDARD__NORMAL			4200//mv
#define CHG_RESUME_CHECK_VOLTAGE__STANDARD__WARM				3900//mv

#define CHG_RESUME_CHECK_VOLTAGE__NON_STANDARD__LITTLE_COLD	3800//mv
#define CHG_RESUME_CHECK_VOLTAGE__NON_STANDARD__COOL			4100//4000//mv
#define CHG_RESUME_CHECK_VOLTAGE__NON_STANDARD__NORMAL		4200//mv
#define CHG_RESUME_CHECK_VOLTAGE__NON_STANDARD__WARM		3900//mv

#define CHG_RESUME_CHECK_VOLTAGE__USB__LITTLE_COLD				3800//mv
#define CHG_RESUME_CHECK_VOLTAGE__USB__COOL					4100//4000//mv
#define CHG_RESUME_CHECK_VOLTAGE__USB__NORMAL					4200//mv
#define CHG_RESUME_CHECK_VOLTAGE__USB__WARM					3900//mv
/*charging resume end*/

/*charger and battery u/ovp begin*/
#define CHARGER_OVP_CHECK_COUNT	3
#define CHARGER_UVP_CHECK_COUNT	3	
#define CHARGER_OVP_VOLTAGE		6800//mv
#define CHARGER_UVP_VOLTAGE		4500//mv

#define BATTERY_OVP_CHECK_COUNT	3
#define BATTERY_OVP_VOLTAGE		4500//mv
#define BATTERY_UVP_VOLTAGE		3000//mv

#define BAT_MISSING_CHECK_COUNT 	3
#define BAT_MISSING_VOLTAGE		2000//mv

#define BAT_MAX_DESIGNED_VOLTAGE 	4500
#define BAT_MIN_DESIGNED_VOLTAGE	2000//mv
/*charger and battery u/ovp end*/

/*charging timeout*/
#define CHG_TIME_OUT

//int g_temp_input_limit_cc_value = MAX_CHG_CURRENT__500MA;
int g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__NORMAL;

int g_temp_battery_limit_pcc_value = 512;
int g_temp_battery_limit_fcc_value = 512;

//#define VIDEO_MODE_CHARGING
#ifdef VIDEO_MODE_CHARGING
extern int OV5647MIPI_get_Video_Mode();
#endif

//#define OPPO_LED_CURRENT_COMPENSATION
//#ifdef OPPO_LED_CURRENT_COMPENSATION
extern volatile  int OPPO_LED_ON;
//#endif
//PengNan@Drv.CHG add for OPPO_LED_Current 2015/03/28
#define INPUT_CURRENT_LIMIT_LED_ON
#ifdef	INPUT_CURRENT_LIMIT_LED_ON
//bool input_current_limit_enable = false;
int input_current_selfadapt = 0;
#endif


extern int plug_in_flag_set_charging_current;
//#define OPPO_CMCC_TEST
#ifdef OPPO_CMCC_TEST//PengNan@OPPO.com for limit input current 2015/04/04
#define CMCC_INPUT_CURRENT_LIMIT_PROTECT
#endif
extern PMU_ChargerStruct BMT_status;
#define NEW_CHARGER_STARDARD_V2_8
int vol_count_temp5_12 = 0;
int vol_count_flag = 0;




#define bq24261_SLAVE_ADDR_WRITE   0xD6
#define bq24261_SLAVE_ADDR_Read    0xD7

static struct i2c_client *new_client = NULL;
static const struct i2c_device_id bq24196_i2c_id[] = {{"bq24196_i2c",0},{}};  

static struct i2c_board_info __initdata i2c_bq24196={ I2C_BOARD_INFO("bq24196_i2c", (bq24261_SLAVE_ADDR_WRITE >> 1))};

volatile kal_bool chargin_hw_init_done_bq24196 = 0;
static void bq24196_reset(struct i2c_client *client);

static int bq24196_driver_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int bq24196_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

struct i2c_driver bq24196_i2c_driver = {                       
    .probe = bq24196_driver_probe,                                       
    .detect = bq24196_driver_detect,                           
    .driver.name = "bq24196_i2c",                 
    .id_table = bq24196_i2c_id, 
//	.shutdown = bq24196_reset,
};



static DEFINE_MUTEX(bq24196_i2c_access);
/**********************************************************
  *
  *   [I2C Function For Read/Write bq24196] 
  *
  *********************************************************/
kal_uint32 bq24196_read_byte(kal_uint8 cmd, kal_uint8 *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&bq24196_i2c_access);
    
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], (1<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;

        mutex_unlock(&bq24196_i2c_access);
        return ret;
    }
    
    readData = cmd_buf[0];
    *returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    mutex_unlock(&bq24196_i2c_access);    
    return 0;
}

kal_uint32 bq24196_write_byte(kal_uint8 cmd, kal_uint8 writeData)
{
    char    write_data[2] = {0};
    int     ret=0;
    
    mutex_lock(&bq24196_i2c_access);
    
    write_data[0] = cmd;
    write_data[1] = writeData;
    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&bq24196_i2c_access);
        return ret;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&bq24196_i2c_access);
    return 0;
}

/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
kal_uint32 bq24196_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 bq24196_reg = 0;
    kal_uint32 ret = 0;

   //printk("--------------------------------------------------\n");
	
    ret = bq24196_read_byte(RegNum, &bq24196_reg);
	
   //printk("[bq24196_read_interface] Reg[%x]=0x%x\n", RegNum, bq24196_reg);
	
    bq24196_reg &= (MASK << SHIFT);
    *val = (bq24196_reg >> SHIFT);
	
   //printk("[bq24196_read_interface] val=0x%x\n", *val);
	
    return ret;
}

kal_uint32 bq24196_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 bq24196_reg = 0;
    kal_uint32 ret = 0;

  // printk("--------------------------------------------------\n");

    ret = bq24196_read_byte(RegNum, &bq24196_reg);
    //printk("[bq24196_config_interface] Reg[%x]=0x%x\n", RegNum, bq24196_reg);
    
    bq24196_reg &= ~(MASK << SHIFT);
    bq24196_reg |= (val << SHIFT);

    ret = bq24196_write_byte(RegNum, bq24196_reg);
    //printk("[bq24196_config_interface] write Reg[%x]=0x%x\n", RegNum, bq24196_reg);

    // Check
    bq24196_read_byte(RegNum, &bq24196_reg);
    printk("[bq24196_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq24196_reg);

    return ret;
}

//write one register directly
kal_uint32 bq24196_reg_config_interface (kal_uint8 RegNum, kal_uint8 val)
{   
    kal_uint32 ret = 0;
    
    ret = bq24196_write_byte(RegNum, val);

    return ret;
}

bool bq24196_float_voltage_write(int vfloat_mv)
{
	 u8 value;
    
	value = (vfloat_mv - BQ24196_MIN_FLOAT_MV)/BQ24196_VFLOAT_STEP_MV;
	value <<= REG04_BQ24196_CHARGING_VOL_LIMIT_SHIFT;
	//printk("bq24196_set_float_voltage value=%d\n", value);

	return bq24196_config_interface(REG04_BQ24196_ADDRESS, value, REG04_BQ24196_CHARGING_VOL_LIMIT_MASK, 0);
}




static bool bq24196_float_voltage__pre_low_temp2()
{
	return bq24196_float_voltage_write(CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__LITTLE_COLD);
}
static bool bq24196_float_voltage__pre_low_temp()
{
	return bq24196_float_voltage_write(CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__COOL);
}
static bool bq24196_float_voltage__good()
{
	return bq24196_float_voltage_write(CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__NORMAL);
}
static bool bq24196_float_voltage__pre_high_temp()
{
	return bq24196_float_voltage_write(CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__WARM);
}

bool bq24196_float_voltage_set()
{
	bool ret = false;
	static int check_term_voltage_count = 0; 
	int batt_vol_pre_high = 0;
	switch(get_battery_temp_status()){
		case BATTERY_STATUS__INVALID:
		case BATTERY_STATUS__REMOVED:
		case BATTERY_STATUS__LOW_TEMP:
		case BATTERY_STATUS__HIGH_TEMP:
			return ret;
			//break;
		case BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0:			
			if(BMT_status.bat_vol >= CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__PRE_LITTLE_COOL)
			{
				check_term_voltage_count++;
				if(check_term_voltage_count >= 3)
				{
					g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__LITTLE_COLD - BQ24196_VFLOAT_STEP_MV;
					check_term_voltage_count = 0;
		//			pchr_turn_off_charging_bq24196();
					batt_vol_pre_high = 1;
				}
			}
			else {
				g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__LITTLE_COLD;
				check_term_voltage_count = 0;
			}
			//ret = bq24196_float_voltage__pre_low_temp2();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_0_5:
		case BATTERY_STATUS__PRE_LOW_TEMP_5_12:	
			if(BMT_status.bat_vol >= CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__PRE_COOL)
			{
				check_term_voltage_count++;
				if(check_term_voltage_count >= 3)
				{
					g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__COOL - BQ24196_VFLOAT_STEP_MV;
					check_term_voltage_count = 0;
		//			pchr_turn_off_charging_bq24196();
					batt_vol_pre_high = 1;
				}
			}
			else {
				g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__COOL;
				check_term_voltage_count = 0;

			}
			//ret = bq24196_float_voltage__pre_low_temp();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_12_16:
		case BATTERY_STATUS__GOOD:		
			//ret = bq24196_float_voltage__good();		
			if(BMT_status.bat_vol >= CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__PRE_HIGH)
			{
				check_term_voltage_count++;
				if(check_term_voltage_count >= 3)
				{
					g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__NORMAL - BQ24196_VFLOAT_STEP_MV;
					check_term_voltage_count = 0;
					pchr_turn_off_charging_bq24196();
					batt_vol_pre_high = 1;
				}
			}
			else {
				g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__NORMAL;
				check_term_voltage_count = 0;
			}
			break;
		case BATTERY_STATUS__PRE_HIGH_TEMP:			
			if(BMT_status.bat_vol >= CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__PRE_WARM)
			{
				check_term_voltage_count++;
				if(check_term_voltage_count >= 3)
				{
					g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__WARM - BQ24196_VFLOAT_STEP_MV;
					check_term_voltage_count = 0;
		//			pchr_turn_off_charging_bq24196();
					batt_vol_pre_high = 1;
				}
			}
			else {
				g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__STANDARD__WARM;
				check_term_voltage_count = 0;
			}
			//ret = bq24196_float_voltage__pre_high_temp();		
			break;
		default:
			break;
	}
	if((!oppo_high_battery_status) && (g_temp_battery_limit_flv_value > CHG_COMPLETE_CHECK_VOLTAGE__NON_HI_BATTERY))		
	{
		g_temp_battery_limit_flv_value = CHG_COMPLETE_CHECK_VOLTAGE__NON_HI_BATTERY;
	}
	printk("bq24196_float_voltage_set-----oppo_high_battery_status = %d,g_temp_battery_limit_flv_value = %d\r\n",oppo_high_battery_status,g_temp_battery_limit_flv_value);
	ret = bq24196_float_voltage_write(g_temp_battery_limit_flv_value);
	if(batt_vol_pre_high ==1){
		msleep(200);
	}
	printk("[%s]check_term_voltage_count = %d,batt_vol_pre_high=%d,BMT_status.bat_vol=%d\n",__func__, check_term_voltage_count, batt_vol_pre_high, BMT_status.bat_vol);
	if(BMT_status.charger_exist == KAL_TRUE){
		bq24196_enable_charging();
	}
	return ret;
}

static int bq24196_usbin_input_current_limit[] = {
    100,    150,    500,    900,
    1200,   1500,   2000,   3000,
};

//#define MAX_COUNT	50
#define SOFT_AICL_VOL	4400
static bool bq24196_input_current_limit_write(int value)
{
	u8 i=0,j=0,k=0;
	kal_int32 chg_vol = 0;

	for (i = ARRAY_SIZE(bq24196_usbin_input_current_limit) - 1; i >= 0; i--) 
	{
		if (bq24196_usbin_input_current_limit[i] <= value) {
			break;
		}
		else if (i == 0) {
		    break;
		}
	}
	#if 1
	//i = i << REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT;
    printk("usb input max current limit=%d setting %02x\n", value, i);
    //j = 2;
    //bq24196_config_interface(REG00_BQ24196_ADDRESS, j<<REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT, REG00_BQ24196_INPUT_CURRENT_LIMIT_MASK, 0);
    //opchg_set_fast_chg_current(chip, chip->max_fast_current[FAST_CURRENT_MAX]);

    for(j = 2; j <= i; j++) {
        bq24196_config_interface(REG00_BQ24196_ADDRESS, j<<REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT, REG00_BQ24196_INPUT_CURRENT_LIMIT_MASK, 0);
		msleep(100);
        //for(k = 0; k < MAX_COUNT; k++) {
		//printk("kong------usb input max current limit=%d setting %02x----------j = %d\n", value, i, j);
            chg_vol = battery_meter_get_charger_voltage();
            //printk("usb input max current limit aicl chg_vol=%d j=%d\n", chg_vol, j);
            if(chg_vol < SOFT_AICL_VOL) {
                if (j > 2) {
                    j = j-1;
                }
                printk("usb input max current limit aicl chg_vol=%d j=%d\n", chg_vol, j);
				input_current_selfadapt = bq24196_usbin_input_current_limit[j];
				printk("[%s]input_current_selfadapt= %d,chg_vol < SOFT_AICL_VOL\n",__func__,input_current_selfadapt);
                bq24196_config_interface(REG00_BQ24196_ADDRESS, j << REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT, REG00_BQ24196_INPUT_CURRENT_LIMIT_MASK, 0);
                //opchg_set_fast_chg_current(chip, chip->max_fast_current[FAST_CURRENT_MIN]);
                return 0;
            }
        //}
    }
#endif
    j = i;
	
    printk("usb input max current limit aicl chg_vol=%d j=%d\n", chg_vol, j);
	input_current_selfadapt = bq24196_usbin_input_current_limit[j];
	printk("[%s]input_current_selfadapt= %d\n",__func__,input_current_selfadapt);
    bq24196_config_interface(REG00_BQ24196_ADDRESS, j << REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT, REG00_BQ24196_INPUT_CURRENT_LIMIT_MASK, 0);
	msleep(1500);
    return 0;
}
static bool bq24196_input_current_limit_standard_set()
{
	if(plug_in_flag_set_charging_current == 1)
	{
		printk("bq24196_input_current_limit_set 2000\r\n");
	return bq24196_input_current_limit_write(2000);
	}
	else
	{
		if(input_current_selfadapt > 1200)
		{
			printk("input_current=1200 from 2000 for OV protect\n");
			bq24196_config_interface(REG00_BQ24196_ADDRESS, 4 << REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT, REG00_BQ24196_INPUT_CURRENT_LIMIT_MASK, 0);
			msleep(50);
		}
		bq24196_input_current_limit_write(2000);
		if(OPPO_LED_ON == 1){
			printk("input_current_limit--------OPPO_LED_ON = %d\r",OPPO_LED_ON);
			if(input_current_selfadapt >= 1200)
			{
				printk("OPPO_LED_ON=1,input_current=1200 from 2000\n");
				return bq24196_config_interface(REG00_BQ24196_ADDRESS, 4 << REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT, REG00_BQ24196_INPUT_CURRENT_LIMIT_MASK, 0);
			}
			else 
				return 0;
		}
	}	
}

static bool bq24196_input_current_limit_standard_set_LED()
{
	u8 i = 0;
	for (i = ARRAY_SIZE(bq24196_usbin_input_current_limit) - 1; i >= 0; i--) 
	{
		if (bq24196_usbin_input_current_limit[i] <= input_current_selfadapt) {
			break;
		}
		else if (i == 0) {
		    break;
		}
	}
	if(plug_in_flag_set_charging_current == 0)
	{
		if(OPPO_LED_ON == 1)
		{	
			printk("input_current_limit--------OPPO_LED_ON = %d\r\n",OPPO_LED_ON);
			if(input_current_selfadapt >= 1200)
			{
				return bq24196_config_interface(REG00_BQ24196_ADDRESS, 4 << REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT, REG00_BQ24196_INPUT_CURRENT_LIMIT_MASK, 0);
			}
			else 
				return 0;
		}
		else if((input_current_selfadapt <= 2000) && (input_current_selfadapt > 1200))
			return bq24196_config_interface(REG00_BQ24196_ADDRESS, i << REG00_BQ24196_INPUT_CURRENT_LIMIT_SHIFT, REG00_BQ24196_INPUT_CURRENT_LIMIT_MASK, 0);
		else 
			return 0;
	}	
	else 
		return 0;
}
static bool bq24196_input_current_limit_non_standard_set()
{
	return bq24196_input_current_limit_write(2000);
}
static bool bq24196_input_current_limit_usb_set()
{

	return	bq24196_input_current_limit_write(500);	
}

bool bq24196_input_current_limit_set()
{
	bool ret = false;
	switch(get_battery_charger_type()){
		case CHARGER_UNKNOWN:		
			break;
		case STANDARD_HOST:
			ret = bq24196_input_current_limit_usb_set();
			break;
		case NONSTANDARD_CHARGER:
		case APPLE_0_5A_CHARGER:
			ret = bq24196_input_current_limit_non_standard_set();
			break;
		case CHARGING_HOST:
		case STANDARD_CHARGER:		
		case APPLE_2_1A_CHARGER:
		case APPLE_1_0A_CHARGER:
			ret = bq24196_input_current_limit_standard_set();
			break;
		default:
			ret = true;
			break;
	}
	return ret;
}

bool bq24196_input_current_limit_set_LED()
{
	bool ret = false;
	switch(get_battery_charger_type()){
		case CHARGER_UNKNOWN:		
			break;
		case STANDARD_HOST:
			break;
		case NONSTANDARD_CHARGER:
		case APPLE_0_5A_CHARGER:
		case CHARGING_HOST:
		case STANDARD_CHARGER:		
		case APPLE_2_1A_CHARGER:
		case APPLE_1_0A_CHARGER:
			ret = bq24196_input_current_limit_standard_set_LED();
			break;
		default:
			ret = true;
			break;
	}
	return ret;
}


static u8 bq24196_charging_mode_get()
{
	int rc;
    u8 reg = 0;
    
    rc = bq24196_read_byte(REG08_BQ24196_ADDRESS, &reg);
    if (rc) {
        printk("Couldn't read STAT_C rc = %d\n", rc);
        return POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
    }
    
    reg &= REG08_BQ24196_CHARGING_STATUS_CHARGING_MASK;
    
    if (reg == REG08_BQ24196_CHARGING_STATUS_FAST_CHARGING) {
        return POWER_SUPPLY_CHARGE_TYPE_FAST;
    }
    else if (reg == REG08_BQ24196_CHARGING_STATUS_PRE_CHARGING) {
        return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
    }
    else {
        return POWER_SUPPLY_CHARGE_TYPE_NONE;
    }
}

static bool bq24196_charging_current_write_pre(int chg_cur)
{
	 u8 value,ret = 0;
    
	value = (chg_cur - BQ24196_MIN_PRE_CURRENT_MA)/BQ24196_PRE_CURRENT_STEP_MA;
	value <<= REG03_BQ24196_PRE_CHARGING_CURRENT_LIMIT_SHIFT;

	ret = bq24196_config_interface(REG03_BQ24196_ADDRESS, value, REG03_BQ24196_PRE_CHARGING_CURRENT_LIMIT_MASK, 0);
	if (false == ret)
		printk("%s:set pre-charging current failed\n", __func__);
	return ret;		
}

static bool bq24196_charging_current_write_fast(int chg_cur)
{	u8 value,ret = 0;
	printk("bq24196_charging_current_write_fast,chg_cur = %d\r\n", chg_cur);
#if 1
	if (chg_cur < BQ24196_MIN_FAST_CURRENT_MA_ALLOWED) {
		if (chg_cur > BQ24196_MIN_FAST_CURRENT_MA_20_PERCENT)
			chg_cur = BQ24196_MIN_FAST_CURRENT_MA_20_PERCENT;
	    chg_cur = chg_cur * 5;
	    value = (chg_cur - BQ24196_MIN_FAST_CURRENT_MA)/BQ24196_FAST_CURRENT_STEP_MA;
	    value <<= REG02_BQ24196_FAST_CHARGING_CURRENT_LIMIT_SHIFT;
	    value = value | REG02_BQ24196_FAST_CHARGING_CURRENT_FORCE20PCT_ENABLE;
	}

#else
	if (chg_cur < BQ24196_MIN_FAST_CURRENT_MA) {
	    chg_cur = chg_cur * 5;
	    value = (chg_cur - BQ24196_MIN_FAST_CURRENT_MA)/BQ24196_FAST_CURRENT_STEP_MA;
	    value <<= REG02_BQ24196_FAST_CHARGING_CURRENT_LIMIT_SHIFT;
	    value = value | REG02_BQ24196_FAST_CHARGING_CURRENT_FORCE20PCT_ENABLE;
	}
#endif
	else {
	    value = (chg_cur - BQ24196_MIN_FAST_CURRENT_MA)/BQ24196_FAST_CURRENT_STEP_MA;
	    value <<= REG02_BQ24196_FAST_CHARGING_CURRENT_LIMIT_SHIFT;
	}
	ret = bq24196_config_interface(REG02_BQ24196_ADDRESS, value, REG02_BQ24196_FAST_CHARGING_CURRENT_LIMIT_MASK | REG02_BQ24196_FAST_CHARGING_CURRENT_FORCE20PCT_MASK, 0);
}

static bool bq24196_set_temrchg_current(int chg_cur)
{
	 u8 value;

	value = (chg_cur - BQ24196_MIN_TERM_CURRENT_MA)/BQ24196_TERM_CURRENT_STEP_MA;
	value <<= REG03_BQ24196_TERM_CHARGING_CURRENT_LIMIT_SHIFT;
	printk("bq24196_set_temrchg_current value=%d\n", value);

	return bq24196_config_interface(REG03_BQ24196_ADDRESS, value, REG03_BQ24196_TERM_CHARGING_CURRENT_LIMIT_MASK, 0);
	
}

static bool bq24196_charging_current_write_fast_ForOvp(int chg_cur)
{
	bool ret = false;
	
	ret = bq24196_charging_current_write_fast(chg_cur);
	return ret;
}

void bq24196_charging_CurrentForOvp(void)
{
	bool ret = false;
	switch(get_battery_temp_status()){
		case BATTERY_STATUS__INVALID:
		case BATTERY_STATUS__REMOVED:
		case BATTERY_STATUS__LOW_TEMP:
		case BATTERY_STATUS__HIGH_TEMP:		
		case BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0:			
		case BATTERY_STATUS__PRE_LOW_TEMP_0_5:
		case BATTERY_STATUS__PRE_LOW_TEMP_5_12:
			return ret;
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_12_16:
		case BATTERY_STATUS__GOOD:
		case BATTERY_STATUS__PRE_HIGH_TEMP:	
			bq24196_charging_current_write_fast_ForOvp(1000);
			msleep(80);
			bq24196_charging_current_write_fast_ForOvp(512);
			msleep(50);
			break;
		default:
			bq24196_charging_current_write_fast_ForOvp(512);
			msleep(50);
			break;
	}
}


static bool bq24196_charging_current_write(int chg_cur)
{
	bq24196_charging_mode chg_mode = bq24196_charging_mode_get();
	if (CHARGING_MODE__PRE_CHG == chg_mode)
		return bq24196_charging_current_write_pre(chg_cur);
	else if(chg_mode == CHARGING_MODE__FAST_CHG )
		return bq24196_charging_current_write_fast(chg_cur);
	else{
		printk("%s:not charging\n", __func__);
		return true;
	}
}


static bool bq24196_charging_current_set_trick()
{
	printk("%s:trick charging no need to do anything for software \n", __func__);
	return true;
}

static bool bq24196_charging_current_set_pre()
{
	//return bq24196_charging_current_write(USB__PRE_CHG__NORMAL_CURRNET);
	g_temp_battery_limit_pcc_value = PRE_CHG_CURRENT__250MA;
	//printk("bq24196_charging_current_set_pre-------PRE_CHG_CURRENT__250MA = %d\r\n",PRE_CHG_CURRENT__250MA);
	return bq24196_charging_current_write_pre(g_temp_battery_limit_pcc_value);
}

static bool bq24196_charging_current_set_fast_usb__pre_low_temp2()
{
	return bq24196_charging_current_write(USB__FAST_CHG__LITTLE_COLD_CURRNET);
}

static bool bq24196_charging_current_set_fast_usb__pre_low_temp()
{
	return bq24196_charging_current_write(USB__FAST_CHG__COOL_CURRNET);
}

static bool bq24196_charging_current_set_fast_usb__good()
{
	return bq24196_charging_current_write(USB__FAST_CHG__NORMAL_CURRNET);
}

static bool bq24196_charging_current_set_fast_usb__pre_high_temp()
{
	return bq24196_charging_current_write(USB__FAST_CHG__WARM_CURRNET);
}


static bool bq24196_charging_current_set_fast_usb()
{
	bool ret = false;
	switch(get_battery_temp_status()){
		case BATTERY_STATUS__INVALID:
		case BATTERY_STATUS__REMOVED:
		case BATTERY_STATUS__LOW_TEMP:
		case BATTERY_STATUS__HIGH_TEMP:
			return ret;
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0:
			g_temp_battery_limit_fcc_value = USB__FAST_CHG__PRE_COLD_CURRNET_NEG3_0;			
			//ret = bq24196_charging_current_set_fast_usb__pre_low_temp2();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_0_5:
		case BATTERY_STATUS__PRE_LOW_TEMP_5_12:
			g_temp_battery_limit_fcc_value = USB__FAST_CHG__PRE_COLD_CURRNET_0_12;			
			//ret = bq24196_charging_current_set_fast_usb__pre_low_temp2();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_12_16:
			g_temp_battery_limit_fcc_value = USB__FAST_CHG__PRE_COLD_CURRNET_12_16;
			//ret = bq24196_charging_current_set_fast_usb__pre_low_temp();
			break;
		case BATTERY_STATUS__GOOD:
			g_temp_battery_limit_fcc_value = USB__FAST_CHG__NORMAL_CURRNET;
			//ret = bq24196_charging_current_set_fast_usb__good();		
			break;
		case BATTERY_STATUS__PRE_HIGH_TEMP:
			g_temp_battery_limit_fcc_value = USB__FAST_CHG__PRE_WARM_CURRNET;
			//ret = bq24196_charging_current_set_fast_usb__pre_high_temp();		
			break;
		default:		
			break;
	}
	ret = bq24196_charging_current_write_fast(g_temp_battery_limit_fcc_value);
	return ret;
}
static bool bq24196_charging_current_set_fast_standard__pre_low_temp2()
{
	return bq24196_charging_current_write(USB__FAST_CHG__LITTLE_COLD_CURRNET);
}

static bool bq24196_charging_current_set_fast_standard__pre_low_temp()
{
	return bq24196_charging_current_write(USB__FAST_CHG__COOL_CURRNET);
}

static bool bq24196_charging_current_set_fast_standard__good()
{
	return bq24196_charging_current_write(FAST_CHG_CURRENT__1300MA);
}

static bool bq24196_charging_current_set_fast_standard__pre_high_temp()
{
	return bq24196_charging_current_write(USB__FAST_CHG__WARM_CURRNET);
}

static bool bq24196_charging_current_set_fast_standard()
{
	bool ret = false;
	switch(get_battery_temp_status()){
		case BATTERY_STATUS__INVALID:
		case BATTERY_STATUS__REMOVED:
		case BATTERY_STATUS__LOW_TEMP:
		case BATTERY_STATUS__HIGH_TEMP:
			return ret;
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0:
			g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_NEG3_0;
			//ret = bq24196_charging_current_set_fast_standard__pre_low_temp2();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_0_5:
			g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12_PRE;
			//ret = bq24196_charging_current_set_fast_standard__pre_low_temp2();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_5_12:
			g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12;
#ifdef NEW_CHARGER_STARDARD_V2_8
			if(BMT_status.bat_vol >= 4180){	
				vol_count_temp5_12++;
				if(vol_count_temp5_12 > 2){
					vol_count_temp5_12 = 0;
					vol_count_flag = 1;
					g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12_PRE;
				}
				else if(vol_count_flag == 1)
					g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12_PRE;
			}
			else if(vol_count_flag == 1){
				vol_count_temp5_12 = 0;
				if(BMT_status.bat_vol >= 4000)
					g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12_PRE;
				else {
					vol_count_flag = 0;
					g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12;
				}
			}
			else {
				vol_count_temp5_12 = 0;
				g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_0_12;
			}

#endif
			//ret = bq24196_charging_current_set_fast_standard__pre_low_temp();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_12_16:
			g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_COLD_CURRNET_12_16;
			//ret = bq24196_charging_current_set_fast_standard__pre_low_temp();
			break;
		case BATTERY_STATUS__GOOD:
			g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__NORMAL_CURRNET;
			//ret = bq24196_charging_current_set_fast_standard__good();		
			break;
		case BATTERY_STATUS__PRE_HIGH_TEMP:
			//ret = bq24196_charging_current_set_fast_standard__pre_high_temp();	
			g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__PRE_WARM_CURRNET;
			
			#ifdef OPPO_LED_CURRENT_COMPENSATION
			if(OPPO_LED_ON == 1)
			{
				g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__COOL_CURRNET;
				printk("OPPO_LED_ON-----------------OPPO_LED_ON = %d\r\n",OPPO_LED_ON);
			}
			#endif
			
			break;
		default:
			g_temp_battery_limit_fcc_value = STANDARD__FAST_CHG__COOL_CURRNET;
			break;
	}
	
	#ifdef VIDEO_MODE_CHARGING
	if((OV5647MIPI_get_Video_Mode() || (!oppo_high_battery_status)) && (g_temp_battery_limit_fcc_value > USB__FAST_CHG__NORMAL_CURRNET))
	{		
		g_temp_battery_limit_fcc_value = USB__FAST_CHG__NORMAL_CURRNET;
		printk("bq24196_charging_current_set_fast_standard-----------------video_mode = %d, oppo_high_battery_status = %d\r\n", OV5647MIPI_get_Video_Mode(),oppo_high_battery_status);
	}
	#endif
	
	ret = bq24196_charging_current_write_fast(g_temp_battery_limit_fcc_value);
	return ret;
}


static bool bq24196_charging_current_set_fast()
{
	bool ret = false;
	switch(get_battery_charger_type()){
		case CHARGER_UNKNOWN:
		case STANDARD_HOST:			
			ret = bq24196_charging_current_set_fast_usb();
			break;
		case APPLE_0_5A_CHARGER:
		case NONSTANDARD_CHARGER:
		case APPLE_2_1A_CHARGER:
		case APPLE_1_0A_CHARGER:
		case STANDARD_CHARGER:
		case CHARGING_HOST:
			ret = bq24196_charging_current_set_fast_standard();
			break;
		default:
			break;
	}
	return ret;
}




static bool bq24196_charging_current_set_taper_usb__pre_low_temp2()
{
	return bq24196_charging_current_write(USB__FAST_CHG__LITTLE_COLD_CURRNET);
}

static bool bq24196_charging_current_set_taper_usb__pre_low_temp()
{
	return bq24196_charging_current_write(USB__FAST_CHG__COOL_CURRNET);
}

static bool bq24196_charging_current_set_taper_usb__good()
{
	return bq24196_charging_current_write(USB__FAST_CHG__NORMAL_CURRNET);
}

static bool bq24196_charging_current_set_taper_usb__pre_high_temp()
{
	return bq24196_charging_current_write(USB__FAST_CHG__WARM_CURRNET);
}

static bool bq24196_charging_current_set_taper_usb()
{
	bool ret = false;
	switch(get_battery_temp_status()){
		case BATTERY_STATUS__INVALID:
		case BATTERY_STATUS__REMOVED:
		case BATTERY_STATUS__LOW_TEMP:
		case BATTERY_STATUS__HIGH_TEMP:
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0:
			ret = bq24196_charging_current_set_taper_usb__pre_low_temp2();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_0_5:
		case BATTERY_STATUS__PRE_LOW_TEMP_5_12:
			ret = bq24196_charging_current_set_taper_usb__pre_low_temp();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_12_16:
			ret = bq24196_charging_current_set_taper_usb__pre_low_temp();
			break;
		case BATTERY_STATUS__GOOD:
			ret = bq24196_charging_current_set_taper_usb__good();		
			break;
		case BATTERY_STATUS__PRE_HIGH_TEMP:
			ret = bq24196_charging_current_set_taper_usb__pre_high_temp();		
			break;
		default:
			break;
	}
	return ret;
}

static bool bq24196_charging_current_set_taper_standard__pre_low_temp2()
{
	return bq24196_charging_current_write(USB__FAST_CHG__LITTLE_COLD_CURRNET);
}

static bool bq24196_charging_current_set_taper_standard__pre_low_temp()
{
	return bq24196_charging_current_write(USB__FAST_CHG__COOL_CURRNET);
}

static bool bq24196_charging_current_set_taper_standard__good()
{
	return bq24196_charging_current_write(FAST_CHG_CURRENT__1300MA);
}

static bool bq24196_charging_current_set_taper_standard__pre_high_temp()
{
	return bq24196_charging_current_write(USB__FAST_CHG__WARM_CURRNET);
}

static bool bq24196_charging_current_set_taper_standard()
{
	bool ret = false;
	switch(get_battery_temp_status()){
		case BATTERY_STATUS__INVALID:
		case BATTERY_STATUS__REMOVED:
		case BATTERY_STATUS__LOW_TEMP:
		case BATTERY_STATUS__HIGH_TEMP:
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0:
			ret = bq24196_charging_current_set_taper_standard__pre_low_temp2();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_0_5:
		case BATTERY_STATUS__PRE_LOW_TEMP_5_12:
			ret = bq24196_charging_current_set_taper_standard__pre_low_temp();
			break;
		case BATTERY_STATUS__PRE_LOW_TEMP_12_16:
			ret = bq24196_charging_current_set_taper_standard__pre_low_temp();
			break;
		case BATTERY_STATUS__GOOD:
			ret = bq24196_charging_current_set_taper_standard__good();		
			break;
		case BATTERY_STATUS__PRE_HIGH_TEMP:
			ret = bq24196_charging_current_set_taper_standard__pre_high_temp();		
			break;
		default:
			break;
	}
	return ret;
}

static bool bq24196_charging_current_set_taper()
{
	bool ret = false;
	switch(get_battery_charger_type()){
		case CHARGER_UNKNOWN:		
		case APPLE_0_5A_CHARGER:
		case NONSTANDARD_CHARGER:
		case STANDARD_HOST:			
			ret = bq24196_charging_current_set_taper_usb();
			break;
		case APPLE_2_1A_CHARGER:
		case APPLE_1_0A_CHARGER:
		case STANDARD_CHARGER:
		case CHARGING_HOST:
			ret = bq24196_charging_current_set_taper_standard();
			break;
		default:
			break;
	}
	return ret;
}



bool bq24196_charging_current_set()
{
	bool ret = false;
	#if 0
	switch(bq24196_charging_mode_get()){
		case CHARGING_MODE__INVALID:
			break;
		//case CHARGING_MODE__TRICK_CHG:
		//	ret = bq24196_charging_current_set_trick();
		//	break;
		case CHARGING_MODE__PRE_CHG:
			ret = bq24196_charging_current_set_pre();
			break;
		case CHARGING_MODE__FAST_CHG:
			ret = bq24196_charging_current_set_fast();
			break;
		//case CHARGING_MODE__TAPER_CHG:
		//	ret = bq24196_charging_current_set_taper();
		//	break;
		default:
			break;
	}
	#endif
	ret = bq24196_charging_current_set_fast();
	return ret;
}


bool bq24196_enable_charging(void)
{
	 int rc;
    rc = bq24196_config_interface(REG01_BQ24196_ADDRESS, REG01_BQ24196_CHARGING_ENABLE, REG01_BQ24196_CHARGING_MASK, 0);
    if (rc < 0) {
		pr_err("Couldn'tbq24196_enable_charging rc = %d\n", rc);
	}
	
	return rc;
}

bool bq24196_disable_charging(void)
{
	 int rc;
    rc = bq24196_config_interface(REG01_BQ24196_ADDRESS, REG01_BQ24196_CHARGING_DISABLE, REG01_BQ24196_CHARGING_MASK, 0);
    if (rc < 0) {
		pr_err("Couldn't bq24196_disable_charging  rc = %d\n", rc);
	}
	
	return rc;	
}

int bq24196_set_enable_volatile_writes()
{
    int rc=0;

	//need do nothing
	
    return rc;
}

int bq24196_set_prechg_current(int ipre_mA)
{
    u8 value;
    
	value = (ipre_mA - BQ24196_MIN_PRE_CURRENT_MA)/BQ24196_PRE_CURRENT_STEP_MA;
	value <<= REG03_BQ24196_PRE_CHARGING_CURRENT_LIMIT_SHIFT;

	return bq24196_config_interface(REG03_BQ24196_ADDRESS, value, REG03_BQ24196_PRE_CHARGING_CURRENT_LIMIT_MASK, 0);
}

int bq24196_set_rechg_voltage(int recharge_mv)
{
   int reg,rc=0;
   /* set recharge voltage*/
    if (recharge_mv >= 300) {
        reg = REG04_BQ24196_RECHARGING_THRESHOLD_VOL_300MV;
    }
	else 
	{
        reg = REG04_BQ24196_RECHARGING_THRESHOLD_VOL_100MV;
    }
    rc = bq24196_config_interface(REG04_BQ24196_ADDRESS, reg, REG04_BQ24196_RECHARGING_THRESHOLD_VOL_MASK, 0);
    if (rc) {
        printk("Couldn't set recharging threshold rc = %d\n", rc);
        return rc;
    }
}

int bq24196_set_complete_charge_timeout(int val)
{
    int rc = 0;
    
    if (val == OVERTIME_AC){
        val = REG05_BQ24196_CHARGING_SAFETY_TIME_ENABLE | REG05_BQ24196_FAST_CHARGING_TIMEOUT_8H;
    }
    else if (val == OVERTIME_USB){
        val = REG05_BQ24196_CHARGING_SAFETY_TIME_ENABLE | REG05_BQ24196_FAST_CHARGING_TIMEOUT_12H;
    }
    else {
        val = REG05_BQ24196_CHARGING_SAFETY_TIME_DISABLE | REG05_BQ24196_FAST_CHARGING_TIMEOUT_8H;
    }
    
    rc = bq24196_config_interface(REG05_BQ24196_ADDRESS, val, REG05_BQ24196_CHARGING_SAFETY_TIME_MASK | REG05_BQ24196_FAST_CHARGING_TIMEOUT_MASK, 0);
    if (rc < 0) {
        printk("Couldn't complete charge timeout rc = %d\n", rc);
    }
    
    return rc;
}

int bq24196_set_wdt_timer(int reg)
{
    int rc = 0;
    
    rc = bq24196_config_interface(REG05_BQ24196_ADDRESS, reg, REG05_BQ24196_I2C_WATCHDOG_TIME_MASK,0);
    
    return rc;
}


int bq24196_kick_wdt(void)
{
	 int rc = 0;
    
    rc = bq24196_config_interface(REG01_BQ24196_ADDRESS, REG01_BQ24196_WDT_TIMER_RESET, REG01_BQ24196_WDT_TIMER_RESET_MASK,0);
    
    return rc;
}

bool bq24196_hardware_init(void)
{
	int rc;
    u8 reg = 0;
	
	/* enable write permission to config registers*/
	rc = bq24196_set_enable_volatile_writes();
	
	bq24196_set_complete_charge_timeout(OVERTIME_DISABLED);
	/*max input current limit 0x00 3~6*/
	//bq24196_input_current_limit_set();
	
	/* set the fast charge current limit 0x02 2~7*/
	//bq24196_charging_current_write(BQ24196_MIN_FAST_CURRENT_MA);//512
	//bq24196_charging_current_set();
	
	/* set the float voltage 0x04 4~7*/
	//bq24196_float_voltage_write(BQ24196_MAX_FLOAT_MV);//4.4
	bq24196_float_voltage_set();
	
    /* set pre-charging current 0x03 4~7*/
    rc = bq24196_set_prechg_current(300);
	
    /* set iterm current 0x03 0~3*/
    rc =bq24196_set_temrchg_current(256);
    
	/* set rechg voltage 0x04 0*/
    bq24196_set_rechg_voltage(200);
    
    /* enable/disable charging  0x01 4*/
    //bq24196_enable_charging();
	
    /* wdt timeout setting 0x05 4~5*/
    bq24196_set_wdt_timer(REG05_BQ24196_I2C_WATCHDOG_TIME_40S);	

	printk("bq24196_hardware_init---------------------------end\r\n");
	return true;
}

int bq24196_check_is_charging()
{
    int rc;
    u8 reg = 0;
    
    rc = bq24196_read_byte(REG01_BQ24196_ADDRESS, &reg);
    if (rc) {
        printk("Couldn't read STAT_C rc = %d\n", rc);
        return 0;
    }
    
    return (reg & REG01_BQ24196_CHARGING_ENABLE) ? 1 : 0;
}

u8 bq24196_registers_read_full()
{
	int rc;
    u8 reg = 0;
    
    rc = bq24196_read_byte(REG08_BQ24196_ADDRESS, &reg);
    if (rc) {
        printk("Couldn't read STAT_C rc = %d\n", rc);
        return 0;
    }
    //printk("bq24196_registers_read_full-------------------------------reg = %d\r\n",reg);
    return ((reg & REG08_BQ24196_CHARGING_STATUS_CHARGING_MASK) == REG08_BQ24196_CHARGING_STATUS_TERM_CHARGING) ? 1 : 0;
}

u8 bq24196_registers_read_charging_fault()
{
	int rc;
    u8 reg = 0;
    
    rc = bq24196_read_byte(REG09_BQ24196_ADDRESS, &reg);
    if (rc) {
        printk("Couldn't read STAT_C rc = %d\n", rc);
        return 0;
    }
    
    return (reg & REG09_BQ24196_CHARGING_MASK);
}


void bq24196_reset_charger()
{
	int rc;
    
    rc = bq24196_config_interface(REG01_BQ24196_ADDRESS, REG01_BQ24196_REGISTER_RESET, REG01_BQ24196_REGISTER_RESET_MASK, 0);
	pr_err("bq24196_reset_charger rc = %d\n", rc);
    if (rc < 0) {
		pr_err("Couldn't bq24196_reset_charger rc = %d\n", rc);
	}
	
	return rc;
}

void bq24196_suspend_charger()
{
	int rc;
    BMT_status.charger_suspend = true;
    rc = bq24196_config_interface(REG00_BQ24196_ADDRESS, REG00_BQ24196_SUSPEND_MODE_ENABLE, REG00_BQ24196_SUSPEND_MODE_MASK, 0);
	pr_err("bq24196_suspend_charger rc = %d\n", rc);
    if (rc < 0) {
		pr_err("Couldn't bq24196_suspend_charger rc = %d\n", rc);
	}
	
	return rc;
}
void bq24196_unsuspend_charger()
{
	int rc;
    BMT_status.charger_suspend = false;
    rc = bq24196_config_interface(REG00_BQ24196_ADDRESS, REG00_BQ24196_SUSPEND_MODE_DISABLE, REG00_BQ24196_SUSPEND_MODE_MASK, 0);
	pr_err("bq24196_unsuspend_charger rc = %d\n", rc);
    if (rc < 0) {
		pr_err("Couldn't bq24196_unsuspend_charger rc = %d\n", rc);
	}	
	return rc;
}
/*for init functions end*/

u8 bq24196_get_suspend_status()
{
	bool ret = false;
	u8 value = 0;
	ret = bq24196_read_interface(REG00_BQ24196_ADDRESS, &value,0x80, 0);
	return value;
/*for init functions end*/
}
int bq24196_otg_enable()
{
    int rc = 0;   
    rc = bq24196_config_interface(REG01_BQ24196_ADDRESS, REG01_BQ24196_OTG_ENABLE, REG01_BQ24196_OTG_MASK, 0);
    if (rc) {
        printk("Couldn't enable  OTG mode rc=%d\n", rc);
    }
	else
	{
		printk("bq24196_otg_enable rc=%d\n", rc);
	}
    
	return rc;
}

int bq24196_otg_disable()
{
    int rc = 0;
    
    rc = bq24196_config_interface(REG01_BQ24196_ADDRESS, REG01_BQ24196_OTG_DISABLE, REG01_BQ24196_OTG_MASK, 0);
    if (rc) {
        printk("Couldn't disable OTG mode rc=%d\n", rc);
	}
	else
	{
		printk("bq24196_otg_disable rc=%d\n", rc);
	}
	
    return rc;
}

int bq24196_check_otg_is_enable()
{
    int rc = 0;
    u8 reg = 0;
    
    rc = bq24196_read_byte(REG01_BQ24196_ADDRESS, &reg);
    if (rc) {
        printk("Couldn't read OTG enable bit rc=%d\n", rc);
        return rc;
    }
    
    return (reg & REG01_BQ24196_OTG_MASK) ? 1 : 0;
}


bool bq24196_start_charging()
{
	bool ret = false;
	int batt_vol;
	int batt_temp;

/*if charger suspended do nothing*/
	if (true == bq24196_get_suspend_status()){
		printk("%s:in suspend mode ,no charging !\n", __func__);
		return false;
	}
	bq24196_hardware_init();
	bq24196_charging_current_set();
	bq24196_enable_charging();

}

bool bq24196_stop_charging()
{
	bool ret = false;
	ret = bq24196_disable_charging();
	return ret;
}
static void bq24196_dump_registers()
{
	int rc;
	u8 reg, addr;
	
	// read config register
	for (addr = 0; addr <= BQ24196_LAST_CNFG_REG; addr++) {
		rc = bq24196_read_byte(addr, &reg);
		if (rc) {
			 printk("Couldn't read 0x%02x rc = %d\n", addr, rc);
		}
		else {
			printk("bq24196_read_reg 0x%02x = 0x%02x\n", addr, reg);
		}
	}
	// read status register   
	for (addr = BQ24196_FIRST_STATUS_REG; addr <= BQ24196_LAST_STATUS_REG; addr++) {
		rc = bq24196_read_byte(addr, &reg);
		if (rc) {
			  printk("Couldn't read 0x%02x rc = %d\n", addr, rc);
		}
		else {
			 printk("bq24196_read_reg 0x%02x = 0x%02x\n", addr, reg);
		}
	}
	// read command register	   
	for (addr = BQ24196_FIRST_CMD_REG; addr <= BQ24196_LAST_CMD_REG; addr++) {
		rc = bq24196_read_byte(addr, &reg);
		if (rc) {
			 printk("Couldn't read 0x%02x rc = %d\n", addr, rc);
		}
		else {
			printk("bq24196_read_reg 0x%02x = 0x%02x\n", addr, reg);
		}
	}
}

void bq24196_dumps()
{
	//bq24196_dump_status();
	bq24196_dump_registers();
}

void bq24196_power(int on)
{
/*
    if(on)
    {
            hwPowerOn(MT6323_POWER_LDO_VCAMD,VOL_1800,"battery"); 
    	    udelay(10);
		    hwPowerOn(MT6323_POWER_LDO_VGP1,VOL_2800,"battery");
    }
    else
    {
            hwPowerDown(MT6323_POWER_LDO_VGP1,"battery");
		    udelay(10);
		    hwPowerDown(MT6323_POWER_LDO_VCAMD,"battery");
    }
*/
}

static void bq24196_reset(struct i2c_client *client)
{
	bq24196_reset_charger();
}

static int bq24196_driver_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{         
    strcpy(info->type, "bq24196_i2c");                                                         

	printk("[bq24196_driver_detect] \n");
	
    return 0;                                                                                       
}

static int bq24196_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
	struct class_device *class_dev = NULL;
    int err=0; 

    printk("[bq24196_driver_probe] \n");

	if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }	
    memset(new_client, 0, sizeof(struct i2c_client));

	new_client = client;	
	chargin_hw_init_done_bq24196 = 1;
    bq24196_power(1);
	bq24196_hardware_init();
    return 0;                                                                                       

exit_kfree:
    kfree(new_client);
    bq24196_power(0);
exit:
    return err;

}
/**********************************************************
  *
  *   [platform_driver API] 
  *
  *********************************************************/
static __init  bq24196_subsys_init(void)
{	
	int ret=0;
	
	printk("[bq24196_init] init start\n");\
	ret = i2c_register_board_info(2, &i2c_bq24196, 1);

	if(ret)
	{
       printk("bq24196 i2c device add  fail\n");
	}
	if(i2c_add_driver(&bq24196_i2c_driver)!=0)
	{
		printk("[bq24196_init] failed to register bq24196 i2c driver.\n");
	}
	else
	{
		printk("[bq24196_init] Success to register bq24196 i2c driver.\n");
	}

	return 0;		
}

static void  bq24196_exit(void)
{
	i2c_del_driver(&bq24196_i2c_driver);
}
/*----------------------------------------------------------------------------*/
/*
static int bq24196_platform_driver_probe(struct platform_device *pdev) 
{
    int ret=0;
    printk("%s\n",__func__);
	ret = bq24196_init();
	if(ret)
	{
	    printk("bq24196_init error!\n");
	}

	return 0;
	
}

static int bq24196_platform_driver_remove(struct platform_device *pdev)
{
    int ret=0;
    printk("%s\n",__func__);
	bq24196_exit();

	return 0;
}

static int bq24196_platform_suspend(struct platform_device *dev, pm_message_t state)	
{
	bq24196_power(0);
    printk("%s,power down i2c\n",__func__);
    return 0;
}

static int bq24196_platform_resume(struct platform_device *dev, pm_message_t state)	
{
	bq24196_power(1);
    printk("%s,power on i2c\n",__func__);
    return 0;
}

static struct platform_driver bq24196_platform_driver = {
	.probe      = bq24196_platform_driver_probe,
	.remove     = bq24196_platform_driver_remove,  
	.suspend    = bq24196_platform_suspend,
	.resume     = bq24196_platform_resume,
	.driver     = {
		.name  = "bq24196",
		.owner = THIS_MODULE,
	}
};


static int __init bq24196_platform_driver_init(void)
{
	printk("%s\n",__func__);
	i2c_register_board_info(1, &i2c_bq24196, 1);
    
	if(platform_driver_register(&bq24196_platform_driver))
	{
		printk("%s failed to register driver",__func__);
		return -ENODEV;
	}
	return 0;    
}

static void __exit bq24196_platform_driver_deinit(void)
{
	printk("%s\n",__func__);
	platform_driver_unregister(&bq24196_platform_driver);

}
*/
/*----------------------------------------------------------------------------*/

/*
module_init(bq24196_platform_driver_init);
module_exit(bq24196_platform_driver_deinit);
*/
subsys_initcall(bq24196_subsys_init);
//module_exit(bq24196_exit);
//MODULE_DESCRIPTION("Driver for bq24196 charger chip");
//MODULE_LICENSE("GPL v2");

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C bq24196 Driver");
MODULE_AUTHOR("James lu<james.lo@mediatek.com>");


