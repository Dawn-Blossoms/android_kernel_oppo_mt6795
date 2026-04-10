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
** 1.1          2012-2-01       dengzy@oppo.com             rm PMIC_IMM_GetOneChannelValue() log
   1.2          2012-2-01       dengzy@oppo.com             upmu_chr_baton_tdet_en(1) for vbat temp
   1.3          2012-7-09       Fanhong.Kong@ProDrv.CHG     add for charger exist USB/phone/URT4/URT1 converter switch in DVT1
   1.4          2012-7-26       Fanhong.Kong@ProDrv.CHG     modified 12015 ADC calibration
   1.5          2012-6-14       Fanhong.Kong@ProDrv.CHG     add for 6 hourse  overtime
   1.6          2012-3-08       Fanhong.Kong@ProDrv.CHG     Add for and low battery poweroff and critical low battery in sleep
   1.7          2012-7-13       Jinquan.Lin@BasicDrv.PM     Add for turning off VUSB
   1.8          2012-5-26       Fanhong.Kong@ProDrv.CHG     Add for client init before bat_thread to avoid null pointer
   1.9          2012-5-19       Fanhong.Kong@BaiscDrv.CHG   modified 6 hours overtime all temp
   2.1          2012-8-08       Fanhong.Kong@BaiscDrv.CHG   modified charger detect 
************************************************************************************************************/

    

#include <linux/init.h>        /* For init/exit macros */
#include <linux/module.h>      /* For MODULE_ marcros  */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/xlog.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <linux/suspend.h>

#include <asm/scatterlist.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/system.h>
#include <mach/mt_sleep.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpt.h>
#include <mach/mt_boot.h>

#include <cust_charging.h>  
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>
#include <mach/charging.h>
#include <mach/battery_common.h>
#include <mach/battery_meter.h>
#include "cust_battery_meter.h"
#include "cust_charging.h"
#include "cust_pmic.h"
#include <mach/mt_boot.h>
#include "mach/mtk_rtc.h"

#ifdef X2_CHARGING_STANDARD_SUPPORT
#include "x2_charging.h"
#endif

#include "bq2202a.h"
#include "mt65xx_battery.h"
#include "oppo_bq24196.h"
#include "oppo_bq27541.h"
#include "oppo_vooc.h"
#include "cust_battery.h" 
#include <linux/gpio.h>
#include <mach/mt_gpio.h>
#include <linux/compat.h>

extern  int primary_display_suspend(void);

//#define NEW_CHARGE_FULL_STOP
#ifdef NEW_CHARGE_FULL_STOP
static kal_uint8 bq24156a_post_vol_count=0;
static kal_uint8 bq24156a_post_status=0;
static kal_uint16 bq24156a_post_vol_time=0;
static kal_uint8 bq24156a_post_current_count=0;
#endif

#define BQ24196_STATUS_READY          	0X00
#define BQ24196_STATUS_IN_PROGESS     	0X01
#define BQ24196_STATUS_DONE           	0X02
#define BQ24196_STATUS_FAULT          	0X03
#define BQ24196_CHR_CCCV   				0X01
#define BQ24196_CHR_FULL   				0X02
#define BQ24196_CHR_FAIL    			0X03

//#define OPPO_BATTERY_ENCRPTION
kal_bool oppo_high_battery_status = 1;
kal_uint8 oppo_check_ID_status = 0;

#ifdef OPPO_BATTERY_ENCRPTION
kal_uint8 oppo_high_battery_check_counts = 0;
kal_bool oppo_battery_status_init_flag = 0;
static void oppo_battery_status_init(void);
static void oppo_battery_status_check(void);
#endif

#define BATTERY_FULLCAPACITY	 4100

#define SYSTEM_OFF_CRITICAL_VOLTAGE         3500
static int g_bimax_dischg=0;
static int g_soc_sync_time=0;
int gSyncPercentage=0;
int g_Calibration_FG=0;

int gADC_BAT_SENSE_temp=0;
int gADC_I_SENSE_temp=0;
int gADC_I_SENSE_offset=0;

int gBAT_counter_15=1;
int gBAT_counter_off=1;
int gFG_can_reset_flag = 1;

#define DEFAULT_SOC_SYNC_UP_TIME_10  						1
#define DEFAULT_SOC_SYNC_UP_TIME_60  						11
#define DEFAULT_SOC_SYNC_DOWN_TIME_NORMAL 					8
#define DEFAULT_SOC_SYNC_DOWN_TIME_FAST_300 				59
#define DEFAULT_SOC_SYNC_DOWN_TIME_FAST_150 				29
#define DEFAULT_SOC_SYNC_DOWN_TIME_FAST_40 					7
#define DEFAULT_SOC_SYNC_DOWN_TIME_FAST_30 					5
#define DEFAULT_SOC_SYNC_DOWN_TIME_FAST_20 					3

static int g_soc_sync_down_times = DEFAULT_SOC_SYNC_DOWN_TIME_NORMAL;
static int g_soc_sync_up_times = DEFAULT_SOC_SYNC_UP_TIME_10;
#define SHUTDOWN_INSTANT_LOW_POWER  		3100 
#define SHUTDOWN_AVERAGE_LOW_POWER  		3300//Add for low battery in sleep
#define SHUTDOWN_WAIT_LOW_POWER				3400
extern  wake_reason_t slp_wake_reason;
extern struct bms_bq27541 *bq27541_di;

int fastchg_present_flag = 0;
int fastchg_present_wait_count = 0;

extern int get_rtc_spare_fg_value(void);
extern int set_rtc_spare_fg_value(int val);
int g_rtc_soc = 0;
int g_point_by_v = 0;
int g_rtc_rst_flag = 0;

static int vchg_cnt=0;
#define NEW_CHGING_TIME
#ifdef NEW_CHGING_TIME
UINT32  max_charging_time_kernel = 10*60*60;//s
#endif
#define RECHG_SOC	96

int g_HW_Charging_Done = 0;
int g_Charging_Over_Time = 0;

#define IPOD_CHARGING_STATUS
#ifdef IPOD_CHARGING_STATUS
extern volatile int OPPO_LED_ON;
static kal_bool g_ipod_charging = KAL_FALSE;
static kal_bool g_ipod_init_done = KAL_FALSE;
#endif

static unsigned short batteryVoltageBuffer[BATTERY_AVERAGE_SIZE];
static unsigned short batteryCurrentBuffer[BATTERY_AVERAGE_SIZE];
static unsigned short batterySOCBuffer[BATTERY_AVERAGE_SIZE];
static int batteryIndex = 0;
static int batteryVoltageSum = 0;
static int batteryCurrentSum = 0;
static int batterySOCSum = 0;

kal_bool g_bat_full_user_view = KAL_FALSE;
kal_bool g_Battery_Fail = KAL_FALSE;
static kal_bool TempIsChanged = KAL_FALSE;
BOOL b_notify_first=TRUE;
int g_NotifyFlag = 0;


static UINT32 check_charger_off_vol = 5000;
#define OPPO_CHARGER_RESUME
#ifdef OPPO_CHARGER_RESUME
#define OVER_CHARGER_RESUME_COUNTS 3
UINT32 over_charger_error_count = 0;
#endif

#define OPPO_USE_FAST_CHARGER_RESET_MCU
#ifdef OPPO_USE_FAST_CHARGER_RESET_MCU
kal_bool fast_charger_reset_sign;
int fast_charger_reset_count;
#endif

#define OPPO_MMI_FASTCHARGER

int plug_in_flag_set_charging_current = 0;
static int pchr_turn_off_plug_out = 0;
static int plug_in_flag_selfadapt = 0;//PengNan@Drv.CHG add for insure selfadpat when plug in 2015/06/17
static int selfadapt_delay_count = 0;//PengNan@Drv.CHG add for accumulate selfadpt time 2015/06/17
int plug_in_Temp_abnomal = 0;

static int get_current_offset_flag=1;
int get_pmic_flag=0;
static int charge_on_cnt=0;
int gForceADCsolution=0;

int g_chr_event = 0;
int bat_volt_cp_flag = 0;
int bat_volt_check_point = 0;

extern int gFG_15_vlot;
extern kal_int32 oam_v_ocv_2;
extern kal_int32 oam_v_ocv_1;
extern kal_int32 gFG_current;
extern kal_int32 gFG_voltage;
extern kal_int32 gFG_DOD0;
extern kal_int32 gFG_DOD1;
extern kal_int32 gFG_columb;
extern kal_bool gFG_Is_Charging;


////////////////////////////////////////////////////////////////////////////////
// AT
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// EM
////////////////////////////////////////////////////////////////////////////////
int g_BAT_TemperatureR = 0;
int g_TempBattVoltage = 0;
int g_InstatVolt = 0;
int g_BatteryAverageCurrent = 0;
int g_BAT_BatterySenseVoltage = 0;
int g_BAT_ISenseVoltage = 0;
int g_BAT_ChargerVoltage = 0;

////////////////////////////////////////////////////////////////////////////////
// Battery Logging Entry
////////////////////////////////////////////////////////////////////////////////
int Enable_BATDRV_LOG = 1;
//static struct proc_dir_entry *proc_entry;
char proc_bat_data[32];  

///////////////////////////////////////////////////////////////////////////////////////////
//// Smart Battery Structure
///////////////////////////////////////////////////////////////////////////////////////////
static TExternalBMT struExternalBMT = {0};
PMU_ChargerStruct BMT_status;

static TBatteryTempBound struBatteryTempBound = {0};

///////////////////////////////////////////////////////////////////////////////////////////
//// Thermal related flags
///////////////////////////////////////////////////////////////////////////////////////////
int g_battery_thermal_throttling_flag=3; // 0:nothing, 1:enable batTT&chrTimer, 2:disable batTT&chrTimer, 3:enable batTT, disable chrTimer
int battery_cmd_thermal_test_mode=0;
int battery_cmd_thermal_test_mode_value=0;
int g_battery_tt_check_flag=0; // 0:default enable check batteryTT, 1:default disable check batteryTT


///////////////////////////////////////////////////////////////////////////////////////////
//// Global Variable
///////////////////////////////////////////////////////////////////////////////////////////
struct wake_lock battery_suspend_lock; 
CHARGING_CONTROL battery_charging_control;
unsigned int g_BatteryNotifyCode=0x0000;
unsigned int g_BN_TestMode=0x0000;
kal_bool g_bat_init_flag = 0;
unsigned int g_call_state = CALL_IDLE;
kal_bool g_charging_full_reset_bat_meter = KAL_FALSE;
int g_platform_boot_mode = 0;
struct timespec g_bat_time_before_sleep;
int g_smartbook_update = 0;

#define BATTERY_MODULE_INIT //2015.4.1

#if defined(MTK_TEMPERATURE_RECHARGE_SUPPORT)
kal_uint32 g_batt_temp_status = TEMP_POS_NORMAL;
#endif

kal_bool battery_suspended = KAL_FALSE;
#ifdef MTK_ENABLE_AGING_ALGORITHM
extern U32 suspend_time;
#endif
////////////////////////////////////////////////////////////////////////////////
// Integrate with NVRAM 
////////////////////////////////////////////////////////////////////////////////
#define ADC_CALI_DEVNAME "MT_pmic_adc_cali"

#define TEST_ADC_CALI_PRINT 			_IO('k', 0)
#define SET_ADC_CALI_Slop 				_IOW('k', 1, int)
#define SET_ADC_CALI_Offset 			_IOW('k', 2, int)
#define SET_ADC_CALI_Cal 				_IOW('k', 3, int)
#define ADC_CHANNEL_READ 				_IOW('k', 4, int)
#define BAT_STATUS_READ 				_IOW('k', 5, int)
#define Set_Charger_Current 			_IOW('k', 6, int)
#define Get_FakeOff_Param 				_IOW('k', 7, int)
#define Get_Notify_Param 				_IOW('k', 8, int)
#define Turn_Off_Charging 				_IOW('k', 9, int)
//add for auto test
#define K_AT_CHG_CHGR_IN   				_IOW('k', 10, int)
#define K_AT_CHG_CHGR_OFF  				_IOW('k', 11, int)
#define K_AT_CHG_ON      				_IOW('k', 12, int)
#define K_AT_CHG_OFF      				_IOW('k', 13, int)
#define K_AT_CHG_INFO         			_IOW('k', 14, int)
#define	SET_SPI_CS_LOW					_IOW('k', 16, int)
//add for meta tool-----------------------------------------
#define Get_META_BAT_VOL 				_IOW('k', 17, int) 
#define Get_META_BAT_SOC 				_IOW('k', 18, int) 
//add for meta tool-----------------------------------------

#ifdef CONFIG_COMPAT
#define COMPAT_TEST_ADC_CALI_PRINT 		_IO('k', 0)
#define COMPAT_SET_ADC_CALI_Slop 		_IOW('k', 1, compat_int_t)
#define COMPAT_SET_ADC_CALI_Offset 		_IOW('k', 2, compat_int_t)
#define COMPAT_SET_ADC_CALI_Cal 		_IOW('k', 3, compat_int_t)
#define COMPAT_ADC_CHANNEL_READ 		_IOW('k', 4, compat_int_t)
#define COMPAT_BAT_STATUS_READ 			_IOW('k', 5, compat_int_t)
#define COMPAT_Set_Charger_Current 		_IOW('k', 6, compat_int_t)
#define COMPAT_Get_FakeOff_Param 		_IOW('k', 7, compat_int_t)
#define COMPAT_Get_Notify_Param 		_IOW('k', 8, compat_int_t)
#define COMPAT_Turn_Off_Charging 		_IOW('k', 9, compat_int_t)
#define COMPAT_K_AT_CHG_CHGR_IN   		_IOW('k', 10, compat_int_t)
#define COMPAT_K_AT_CHG_CHGR_OFF  		_IOW('k', 11, compat_int_t)
#define COMPAT_K_AT_CHG_ON      		_IOW('k', 12, compat_int_t)
#define COMPAT_K_AT_CHG_OFF      		_IOW('k', 13, compat_int_t)
#define COMPAT_K_AT_CHG_INFO         	_IOW('k', 14, compat_int_t)
#define	COMPAT_SET_SPI_CS_LOW			_IOW('k', 16, compat_int_t)
#define COMPAT_Get_META_BAT_VOL 		_IOW('k', 17, compat_int_t) 
#define COMPAT_Get_META_BAT_SOC 		_IOW('k', 18, compat_int_t) 
#endif

int g_hw_version = HW_VERSION__EVT;
int gpio_oppo_vooc_sw_ctrl = OPPO_VOOC_SW_CTRL_EVT;
void init_hw_version(void);

static int auto_out_data[5]={0,0,0,0,0};
int at_test_chg_on=1;// default is on;

int fakeoff_out_data[4] = {0,0,0,0};
int notify_out_data[1] = {0};

static struct class *adc_cali_class = NULL;
static int adc_cali_major = 0;
static dev_t adc_cali_devno;
static struct cdev *adc_cali_cdev;

int adc_cali_slop[14] = {1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000};
int adc_cali_offset[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int adc_cali_cal[1] = {0};
int battery_in_data[1] = {0};
int battery_out_data[1] = {0};    
int charging_level_data[1] = {0};
kal_bool g_ADC_Cali = KAL_FALSE;
kal_bool g_ftm_battery_flag = KAL_FALSE;
#if !defined(CONFIG_POWER_EXT)
static int g_wireless_state = 0;
#endif
#if defined(MTK_PUMP_EXPRESS_SUPPORT)
#undef V_CHARGER_MAX
#ifdef TA_9V_SUPPORT
#define V_CHARGER_MAX 9500				// 9.5 V
#else
#define V_CHARGER_MAX 6000				// 6.0 V
#endif	//TA_9V_SUPPORT
#endif	//MTK_PUMP_EXPRESS_SUPPORT
///////////////////////////////////////////////////////////////////////////////////////////
//// Thread related 
///////////////////////////////////////////////////////////////////////////////////////////
#define BAT_MS_TO_NS(x) (x * 1000 * 1000)
static kal_bool bat_thread_timeout = KAL_FALSE;
static kal_bool chr_wake_up_bat = KAL_FALSE;	// charger in/out to wake up battery thread
static kal_bool bat_meter_timeout = KAL_FALSE;
static DEFINE_MUTEX(bat_mutex);
static DEFINE_MUTEX(charger_type_mutex);
static DECLARE_WAIT_QUEUE_HEAD(bat_thread_wq);
static struct hrtimer charger_hv_detect_timer;
static struct task_struct *charger_hv_detect_thread = NULL;
static kal_bool charger_hv_detect_flag = KAL_FALSE;
static DECLARE_WAIT_QUEUE_HEAD(charger_hv_detect_waiter);
static struct hrtimer battery_kthread_timer;
static kal_bool g_battery_soc_ready=KAL_FALSE;
extern BOOL bat_spm_timeout;

static kal_bool KernelVendorBatTempIsGood(void);
static int kernel_get_bat_health(void);
kal_bool pmic_chrdet_status(void);
CHARGER_TYPE mt_charger_type_detection(void);
static void KernelVendorChgrInInitVariables(void);
static void KernelVendorChgrOutResetVariables(void);
static void KernelVendorVoteToStopCharging(TChgCmd cmd, TChgStopVoter voter);
void pchr_turn_off_charging_bq24196(void);
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
////////////////////////////////////////////////////////////////////////////////
// FOR ADB CMD
////////////////////////////////////////////////////////////////////////////////
/* Dual battery */
int g_status_2nd = POWER_SUPPLY_STATUS_NOT_CHARGING;
int g_capacity_2nd = 50;
int g_present_2nd = 0;
/* ADB charging CMD */
static int cmd_discharging = -1;
static int adjust_power = -1;
static int suspend_discharging = -1;
/*NEW CHARGER STANDARD V2_8 2015.12.3*/
extern int vol_count_temp5_12;
extern int vol_count_flag;

////////////////////////////////////////////////////////////////////////////////
// FOR ANDROID BATTERY SERVICE
////////////////////////////////////////////////////////////////////////////////

struct wireless_data {
    struct power_supply psy;
    int WIRELESS_ONLINE;    
};

struct ac_data {
    struct power_supply psy;
    int AC_ONLINE;    
};

struct usb_data {
    struct power_supply psy;
    int USB_ONLINE;
	int otg_switch; 
	int type_identify;
};

struct battery_data {
    struct power_supply psy;
    int BAT_STATUS;
    int BAT_HEALTH;
    int BAT_PRESENT;
    int BAT_TECHNOLOGY;
    int BAT_CAPACITY;
    /* Add for Battery Service*/
    int BAT_batt_vol;
    int BAT_batt_temp;
    /* Add for EM */
    int BAT_TemperatureR;
    int BAT_TempBattVoltage;
    int BAT_InstatVolt;
    int BAT_BatteryAverageCurrent;
    int BAT_BatterySenseVoltage;
    int BAT_ISenseVoltage;
    int BAT_ChargerVoltage;
	int battery_request_poweroff;//low battery in sleep
	int fastcharger;
	int charge_technology;
    /* Dual battery */
    int status_2nd;
    int capacity_2nd;
    int present_2nd;
    int adjust_power;
	int BAT_Fullcapacity;	//for Engineering Mode
	int BAT_MMI_CHG;			//for MMI_CHG_TEST
	int BAT_fcc;
	int BAT_soh;
	int BAT_cc;
};

static enum power_supply_property wireless_props[] = {
    POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property ac_props[] = {
    POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property usb_props[] = {
    POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_OTG_SWITCH,
	POWER_SUPPLY_PROP_PRIMAL_PROPERTY,
};

static enum power_supply_property battery_props[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_TECHNOLOGY,
    POWER_SUPPLY_PROP_CAPACITY,
    /* Add for Battery Service */
    POWER_SUPPLY_PROP_batt_vol,
    POWER_SUPPLY_PROP_batt_temp,    
    /* Add for EM */
    POWER_SUPPLY_PROP_TemperatureR,
    POWER_SUPPLY_PROP_TempBattVoltage,
    POWER_SUPPLY_PROP_InstatVolt,
    POWER_SUPPLY_PROP_BatteryAverageCurrent,
	POWER_SUPPLY_PROP_BatteryRequestPoweroff,
	POWER_SUPPLY_PROP_CHARGE_TECHNOLOGY,
	POWER_SUPPLY_PROP_FAST_CHARGE,
    POWER_SUPPLY_PROP_BatterySenseVoltage,
    POWER_SUPPLY_PROP_ISenseVoltage,
    POWER_SUPPLY_PROP_ChargerVoltage,
    POWER_SUPPLY_PROP_BatFullcapacity, 	//add for Engineering Mode
    POWER_SUPPLY_PROP_BATTERY_CHARGING_ENABLE,	//add for MMI_CHG_TEST
    POWER_SUPPLY_PROP_BATTERY_FCC,			
	POWER_SUPPLY_PROP_BATTERY_SOH,			
	POWER_SUPPLY_PROP_BATTERY_CC,
    /* Dual battery */
//    POWER_SUPPLY_PROP_status_2nd,
 //   POWER_SUPPLY_PROP_capacity_2nd,
 //   POWER_SUPPLY_PROP_present_2nd,
    /* ADB CMD Discharging */
    POWER_SUPPLY_PROP_adjust_power,
};



///////////////////////////////////////////////////////////////////////////////////////////
//// extern function
///////////////////////////////////////////////////////////////////////////////////////////
//extern void mt_power_off(void);
extern bool mt_usb_is_device(void);

#define CHARGE_PLUG_IN_TP_AVOID_DISTURB
#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
//pengnan  2015/5/1 add for tp avoid charge disturb 
extern int charge_plug_tp_avoid_distrub(int enable,int is_fast_charge);//add by PengNan for TP
int is_oppo_fast_charger = 0;
#endif 
#if defined(CONFIG_USB_MTK_HDRC) || defined(CONFIG_USB_MU3D_DRV)
//#if 0 // TBD, wait USB ready
extern void mt_usb_connect(void);
extern void mt_usb_disconnect(void);
#else
#define mt_usb_connect() do { } while (0)
#define mt_usb_disconnect() do { } while (0)
#endif

void charging_suspend_enable(void)
{
    U32 charging_enable = true;

    suspend_discharging = 0;
    battery_charging_control(CHARGING_CMD_ENABLE,&charging_enable);
}

void charging_suspend_disable(void)
{
    U32 charging_enable = false;

    suspend_discharging = 1;
    battery_charging_control(CHARGING_CMD_ENABLE,&charging_enable);
}

bool check_batt_exist()
{
	return BMT_status.bat_exist;
}

int read_tbat_value(void)
{
    return BMT_status.temperature;
}

int get_charger_detect_status(void)
{
	kal_bool chr_status;

	battery_charging_control(CHARGING_CMD_GET_CHARGER_DET_STATUS,&chr_status);
    return chr_status;
}

#if defined(MTK_POWER_EXT_DETECT)
kal_bool bat_is_ext_power(void)
{
	kal_bool pwr_src = 0;

	battery_charging_control(CHARGING_CMD_GET_POWER_SOURCE, &pwr_src);
	battery_xlog_printk(BAT_LOG_FULL, "[BAT_IS_EXT_POWER] is_ext_power = %d\n", pwr_src);
	return pwr_src;
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////
//// PMIC PCHR Related APIs
///////////////////////////////////////////////////////////////////////////////////////////
kal_bool upmu_is_chr_det(void)
{
#if !defined(CONFIG_POWER_EXT)
	kal_uint32 tmp32;
#endif	
	if (!g_bat_init_flag) {
		battery_xlog_printk(BAT_LOG_CRTI, "[upmu_is_chr_det] battery thread not ready, will do after bettery init.\n");
		return KAL_FALSE;
	}
#if defined(CONFIG_POWER_EXT)
    //return KAL_TRUE;
    return get_charger_detect_status();
#else

    if (suspend_discharging==1)
        return KAL_FALSE;

    tmp32=get_charger_detect_status();

#ifdef MTK_POWER_EXT_DETECT
	if (KAL_TRUE == bat_is_ext_power())
		return tmp32;
#endif
	
    if(tmp32 == 0)
    {
        return KAL_FALSE;
    }
    else
    {
        if( mt_usb_is_device() )
        {
        	battery_xlog_printk(BAT_LOG_FULL, "[upmu_is_chr_det] Charger exist and USB is not host\n");

            return KAL_TRUE;
        }
        else
        {
            battery_xlog_printk(BAT_LOG_CRTI, "[upmu_is_chr_det] Charger exist but USB is host\n");

            return KAL_FALSE;
        }
    }
#endif	
}
EXPORT_SYMBOL(upmu_is_chr_det);


void wake_up_bat (void)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] wake_up_bat. \r\n");
    
    chr_wake_up_bat = KAL_TRUE;    
    bat_thread_timeout = KAL_TRUE;
#ifdef MTK_ENABLE_AGING_ALGORITHM
    suspend_time = 0;
#endif
    wake_up(&bat_thread_wq);
}
EXPORT_SYMBOL(wake_up_bat);


static ssize_t bat_log_write( struct file *filp, const char __user *buff,
                        size_t len, loff_t *data )
{
    if (copy_from_user( &proc_bat_data, buff, len )) {
        battery_xlog_printk(BAT_LOG_FULL, "bat_log_write error.\n");
        return -EFAULT;
    }

    if (proc_bat_data[0] == '1') {
        battery_xlog_printk(BAT_LOG_CRTI, "enable battery driver log system\n");
        Enable_BATDRV_LOG = 1;
    } else if (proc_bat_data[0] == '2') {
        battery_xlog_printk(BAT_LOG_CRTI, "enable battery driver log system:2\n");
        Enable_BATDRV_LOG = 2;    
    } else {
        battery_xlog_printk(BAT_LOG_CRTI, "Disable battery driver log system\n");
        Enable_BATDRV_LOG = 0;
    }
    
    return len;
}

static const struct file_operations bat_proc_fops = { 
    .write = bat_log_write,
};

int init_proc_log(void)
{
    int ret=0;

#if 1
    proc_create("batdrv_log", 0644, NULL, &bat_proc_fops);
    battery_xlog_printk(BAT_LOG_CRTI, "proc_create bat_proc_fops\n");
#else    
    proc_entry = create_proc_entry( "batdrv_log", 0644, NULL );
    
    if (proc_entry == NULL) {
        ret = -ENOMEM;
        battery_xlog_printk(BAT_LOG_FULL, "init_proc_log: Couldn't create proc entry\n");
    } else {
        proc_entry->write_proc = bat_log_write;       
        battery_xlog_printk(BAT_LOG_CRTI, "init_proc_log loaded.\n");
    }
#endif
  
    return ret;
}


static int wireless_get_property(struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    int ret = 0;
    struct wireless_data *data = container_of(psy, struct wireless_data, psy);    
 
    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:                           
        val->intval = data->WIRELESS_ONLINE;    
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int ac_get_property(struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    int ret = 0;
    struct ac_data *data = container_of(psy, struct ac_data, psy);    
	
	if( pmic_chrdet_status() == KAL_TRUE )
	{		 
		if ( (BMT_status.charger_type == NONSTANDARD_CHARGER) || 
			 (BMT_status.charger_type == STANDARD_CHARGER) 	  || 
			 (BMT_status.charger_type == APPLE_2_1A_CHARGER)  ||
			 (BMT_status.charger_type == APPLE_1_0A_CHARGER)  ||
			 (BMT_status.charger_type == APPLE_0_5A_CHARGER)
			)
		{
			data->AC_ONLINE = 1; 	   
			psy->type = POWER_SUPPLY_TYPE_MAINS;
		}
		else
    	{
    		if(opchg_get_prop_fast_switch_to_normal() == KAL_TRUE)
			{
				data->AC_ONLINE = 1;
				printk("ac_get_property,pmic_chrdet_status = true,opchg_get_prop_fast_switch_to_normal\r\n");
			}
			else if(opchg_get_fast_normal_to_warm() == true)
			{
				data->AC_ONLINE = 1;
				printk("ac_get_property,pmic_chrdet_status = true,opchg_get_fast_normal_to_warm\r\n");
			}
			else
			{
				data->AC_ONLINE = 0; 	
//				printk("ac_get_property,pmic_chrdet_status = true,else data->AC_ONLINE = 0\r\n");
			}				
    	}
	}
	else
	{
		if(opchg_get_prop_fast_switch_to_normal() == KAL_TRUE)
		{
			data->AC_ONLINE = 1;
			printk("ac_get_property,pmic_chrdet_status = false,opchg_get_prop_fast_switch_to_normal\r\n");
		}
		else if(opchg_get_fast_normal_to_warm() == true)
		{
			data->AC_ONLINE = 1;
			printk("ac_get_property,pmic_chrdet_status = false,opchg_get_fast_normal_to_warm\r\n");
		}
		else
		{
			data->AC_ONLINE = 0;
//			printk("ac_get_property,pmic_chrdet_status = false,else data->AC_ONLINE = 0\r\n");
		}
		
	}
	
    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:                           
        val->intval = data->AC_ONLINE;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int usb_property_is_writeable(struct power_supply *psy,
    enum power_supply_property psp)
{
	switch (psp) {
    case POWER_SUPPLY_PROP_OTG_SWITCH:
        return 1;
    default:
        break;
    }
    
    return 0;
}
//extern void mt_usb_ext_iddig_int(void);
//#define IDDIG_EINT_PIN 63
//extern void usb_hub_cleanup(void);
extern void mtk_xhci_eint_iddig_gpio_mode(void);

static int usb_set_property(struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
	int ret = 0;
	struct usb_data *data = container_of(psy, struct usb_data, psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_OTG_SWITCH:
        if(val->intval == KAL_TRUE){
		    /*
			#if 1
			mt_set_gpio_mode(GPIO_OTG_IDDIG_EINT_PIN, GPIO_OTG_IDDIG_EINT_PIN_M_IDDIG);
			mt_set_gpio_dir(GPIO_OTG_IDDIG_EINT_PIN, GPIO_DIR_IN);
			mt_set_gpio_pull_enable(GPIO_OTG_IDDIG_EINT_PIN, GPIO_PULL_ENABLE);
			mt_set_gpio_pull_select(GPIO_OTG_IDDIG_EINT_PIN, GPIO_PULL_UP);
			#endif
			#if 1
			mt_eint_unmask(IDDIG_EINT_PIN);
			#endif
			*/
			
/*
			mt_eint_set_sens(IDDIG_EINT_PIN, MT_LEVEL_SENSITIVE);
			mt_eint_set_hw_debounce(IDDIG_EINT_PIN,64);
			mt_eint_registration(IDDIG_EINT_PIN, EINTF_TRIGGER_LOW, mt_usb_ext_iddig_int, FALSE);
			//mt_set_gpio_dir(GPIO_OTG_IDDIG_EINT_PIN, 1)	;	*/
			data->otg_switch = 1;
			//printk("[%s]usb_set_property = %d,mt_get_gpio_dir(GPIO_OTG_IDDIG_EINT_PIN) = %d,mt_get_gpio_in(GPIO_OTG_IDDIG_EINT_PIN) =%d,mt_eint_get_mask(IDDIG_EINT_PIN) = %d",__func__, val->intval,mt_get_gpio_dir(GPIO_OTG_IDDIG_EINT_PIN),mt_get_gpio_in(GPIO_OTG_IDDIG_EINT_PIN),mt_eint_get_mask(IDDIG_EINT_PIN));
		}
		else {
		/*
			#if 1
			mt_eint_mask(IDDIG_EINT_PIN);
			#endif
			mt_usb_ext_iddig_int();
			#if 1
			mt_set_gpio_mode(GPIO_OTG_IDDIG_EINT_PIN, GPIO_MODE_00);
			mt_set_gpio_dir(GPIO_OTG_IDDIG_EINT_PIN, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO_OTG_IDDIG_EINT_PIN, 0);
			#endif
			*/
			data->otg_switch = 0;
			mtk_xhci_eint_iddig_gpio_mode();
			//usb_hub_cleanup();
			bq24196_otg_disable();
			//printk("[%s]usb_set_property = %d,mt_get_gpio_dir(GPIO_OTG_IDDIG_EINT_PIN) = %d,mt_get_gpio_out(GPIO_OTG_IDDIG_EINT_PIN) = %d,mt_get_gpio_mode(GPIO_OTG_IDDIG_EINT_PIN) = %d,mt_eint_get_mask(IDDIG_EINT_PIN) = %d\n",__func__, val->intval,mt_get_gpio_dir(GPIO_OTG_IDDIG_EINT_PIN),mt_get_gpio_out(GPIO_OTG_IDDIG_EINT_PIN),mt_get_gpio_mode(GPIO_OTG_IDDIG_EINT_PIN),mt_eint_get_mask(IDDIG_EINT_PIN));
		}
        break;

	default:
        ret = -EINVAL;
        break;
	}
}

static int usb_get_property(struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    int ret = 0;
    struct usb_data *data = container_of(psy, struct usb_data, psy);    

    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:     
        #if defined(CONFIG_POWER_EXT)
        //#if 0
        data->USB_ONLINE = 1;
        val->intval = data->USB_ONLINE;
        #else
		#if defined(MTK_POWER_EXT_DETECT)
		if(KAL_TRUE == bat_is_ext_power())
			data->USB_ONLINE = 1;
		#endif
        val->intval = data->USB_ONLINE;
        #endif        
        break;
	case POWER_SUPPLY_PROP_OTG_SWITCH:		
        val->intval = data->otg_switch;
        break;
	case POWER_SUPPLY_PROP_PRIMAL_PROPERTY:
		val->intval = data->type_identify;
		printk("%s: type_identify:%d\n",__func__,data->type_identify);
		break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int battery_property_is_writeable(struct power_supply *psy,
    enum power_supply_property psp)
{
	switch (psp) {
    case POWER_SUPPLY_PROP_BATTERY_CHARGING_ENABLE:
        return 1;
    default:
        break;
    }
    
    return 0;
}

static int battery_set_property(struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
	int ret = 0;
	struct battery_data *data = container_of(psy, struct battery_data, psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_BATTERY_CHARGING_ENABLE:
        if(val->intval == KAL_TRUE){
			#ifdef OPPO_MMI_FASTCHARGER
			opchg_set_fast_switch_to_normal_false();
			#endif
			bq24196_enable_charging();
			data->BAT_MMI_CHG = 1;
			at_test_chg_on = 1;
			BMT_status.bat_charging_state = BQ24196_CHR_CCCV;
			printk("[%s]Enable val_intval = %d, bat_charging_state = %d\n",__func__, val->intval, BMT_status.bat_charging_state);
		}
		else {
			#ifdef OPPO_MMI_FASTCHARGER
			opchg_set_switch_mode(NORMAL_CHARGER_MODE);
			opchg_set_fast_switch_to_normal_true();
			#endif
			pchr_turn_off_charging_bq24196();
			data->BAT_MMI_CHG = 0;
			at_test_chg_on = 0;
			BMT_status.bat_charging_state = BQ24196_CHR_FAIL;
			printk("[%s]Disable val_intval = %d, bat_charging_state = %d\n",__func__, val->intval, BMT_status.bat_charging_state);
		}
        break;

	default:
        ret = -EINVAL;
        break;
	}
}

static int battery_get_property(struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    int ret = 0;     
    struct battery_data *data = container_of(psy, struct battery_data, psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
        val->intval = data->BAT_STATUS;
        break;
    case POWER_SUPPLY_PROP_HEALTH:
        val->intval = data->BAT_HEALTH;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = data->BAT_PRESENT;
        break;
    case POWER_SUPPLY_PROP_TECHNOLOGY:
        val->intval = data->BAT_TECHNOLOGY;
        break;
    case POWER_SUPPLY_PROP_CAPACITY:
        val->intval = data->BAT_CAPACITY;
        break;        
    case POWER_SUPPLY_PROP_batt_vol:
        val->intval = data->BAT_batt_vol;
        break;
    case POWER_SUPPLY_PROP_batt_temp:
        val->intval = data->BAT_batt_temp;
        break;
    case POWER_SUPPLY_PROP_TemperatureR:
        val->intval = data->BAT_TemperatureR;
        break;    
    case POWER_SUPPLY_PROP_TempBattVoltage:        
        val->intval = data->BAT_TempBattVoltage;
        break;    
    case POWER_SUPPLY_PROP_InstatVolt:
        val->intval = data->BAT_InstatVolt;
        break;    
    case POWER_SUPPLY_PROP_BatteryAverageCurrent:
		//val->intval = data->BAT_BatteryAverageCurrent;
		BMT_status.ICharging = opchg_get_prop_current_now();
        val->intval = BMT_status.ICharging;
        break;
	case POWER_SUPPLY_PROP_BatteryRequestPoweroff:  //Fanhong.Kong@ProDrv.CHG, 2012/03/08, Add for low battery in sleep
	    val->intval = data->battery_request_poweroff;
	    break;
	case POWER_SUPPLY_PROP_CHARGE_TECHNOLOGY:
        val->intval = data->charge_technology;
        break;
	case POWER_SUPPLY_PROP_FAST_CHARGE:
		data->fastcharger = opchg_get_prop_fast_chg_started();
		val->intval = data->fastcharger;
		#if 1
			if((val->intval == 0) && (data->BAT_STATUS != POWER_SUPPLY_STATUS_FULL)
				&& (struExternalBMT.nChgStopVoterMask & CHG_STOP_VOTER__BATTTEMP_ABNORMAL) != CHG_STOP_VOTER__BATTTEMP_ABNORMAL
				&& (struExternalBMT.nChgStopVoterMask & CHG_STOP_VOTER__VCHG_ABNORMAL) != CHG_STOP_VOTER__VCHG_ABNORMAL
				&& (struExternalBMT.nChgStopVoterMask & CHG_STOP_VOTER__VBAT_TOO_HIGH) != CHG_STOP_VOTER__VBAT_TOO_HIGH
				&& (struExternalBMT.nChgStopVoterMask & CHG_STOP_VOTER__MAX_CHGING_TIME) != CHG_STOP_VOTER__MAX_CHGING_TIME) {
				if((opchg_get_prop_fast_switch_to_normal() == true && data->BAT_MMI_CHG == 1) || 
					opchg_get_fast_normal_to_warm() == true) {
					val->intval = 1;
				}
			}
		#endif
        break;
    case POWER_SUPPLY_PROP_BatterySenseVoltage:
        val->intval = data->BAT_BatterySenseVoltage;
        break;    
    case POWER_SUPPLY_PROP_ISenseVoltage:
        val->intval = data->BAT_ISenseVoltage;
        break;    
    case POWER_SUPPLY_PROP_ChargerVoltage:
        val->intval = data->BAT_ChargerVoltage;
        break;
	case POWER_SUPPLY_PROP_BatFullcapacity:		//add for Engineering Mode
        val->intval = data->BAT_Fullcapacity;
        break;
	case POWER_SUPPLY_PROP_BATTERY_CHARGING_ENABLE:	//add for MMI_CHG TEST
        val->intval = data->BAT_MMI_CHG;
        break;
	case POWER_SUPPLY_PROP_BATTERY_FCC:
		val->intval = data->BAT_fcc;
		if(bq27541_di == NULL)
		{
			val->intval = 100;
		}
		else
		{
			//val->intval = bq27541_di->full_charge_soc;
			val->intval = bq27541_di->fcc_pre;
		}
        break;
	case POWER_SUPPLY_PROP_BATTERY_SOH:
		val->intval = data->BAT_soh;
		if(bq27541_di == NULL)
		{
			val->intval = 100;
		}
		else
		{
			//val->intval = bq27541_di->state_of_health;
			val->intval = bq27541_di->soh_pre;
		}
        break;
	case POWER_SUPPLY_PROP_BATTERY_CC:
		val->intval = data->BAT_cc;
		if(bq27541_di == NULL)
		{
			val->intval = 100;
		}
		else
		{
			//val->intval = bq27541_di->state_of_health;
			val->intval = bq27541_di->cc_pre;
		}
        break;
    /* Dual battery */
 //   case POWER_SUPPLY_PROP_status_2nd :
 //       val->intval = data->status_2nd;
 //       break;
 //   case POWER_SUPPLY_PROP_capacity_2nd :
 //       val->intval = data->capacity_2nd;
 //       break;
 //   case POWER_SUPPLY_PROP_present_2nd :
 //       val->intval = data->present_2nd;
 //       break;
    case POWER_SUPPLY_PROP_adjust_power :
        val->intval = data->adjust_power;
        break;

    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

/* wireless_data initialization */
static struct wireless_data wireless_main = {
    .psy = {
    .name = "wireless",
    .type = POWER_SUPPLY_TYPE_WIRELESS,
    .properties = wireless_props,
    .num_properties = ARRAY_SIZE(wireless_props),
    .get_property = wireless_get_property,                
    },
    .WIRELESS_ONLINE = 0,
};

/* ac_data initialization */
static struct ac_data ac_main = {
    .psy = {
    .name = "ac",
    .type = POWER_SUPPLY_TYPE_MAINS,
    .properties = ac_props,
    .num_properties = ARRAY_SIZE(ac_props),
    .get_property = ac_get_property,                
    },
    .AC_ONLINE = 0,
};

/* usb_data initialization */
static struct usb_data usb_main = {
    .psy = {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .properties = usb_props,
    .num_properties = ARRAY_SIZE(usb_props),
    .get_property = usb_get_property,
	.set_property = usb_set_property,
    .property_is_writeable = usb_property_is_writeable,                
    },
	.otg_switch = 0,
    .USB_ONLINE = 0,
	.type_identify = 0,
};

bool get_otg_switch()
{
	return usb_main.otg_switch;
}

/* battery_data initialization */
static struct battery_data battery_main = {
    .psy = {
    .name = "battery",
    .type = POWER_SUPPLY_TYPE_BATTERY,
    .properties = battery_props,
    .num_properties = ARRAY_SIZE(battery_props),
    .get_property = battery_get_property,
    .set_property = battery_set_property,
    .property_is_writeable = battery_property_is_writeable,
    },
/* CC: modify to have a full power supply status */
#if defined(CONFIG_POWER_EXT)
    .BAT_STATUS = POWER_SUPPLY_STATUS_FULL,    
    .BAT_HEALTH = POWER_SUPPLY_HEALTH_GOOD,
    .BAT_PRESENT = 1,
    .BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION,
    .BAT_CAPACITY = 100,
    .BAT_batt_vol = 4200,
    .BAT_batt_temp = 22,
    /* Dual battery */
    .status_2nd = POWER_SUPPLY_STATUS_NOT_CHARGING,
    .capacity_2nd = 50,
    .present_2nd = 0,
    /* ADB CMD discharging*/
    .adjust_power = -1,
#else
    .BAT_STATUS = POWER_SUPPLY_STATUS_NOT_CHARGING,    
    .BAT_HEALTH = POWER_SUPPLY_HEALTH_GOOD,
    .BAT_PRESENT = 1,
    .BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION,
    .BAT_CAPACITY = 50,
    .BAT_batt_vol = 0,
    .BAT_batt_temp = 0,
	.battery_request_poweroff=0,
	.fastcharger = 0,
	.charge_technology = POWER_SUPPLY_CHARGE_TECHNOLOGY_FAST,
    /* Dual battery */
    .status_2nd = POWER_SUPPLY_STATUS_NOT_CHARGING,
    .capacity_2nd = 50,
    .present_2nd = 0,
    /* ADB CMD discharging*/
    .adjust_power = -1,
    /*Add for Engineering Mode*/
	.BAT_Fullcapacity = BATTERY_FULLCAPACITY,
	.BAT_MMI_CHG = 1,
	.BAT_fcc = 4100,
	.BAT_soh = 0,
	.BAT_cc = 0,
#endif
};

UINT32 auto_test_revert_g_notify_flag(void)
{
    /*
#define     Notify_Charger_Over_Vol                   1 
#define     Notify_Charger_Low_Vol                    2 
#define     Notify_Bat_Over_Temp                      3
#define     Notify_Bat_Low_Temp                       4
#define     Notify_Bat_Not_Connect                    5
#define     Notify_Bat_Over_Vol                       6
#define     Notify_Bat_Full                           7
#define     Notify_Chging_Current                     8
#define		Notify_Chging_OverTime					  9
    */
    UINT32 data=0;
	if(g_NotifyFlag==Notify_Chging_OverTime)
    {
        data=0x0001;
    }
    else if(g_NotifyFlag==Notify_Charger_Over_Vol)
    {
        data=0x0010;
    }
    else if(g_NotifyFlag==Notify_Charger_Low_Vol)
    {
        data=0x0020;
    }
    else if(g_NotifyFlag==Notify_Bat_Over_Temp)
    {
        data=0x0040;
    }
    else if(g_NotifyFlag==Notify_Bat_Low_Temp)
    {
        data=0x0080;
    }
    else if(g_NotifyFlag==Notify_Bat_Not_Connect)
    {
        data=0x0400;
    }
    else if(g_NotifyFlag==Notify_Bat_Over_Vol)
    {
        data=0x0200;
    }
    else if(g_NotifyFlag==Notify_Bat_Full)
    {
        data=0x0004;
    }
    else if(g_NotifyFlag==Notify_Chging_Current)
    {
        data=0x0100;
    }
    else
    {
        data=0x0002;
    }

    return data;
    
}


#if !defined(CONFIG_POWER_EXT)
///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Charger_Voltage
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Charger_Voltage(struct device *dev,struct device_attribute *attr, char *buf)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] show_ADC_Charger_Voltage : %d\n", BMT_status.charger_vol);
    return sprintf(buf, "%d\n", BMT_status.charger_vol);
}
static ssize_t store_ADC_Charger_Voltage(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Charger_Voltage, 0664, show_ADC_Charger_Voltage, store_ADC_Charger_Voltage);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_0_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_0_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+0));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_0_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_0_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_0_Slope, 0664, show_ADC_Channel_0_Slope, store_ADC_Channel_0_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_1_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_1_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+1));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_1_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_1_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_1_Slope, 0664, show_ADC_Channel_1_Slope, store_ADC_Channel_1_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_2_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_2_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+2));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_2_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_2_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_2_Slope, 0664, show_ADC_Channel_2_Slope, store_ADC_Channel_2_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_3_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_3_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+3));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_3_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_3_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_3_Slope, 0664, show_ADC_Channel_3_Slope, store_ADC_Channel_3_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_4_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_4_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+4));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_4_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_4_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_4_Slope, 0664, show_ADC_Channel_4_Slope, store_ADC_Channel_4_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_5_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_5_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+5));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_5_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_5_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_5_Slope, 0664, show_ADC_Channel_5_Slope, store_ADC_Channel_5_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_6_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_6_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+6));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_6_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_6_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_6_Slope, 0664, show_ADC_Channel_6_Slope, store_ADC_Channel_6_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_7_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_7_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+7));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_7_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_7_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_7_Slope, 0664, show_ADC_Channel_7_Slope, store_ADC_Channel_7_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_8_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_8_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+8));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_8_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_8_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_8_Slope, 0664, show_ADC_Channel_8_Slope, store_ADC_Channel_8_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_9_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_9_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+9));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_9_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_9_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_9_Slope, 0664, show_ADC_Channel_9_Slope, store_ADC_Channel_9_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_10_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_10_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+10));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_10_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_10_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_10_Slope, 0664, show_ADC_Channel_10_Slope, store_ADC_Channel_10_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_11_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_11_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+11));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_11_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_11_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_11_Slope, 0664, show_ADC_Channel_11_Slope, store_ADC_Channel_11_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_12_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_12_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+12));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_12_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_12_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_12_Slope, 0664, show_ADC_Channel_12_Slope, store_ADC_Channel_12_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_13_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_13_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_slop+13));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_13_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_13_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_13_Slope, 0664, show_ADC_Channel_13_Slope, store_ADC_Channel_13_Slope);


///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_0_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_0_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+0));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_0_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_0_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_0_Offset, 0664, show_ADC_Channel_0_Offset, store_ADC_Channel_0_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_1_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_1_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+1));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_1_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_1_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_1_Offset, 0664, show_ADC_Channel_1_Offset, store_ADC_Channel_1_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_2_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_2_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+2));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_2_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_2_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_2_Offset, 0664, show_ADC_Channel_2_Offset, store_ADC_Channel_2_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_3_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_3_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+3));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_3_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_3_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_3_Offset, 0664, show_ADC_Channel_3_Offset, store_ADC_Channel_3_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_4_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_4_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+4));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_4_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_4_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_4_Offset, 0664, show_ADC_Channel_4_Offset, store_ADC_Channel_4_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_5_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_5_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+5));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_5_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_5_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_5_Offset, 0664, show_ADC_Channel_5_Offset, store_ADC_Channel_5_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_6_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_6_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+6));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_6_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_6_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_6_Offset, 0664, show_ADC_Channel_6_Offset, store_ADC_Channel_6_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_7_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_7_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+7));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_7_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_7_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_7_Offset, 0664, show_ADC_Channel_7_Offset, store_ADC_Channel_7_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_8_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_8_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+8));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_8_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_8_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_8_Offset, 0664, show_ADC_Channel_8_Offset, store_ADC_Channel_8_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_9_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_9_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+9));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_9_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_9_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_9_Offset, 0664, show_ADC_Channel_9_Offset, store_ADC_Channel_9_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_10_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_10_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+10));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_10_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_10_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_10_Offset, 0664, show_ADC_Channel_10_Offset, store_ADC_Channel_10_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_11_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_11_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+11));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_11_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_11_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_11_Offset, 0664, show_ADC_Channel_11_Offset, store_ADC_Channel_11_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_12_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_12_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+12));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_12_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_12_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_12_Offset, 0664, show_ADC_Channel_12_Offset, store_ADC_Channel_12_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_13_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_13_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = (*(adc_cali_offset+13));
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_13_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_13_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_13_Offset, 0664, show_ADC_Channel_13_Offset, store_ADC_Channel_13_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : ADC_Channel_Is_Calibration
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_ADC_Channel_Is_Calibration(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=2;
    ret_value = g_ADC_Cali;
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] ADC_Channel_Is_Calibration : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_ADC_Channel_Is_Calibration(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(ADC_Channel_Is_Calibration, 0664, show_ADC_Channel_Is_Calibration, store_ADC_Channel_Is_Calibration);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : Power_On_Voltage
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_Power_On_Voltage(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = 3400;
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Power_On_Voltage : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_Power_On_Voltage(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(Power_On_Voltage, 0664, show_Power_On_Voltage, store_Power_On_Voltage);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : Power_Off_Voltage
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_Power_Off_Voltage(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = 3400;
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Power_Off_Voltage : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_Power_Off_Voltage(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(Power_Off_Voltage, 0664, show_Power_Off_Voltage, store_Power_Off_Voltage);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : Charger_TopOff_Value
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_Charger_TopOff_Value(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=1;
    ret_value = 4110;
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Charger_TopOff_Value : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_Charger_TopOff_Value(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(Charger_TopOff_Value, 0664, show_Charger_TopOff_Value, store_Charger_TopOff_Value);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : FG_Battery_CurrentConsumption
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_FG_Battery_CurrentConsumption(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value=8888;
    ret_value = battery_meter_get_battery_current();    
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] FG_Battery_CurrentConsumption : %d/10 mA\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_FG_Battery_CurrentConsumption(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(FG_Battery_CurrentConsumption, 0664, show_FG_Battery_CurrentConsumption, store_FG_Battery_CurrentConsumption);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : FG_SW_CoulombCounter
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_FG_SW_CoulombCounter(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_int32 ret_value=7777;
    ret_value = battery_meter_get_car();
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] FG_SW_CoulombCounter : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_FG_SW_CoulombCounter(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(FG_SW_CoulombCounter, 0664, show_FG_SW_CoulombCounter, store_FG_SW_CoulombCounter);


static ssize_t show_Charging_CallState(struct device *dev,struct device_attribute *attr, char *buf)
{
    battery_xlog_printk(BAT_LOG_CRTI, "call state = %d\n",g_call_state);    
    return sprintf(buf, "%u\n", g_call_state);
}
static ssize_t store_Charging_CallState(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%u", &g_call_state);
    battery_xlog_printk(BAT_LOG_CRTI, "call state = %d\n",g_call_state);    
    return size;
}
static DEVICE_ATTR(Charging_CallState, 0664, show_Charging_CallState, store_Charging_CallState);

static ssize_t show_Charger_Type(struct device *dev,struct device_attribute *attr, char *buf)
{
    UINT32 chr_ype = CHARGER_UNKNOWN;
    chr_ype = BMT_status.charger_exist ? BMT_status.charger_type : CHARGER_UNKNOWN;

    battery_xlog_printk(BAT_LOG_CRTI, "CHARGER_TYPE = %d\n",chr_ype);
    return sprintf(buf, "%u\n", chr_ype);
}
static ssize_t store_Charger_Type(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[EM] Not Support Write Function\n");
    return size;
}
static DEVICE_ATTR(Charger_Type, 0664, show_Charger_Type, store_Charger_Type);

#if defined(MTK_PUMP_EXPRESS_SUPPORT) || defined(MTK_PUMP_EXPRESS_PLUS_SUPPORT)
static ssize_t show_Pump_Express(struct device *dev,struct device_attribute *attr, char *buf)
{
    battery_xlog_printk(BAT_LOG_CRTI, "Pump express = %d\n",is_ta_connect);    
    return sprintf(buf, "%u\n", is_ta_connect);
}
static ssize_t store_Pump_Express(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%u", &is_ta_connect);
    battery_xlog_printk(BAT_LOG_CRTI, "Pump express= %d\n",is_ta_connect);    
    return size;
}
static DEVICE_ATTR(Pump_Express, 0664, show_Pump_Express, store_Pump_Express);
#endif

static void mt_battery_update_EM(struct battery_data *bat_data)
{
	//bat_data->BAT_CAPACITY = BMT_status.UI_SOC;
	bat_data->BAT_CAPACITY = bat_volt_check_point;
    bat_data->BAT_TemperatureR=BMT_status.temperatureR;	//API
    bat_data->BAT_TempBattVoltage=BMT_status.temperatureV; // API
    bat_data->BAT_InstatVolt=BMT_status.bat_vol;	//VBAT
    bat_data->BAT_BatteryAverageCurrent=BMT_status.ICharging;	
    bat_data->BAT_BatterySenseVoltage=BMT_status.bat_vol;	
    bat_data->BAT_ISenseVoltage=BMT_status.Vsense;	// API
    bat_data->BAT_ChargerVoltage=BMT_status.charger_vol;	
    /* Dual battery */
    bat_data->status_2nd = g_status_2nd;
    bat_data->capacity_2nd = g_capacity_2nd;
    bat_data->present_2nd = g_present_2nd;
	battery_xlog_printk(BAT_LOG_FULL, "status_2nd = %d, capacity_2nd = %d, present_2nd = %d\n", bat_data->status_2nd, bat_data->capacity_2nd, bat_data->present_2nd);
	if (cmd_discharging == 1) {
		bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_CMD_DISCHARGING;
	}
	if (adjust_power != -1) {
			bat_data->adjust_power = adjust_power;
			battery_xlog_printk(BAT_LOG_CRTI, "adjust_power=(%d)\n", adjust_power);
	}
	if (Enable_BATDRV_LOG == 1) {
    	xlog_printk(ANDROID_LOG_INFO, "------------mt6577_update", "[BATTERY] BMT_status.SOC = %d,BMT_status.bat_vol = %d, bat_volt_check_point = %d!\n\r", BMT_status.SOC, BMT_status.bat_vol, bat_volt_check_point);            
		}
}

#if 0
static kal_bool mt_battery_100Percent_tracking_check(void)
{
	kal_bool resetBatteryMeter = KAL_FALSE;
	
#if defined(MTK_JEITA_STANDARD_SUPPORT)
	kal_uint32 cust_sync_time = CUST_SOC_JEITA_SYNC_TIME;
	static kal_uint32 timer_counter = (CUST_SOC_JEITA_SYNC_TIME/BAT_TASK_PERIOD);
#else
	kal_uint32 cust_sync_time = ONEHUNDRED_PERCENT_TRACKING_TIME;
	static kal_uint32 timer_counter = (ONEHUNDRED_PERCENT_TRACKING_TIME/BAT_TASK_PERIOD);
#endif

	 if(BMT_status.bat_full == KAL_TRUE)	// charging full first, UI tracking to 100%
	 {
	 	if(BMT_status.UI_SOC >= 100)
		{
			BMT_status.UI_SOC = 100;
			
			if((g_charging_full_reset_bat_meter == KAL_TRUE) && (BMT_status.bat_charging_state == CHR_BATFULL))
			{
				resetBatteryMeter = KAL_TRUE;
				g_charging_full_reset_bat_meter = KAL_FALSE;
			}
			else
			{
				resetBatteryMeter = KAL_FALSE;
			}	
		}
		else
		{
		        //increase UI percentage every xxs
                        if(timer_counter >= (cust_sync_time/BAT_TASK_PERIOD))
                        {
                                timer_counter=1;
                                BMT_status.UI_SOC++;				   
                        }
                        else
                        {
                                timer_counter++;

                                return resetBatteryMeter;
                         }	
	 	
                         resetBatteryMeter = KAL_TRUE;
	        }
		
		battery_xlog_printk(BAT_LOG_CRTI, "[100percent], UI_SOC(%d), reset(%d) \n",
			BMT_status.UI_SOC,resetBatteryMeter);
	 }
	 else	
	 {
	 	// charging is not full,  UI keep 99% if reaching 100%,  
				
		if(BMT_status.UI_SOC>=99)
        {
            BMT_status.UI_SOC=99;
			resetBatteryMeter = KAL_FALSE;
       		
			battery_xlog_printk(BAT_LOG_CRTI, "[100percent],UI_SOC = %d \n", BMT_status.UI_SOC);
 		}

		timer_counter = (cust_sync_time/BAT_TASK_PERIOD);

	 }
	 
	 return resetBatteryMeter;	 
}


static kal_bool mt_battery_nPercent_tracking_check(void)
{
	kal_bool resetBatteryMeter = KAL_FALSE;
#if defined(SOC_BY_HW_FG)
	static kal_uint32 timer_counter = (NPERCENT_TRACKING_TIME/BAT_TASK_PERIOD);	

    if (BMT_status.nPrecent_UI_SOC_check_point == 0)
        return KAL_FALSE;
			
	// fuel gauge ZCV < 15%, but UI > 15%,  15% can be customized 
	if ( (BMT_status.ZCV <= BMT_status.nPercent_ZCV) &&(BMT_status.UI_SOC > BMT_status.nPrecent_UI_SOC_check_point) )
	{
		  if(timer_counter == (NPERCENT_TRACKING_TIME/BAT_TASK_PERIOD))	// every x sec decrease UI percentage
		  {
		  	 BMT_status.UI_SOC--;
			 timer_counter=1;
		  }
		  else
		  {
			 timer_counter++;
			 return resetBatteryMeter;
		  }
			  
  		  resetBatteryMeter = KAL_TRUE;

		   battery_xlog_printk(BAT_LOG_CRTI, "[nPercent] ZCV %d <= nPercent_ZCV %d, UI_SOC=%d., tracking UI_SOC=%d \n", 
                BMT_status.ZCV, BMT_status.nPercent_ZCV, BMT_status.UI_SOC, BMT_status.nPrecent_UI_SOC_check_point);
	}
	else if ( (BMT_status.ZCV > BMT_status.nPercent_ZCV)&&(BMT_status.UI_SOC==BMT_status.nPrecent_UI_SOC_check_point) )
    {
    	//UI less than 15 , but fuel gague is more than 15, hold UI 15%
    	timer_counter=(NPERCENT_TRACKING_TIME/BAT_TASK_PERIOD);
		resetBatteryMeter = KAL_TRUE;
		
        battery_xlog_printk(BAT_LOG_CRTI, "[nPercent] ZCV %d > BMT_status.nPercent_ZCV %d and UI SOC=%d, then keep %d. \n", 
            BMT_status.ZCV, BMT_status.nPercent_ZCV, BMT_status.UI_SOC, BMT_status.nPrecent_UI_SOC_check_point);
 	}
	else
	{
		timer_counter=(NPERCENT_TRACKING_TIME/BAT_TASK_PERIOD);
	}
#endif	
	return resetBatteryMeter;

}

static kal_bool mt_battery_0Percent_tracking_check(void)
{
	kal_bool resetBatteryMeter = KAL_TRUE;
	 
	if(BMT_status.UI_SOC <= 0)
	{
		BMT_status.UI_SOC=0;
	} else {
		if (BMT_status.bat_vol > SYSTEM_OFF_VOLTAGE && BMT_status.UI_SOC > 1) {
			BMT_status.UI_SOC--;
		} else if (BMT_status.bat_vol <= SYSTEM_OFF_VOLTAGE) {
			BMT_status.UI_SOC--;
		}
	}

	battery_xlog_printk(BAT_LOG_CRTI, "0Percent, VBAT < %d UI_SOC=%d\r\n", SYSTEM_OFF_VOLTAGE, BMT_status.UI_SOC);                

	return resetBatteryMeter;	
}


static void mt_battery_Sync_UI_Percentage_to_Real(void)
{
	static kal_uint32 timer_counter = 0;	
	
	if( (BMT_status.UI_SOC > BMT_status.SOC) && ((BMT_status.UI_SOC!=1)) ) {
#if !defined (SYNC_UI_SOC_IMM)
		//reduce after xxs
		if(timer_counter == (SYNC_TO_REAL_TRACKING_TIME/BAT_TASK_PERIOD)) {
			BMT_status.UI_SOC--;
			timer_counter = 0;
		} else {
			timer_counter ++;
		}
#else
		BMT_status.UI_SOC--;
#endif
		battery_xlog_printk(BAT_LOG_CRTI, "[Sync_Real] UI_SOC=%d, SOC=%d, counter = %d\n", 
                      BMT_status.UI_SOC, BMT_status.SOC, timer_counter);
	} else {
		timer_counter = 0;
		BMT_status.UI_SOC = BMT_status.SOC;
	}

	if(BMT_status.UI_SOC <= 0 ) {
		BMT_status.UI_SOC=1;
		battery_xlog_printk(BAT_LOG_CRTI, "[Battery]UI_SOC get 0 first (%d)\r\n", BMT_status.UI_SOC);
	}
}
#endif

static void check_battery_request_poweroff(struct battery_data *bat_data)
{
    UINT32  instant_bat_vol=0;   
    static kal_int8 shutdown_instant_times = 0;
	static kal_int8 shutdown_average_times = 0;	
	instant_bat_vol = BAT_Get_Battery_Voltage(0);

    if(instant_bat_vol <= SHUTDOWN_INSTANT_LOW_POWER)
    {
        shutdown_instant_times++;
		printk("SHUT_DOWN,instant_bat_vol=%d <SHUTDOWN_INSTANT_LOW_POWER, times = %d, request power off\n", instant_bat_vol, shutdown_instant_times);
        if(shutdown_instant_times >= 3)
        {
            shutdown_instant_times = 0;
			g_rtc_rst_flag = 4;
        }        
    }
	else if(BMT_status.bat_vol <= SHUTDOWN_AVERAGE_LOW_POWER)
	{
        shutdown_average_times++;
		printk("SHUT_DOWN,BMT_status.bat_vol=%d <SHUTDOWN_AVERAGE_LOW_POWER, times = %d, request power off\n", BMT_status.bat_vol, shutdown_average_times);       
		if(shutdown_average_times >= 3)
		{
			//bat_data->battery_request_poweroff=1;
			shutdown_average_times = 0;
			g_rtc_rst_flag = 1;
		}		   	
	}
	else
	{
		shutdown_instant_times = 0;
		shutdown_average_times = 0;
		g_rtc_rst_flag = 0;
		
		if(g_Charging_Over_Time == 1)//for 6 hourse  overtime
		{
			bat_data->battery_request_poweroff = 2; 
		}	        
		else
		{   
			bat_data->battery_request_poweroff=0;
		}
	}
}

static void battery_rtc_soc_save(struct battery_data *bat_data)
{
	if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT)
	{
		xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[mt6752_battery_update]set_rtc_spare_fg_value point = %d, g_boot_mode = %d KERNEL POWER OFF CHARGING!\r\n", bat_volt_check_point, g_boot_mode);
		set_rtc_spare_fg_value(bat_volt_check_point);
	}
	else	
	{
		if((BMT_status.bat_vol > SHUTDOWN_WAIT_LOW_POWER) &&(bat_volt_check_point <= 1))
		{
			bat_volt_check_point = 1;
			set_rtc_spare_fg_value(bat_volt_check_point);
			printk("SHUT_DOWN,BMT_status.bat_vol=%d > SHUTDOWN_WAIT_LOW_POWER,wait for\n", BMT_status.bat_vol); 
			return;
		}
		
		if(g_rtc_rst_flag == 4)
		{
			if(bat_volt_check_point > 0)
			{
				bat_volt_check_point--;
				set_rtc_spare_fg_value(bat_volt_check_point);
			}
			else
			{
				bat_volt_check_point = 0;
				bat_data->battery_request_poweroff=4;
				set_rtc_spare_fg_value(1);
			}
		}
		else if(g_rtc_rst_flag == 1)
		{
			if(bat_volt_check_point > 0)
			{
				bat_volt_check_point--;
				set_rtc_spare_fg_value(bat_volt_check_point);
			}
			else
			{
				bat_volt_check_point = 0;
				bat_data->battery_request_poweroff=1;
				set_rtc_spare_fg_value(1);
			}
		}
		else
		{
			set_rtc_spare_fg_value(bat_volt_check_point);
		}
		xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[mt6752_battery_update]set_rtc_spare_fg_value gBAT_counter_off = %d, point = %d, g_rtc_rst_flag = %d,request = %d\r\n",gBAT_counter_off, bat_volt_check_point, g_rtc_rst_flag,bat_data->battery_request_poweroff);
	}

}

static void battery_update(struct battery_data *bat_data)
{
	struct power_supply *bat_psy = &bat_data->psy;
	int i;
	bat_data->BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION;
	bat_data->fastcharger = opchg_get_prop_fast_chg_started();
	bat_data->charge_technology = POWER_SUPPLY_CHARGE_TECHNOLOGY_FAST;
	bat_data->BAT_batt_vol = BMT_status.bat_vol;
	bat_data->BAT_batt_temp= BMT_status.temperature * 10;
	bat_data->BAT_PRESENT = BMT_status.bat_exist;
	
	if(KAL_TRUE == struExternalBMT.bOverBatVol)
	{
		bat_data->BAT_HEALTH = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	}
	else
	{
		//bat_data->BAT_HEALTH = struExternalBMT.enBatStatus;
		bat_data->BAT_HEALTH = kernel_get_bat_health();
	}

	gSyncPercentage=0;

	if(bat_volt_check_point == 100)
	{
		g_soc_sync_down_times = DEFAULT_SOC_SYNC_DOWN_TIME_FAST_300;
		g_soc_sync_up_times = DEFAULT_SOC_SYNC_UP_TIME_60;
	}
	else if(bat_volt_check_point >= 95)
	{
		g_soc_sync_down_times = DEFAULT_SOC_SYNC_DOWN_TIME_FAST_150;
		g_soc_sync_up_times = DEFAULT_SOC_SYNC_UP_TIME_60;
	}
	else
	{
		g_soc_sync_down_times = DEFAULT_SOC_SYNC_DOWN_TIME_NORMAL;
		g_soc_sync_up_times = DEFAULT_SOC_SYNC_UP_TIME_10;
	}
	
	if(BMT_status.charger_exist && BMT_status.bat_exist
        && ((KAL_TRUE == BMT_status.bat_full )||(g_bat_full_user_view)))
	{
        if( gForceADCsolution == 1 )
		{
            
			bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_FULL;
			if(BMT_status.bat_vol >= OPPO_BAT_VOLT_100)	
			{
				for (i=0; i<BATTERY_AVERAGE_SIZE; i++) 
				{
					batterySOCBuffer[i] = 100; 	
				}
				batterySOCSum = 100 * BATTERY_AVERAGE_SIZE;
			#if 1	
				if(g_soc_sync_time >= g_soc_sync_up_times)  // 1 MINS
				{
					g_soc_sync_time = 0;
					bat_volt_check_point++;
				}  
				else
				{
					g_soc_sync_time+=1; //6; //10;   
				}
				if(bat_volt_check_point>=100)
				{
					bat_volt_check_point=100;
					//bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_FULL;						
				}
			#endif
				//bat_volt_check_point = Battery_Percent_100;
			}
			else
			{
				if ( BMT_status.SOC > bat_volt_check_point ) 
				{						
					if(g_soc_sync_time >= g_soc_sync_up_times)  // 1 MINS
					{
						g_soc_sync_time = 0;
						bat_volt_check_point++;
								
					}  
					else
					{
						g_soc_sync_time+=1; //6; //10;   
					}
				}
				
				if(bat_volt_check_point>=100)
				{
					bat_volt_check_point=100;					
				}
			}
			//bat_data->BAT_CAPACITY = bat_volt_check_point;
        }
        else
        {
            gSyncPercentage=1;
			
	//		if((struExternalBMT.enBatStatus == BATTERY_STATUS__GOOD) || (struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_12_16))
			//modified by PengNan for NEW_CHARGER_STARDARD_V2_8 2015.12.3
			if((struExternalBMT.enBatStatus == BATTERY_STATUS__GOOD) || (struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_12_16) || (struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_0_5) || (struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_5_12))
			{
				if(g_soc_sync_time >= g_soc_sync_up_times)  // 1 MINS
	    		{
	    			g_soc_sync_time = 0;
					bat_volt_check_point++;
	    		}  
	    		else
	    		{
	    			g_soc_sync_time+=1; //6; //10;   
	    		}
				
				if(bat_volt_check_point>=100)
				{
					bat_volt_check_point=100;					
				}
				
				if(bat_volt_check_point==100){
					bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_FULL;
				}
				else{
					if(bat_volt_check_point>=100){
						bat_volt_check_point=100;					
					}
					bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_CHARGING;
				}
				
			}
			else
			{
				bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_FULL;	
			}
			//bat_data->BAT_CAPACITY = bat_volt_check_point;
			
			if (Enable_BATDRV_LOG == 1) {
				xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] In FULL Range (%d)\r\n", bat_volt_check_point);
			}
			if (Enable_BATDRV_LOG == 1) {
				xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery_SyncRecharge] In recharging state, do not sync FG\r\n");
			}
        }
	}
    else if(BMT_status.charger_exist && BMT_status.bat_exist&&(BQ24196_CHR_FAIL!= BMT_status.bat_charging_state)&& (g_Charging_Over_Time != 1))
	{
         bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_CHARGING;
		 if( gForceADCsolution == 1 )
         {
            //modified here to fix :chging SOC up too quick!
            if ( BMT_status.SOC > bat_volt_check_point ) 
            {						
    			if(g_soc_sync_time >= g_soc_sync_up_times)  // 1 MINS
    			{
    				g_soc_sync_time = 0;
    				bat_volt_check_point++;
    						
    			}  
    			else
    			{
    				g_soc_sync_time+=1; //6; //10;   
    			}
    	    }
			else if(bat_volt_check_point == BMT_status.SOC)
			{
				if (Enable_BATDRV_LOG == 1) {
				xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] Can sync due to bat_volt_check_point=%d, BMT_status.SOC=%ld\r\n", 
					bat_volt_check_point, BMT_status.SOC);
				}
			}
			else
			{
				if(g_soc_sync_time>=g_soc_sync_down_times)
				{
					g_soc_sync_time=0;
					bat_volt_check_point--;
				}
				else
				{
					g_soc_sync_time+=1;
				}
				xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] g_soc_sync_time = %d,point = %d,soc = %d, g_soc_sync_down_times = %d\r\n",g_soc_sync_time,bat_volt_check_point,BMT_status.SOC,g_soc_sync_down_times);
			}
			/*
			if(bat_volt_check_point>=100) 
			{
			    bat_volt_check_point=99;  
			}
			*/
			//bat_data->BAT_CAPACITY = bat_volt_check_point;
        }
        else
		{					
				/*bat_volt_check_point=99;
				//BMT_status.SOC=99;
				gSyncPercentage=1;
				*/
				//if (Enable_BATDRV_LOG == 1) {
				//}
				if(bat_volt_check_point == BMT_status.SOC)
				{
					gSyncPercentage=0;
					if (Enable_BATDRV_LOG == 1) {
					xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] Can sync due to bat_volt_check_point=%d, BMT_status.SOC=%ld\r\n", 
						bat_volt_check_point, BMT_status.SOC);
					}
				}
				else
				{
					gSyncPercentage=1;
					if ( BMT_status.SOC > bat_volt_check_point )
					{
						if(g_soc_sync_time>g_soc_sync_up_times)
						{
							g_soc_sync_time=0;
							bat_volt_check_point++;
						}
						else
						{
							g_soc_sync_time+=1;
						}
					}
					else
					{
						if(g_soc_sync_time>=g_soc_sync_down_times)
						{
							g_soc_sync_time=0;
							bat_volt_check_point--;
						}
						else
						{
							g_soc_sync_time+=1;
						}
					}
					if (Enable_BATDRV_LOG == 1) {
					xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] Keep UI due to bat_volt_check_point=%d, BMT_status.SOC=%ld\r\n", 
						bat_volt_check_point, BMT_status.SOC);
					}
				}
			/*
			if(bat_volt_check_point >= 100 )
			{					
				//bat_volt_check_point=100;
				bat_volt_check_point=99;
				//BMT_status.SOC=99;
				gSyncPercentage=1;
				
				//if (Enable_BATDRV_LOG == 1) {
					xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] Use gas gauge : gas gague get 100 first (%d)\r\n", bat_volt_check_point);
				//}
			}
			*/
			//bat_data->BAT_CAPACITY = bat_volt_check_point;
        }   			
	}
	// Only Battery or only charger 
	else  // if(!(BMT_status.charger_exist) && BMT_status.bat_exist)
	{
		bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_NOT_CHARGING;
#if 0
        if(BMT_status.bat_vol <=SYSTEM_OFF_CRITICAL_VOLTAGE)  //power off now
        {
            if( gForceADCsolution == 1 )
            {
				bat_volt_check_point--;
				if(bat_volt_check_point <= 0)
				{
					bat_volt_check_point=0;
				}
                xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[BAT BATTERY] VBAT < %d mV : Android will Power Off System !!\r\n", SYSTEM_OFF_CRITICAL_VOLTAGE);                              
            }
            else
            {
                gSyncPercentage=1;				
				bat_volt_check_point--;
				if(bat_volt_check_point <= 0)
				{
					bat_volt_check_point=0;
				}
                g_Calibration_FG = 0;
        		battery_meter_reset();
    			gFG_DOD0=100-bat_volt_check_point;
    			gFG_DOD1=gFG_DOD0;
    			BMT_status.SOC=bat_volt_check_point;
				//bat_data->BAT_CAPACITY = bat_volt_check_point;
				printk("[Battery]BMT_status.bat_vol <=SYSTEM_OFF_CRITICAL_VOLTAGE VBAT < %d mV (%d, gFG_DOD0=%d)\r\n", SYSTEM_OFF_CRITICAL_VOLTAGE, bat_volt_check_point,gFG_DOD0);				
            }
            
        }
		else if(BMT_status.bat_vol <= SYSTEM_OFF_VOLTAGE_HIGH)
    	{
    	    gSyncPercentage=1;
    	    if(g_bimax_dischg == KAL_TRUE)
    	    {
    	        g_bimax_dischg=KAL_FALSE;
    	        g_soc_sync_time=0;
    	        bat_volt_check_point=BMT_status.SOC;
    	    }
    	    else if(bat_volt_check_point>0)
    	    {
    	        if(g_soc_sync_time>=g_soc_sync_down_times)
    	        {
    	            g_soc_sync_time=0;
    	            bat_volt_check_point--;
    	        }
    	        else
    	        {
    	            g_soc_sync_time+=1;
    	        }
				printk("[Battery]BMT_status.bat_vol <=SYSTEM_OFF_VOLTAGE_HIGH------ VBAT < %d mV-------point = %d, g_soc_sync_down_times = %d\r\n", SYSTEM_OFF_VOLTAGE_HIGH, bat_volt_check_point, g_soc_sync_down_times);
    	    }
    	    else
    	    {
    	        bat_volt_check_point = 0;
    	    }
			
    	}
		else if ( (gFG_voltage <= gFG_15_vlot)&&(gForceADCsolution==0)&&(bat_volt_check_point>=15) )
		{
			gSyncPercentage=1;			
			if(gBAT_counter_15==0)
			{
				bat_volt_check_point--;
				gBAT_counter_15=1;
			}		
			else  //VOL >SYSTEM_OFF_VOL
			{
				gBAT_counter_15=0;
			}
			g_Calibration_FG = 0;
    		battery_meter_reset();
			gFG_DOD0=100-bat_volt_check_point;
			gFG_DOD1=gFG_DOD0;
			BMT_status.SOC=bat_volt_check_point;
			//bat_data->BAT_CAPACITY = bat_volt_check_point;
			xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] FG_VBAT <= %d, then SOC run to 15. (SOC=%ld,Point=%d,D1=%d,D0=%d)\r\n", 
				gFG_15_vlot, BMT_status.SOC, bat_volt_check_point, gFG_DOD1, gFG_DOD0);
		}
		else if ( (gFG_voltage > gFG_15_vlot)&&(gForceADCsolution==0)&&(bat_volt_check_point==15) )
		{
			gSyncPercentage=1;
			gBAT_counter_15=1;
			g_Calibration_FG = 0;
    		battery_meter_reset();
			gFG_DOD0=100-bat_volt_check_point;
			gFG_DOD1=gFG_DOD0;
			BMT_status.SOC=bat_volt_check_point;
			//bat_data->BAT_CAPACITY = bat_volt_check_point;
			xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] FG_VBAT(%d) > gFG_15_vlot(%d) and FG_report=15, then UI(%d) keep 15. (D1=%d,D0=%d)\r\n", 
				gFG_voltage, gFG_15_vlot, bat_volt_check_point, gFG_DOD1, gFG_DOD0);
		}
		else  
#endif
		{
			gBAT_counter_15=1;
			if ( BMT_status.SOC < bat_volt_check_point ) 
			{

    			if(g_bimax_dischg == KAL_TRUE)
        	    {
        	        g_bimax_dischg=KAL_FALSE;
        	        g_soc_sync_time=0;
        	        bat_volt_check_point=BMT_status.SOC;
        	    }
        	    else
        	    {
        	        gSyncPercentage=1;
        	        if(g_soc_sync_time>=g_soc_sync_down_times)
        	        {
        	            g_soc_sync_time=0;
        	            bat_volt_check_point--;
        	        }
        	        else
        	        {
        	            g_soc_sync_time+=1;
        	        }
        	    }
				xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] g_soc_sync_time = %d,point = %d,soc = %d, g_soc_sync_down_times = %d\r\n",g_soc_sync_time,bat_volt_check_point,BMT_status.SOC,g_soc_sync_down_times);
			}
		}

		//bat_data->BAT_CAPACITY = bat_volt_check_point;
	}
	if(BMT_status.charger_exist == KAL_FALSE)
		check_battery_request_poweroff(bat_data);
	
	// set RTC SOC to 2 to avoid SOC jump in charger boot.
	battery_rtc_soc_save(bat_data);
	mt_battery_update_EM(bat_data);
	power_supply_changed(bat_psy);    
}

void update_charger_info(int wireless_state)
{
    #if defined(CONFIG_POWER_VERIFY)
    battery_xlog_printk(BAT_LOG_CRTI, "[update_charger_info] no support\n");
    #else
    g_wireless_state = wireless_state;
    battery_xlog_printk(BAT_LOG_CRTI, "[update_charger_info] get wireless_state=%d\n",
        wireless_state);

    wake_up_bat();
    #endif
}

static void wireless_update(struct wireless_data *wireless_data)
{
    struct power_supply *wireless_psy = &wireless_data->psy;

    if( BMT_status.charger_exist == KAL_TRUE || g_wireless_state)
    {         
        if ( (BMT_status.charger_type == WIRELESS_CHARGER) || g_wireless_state)
        {
            wireless_data->WIRELESS_ONLINE = 1;        
            wireless_psy->type = POWER_SUPPLY_TYPE_WIRELESS;
        }
        else
        {
            wireless_data->WIRELESS_ONLINE = 0;
        }
    }
    else
    {
        wireless_data->WIRELESS_ONLINE = 0;        
    }
    
    power_supply_changed(wireless_psy);    
}

static void ac_update(struct ac_data *ac_data)
{
    struct power_supply *ac_psy = &ac_data->psy;
	if( pmic_chrdet_status() == KAL_TRUE )
	{		 
		if ( (BMT_status.charger_type == NONSTANDARD_CHARGER) || 
			 (BMT_status.charger_type == STANDARD_CHARGER) 	  || 
			 (BMT_status.charger_type == APPLE_2_1A_CHARGER)  ||
			 (BMT_status.charger_type == APPLE_1_0A_CHARGER)  ||
			 (BMT_status.charger_type == APPLE_0_5A_CHARGER)
			)
		{
			ac_data->AC_ONLINE = 1; 	   
			ac_psy->type = POWER_SUPPLY_TYPE_MAINS;
			BMT_status.charger_exist = KAL_TRUE;
		}
		else
    	{
    		ac_data->AC_ONLINE = 0; 
			BMT_status.charger_exist = KAL_FALSE;	   
    	}
	}
	else
	{
		ac_data->AC_ONLINE = 0; 
		BMT_status.charger_exist = KAL_FALSE;	   
	}
    power_supply_changed(ac_psy);    
}

static void usb_update(struct usb_data *usb_data)
{
    struct power_supply *usb_psy = &usb_data->psy;
	if( pmic_chrdet_status() == KAL_TRUE )		
    {   
        if(BMT_status.charger_type == CHARGING_HOST)//for standard charger before poweron
        {
			mt_charger_type_detection();
			 if((BMT_status.charger_type != STANDARD_HOST) && (BMT_status.charger_type != CHARGING_HOST) )
			 {
			    mt_usb_disconnect();
			 }
			 #ifdef NEW_CHGING_TIME
			 if(BMT_status.charger_type == STANDARD_CHARGER)
				max_charging_time_kernel = 10*60*60;//s			
			 #endif
         }
		if ( (BMT_status.charger_type == STANDARD_HOST) ||
			 (BMT_status.charger_type == CHARGING_HOST)		)
		{
		    usb_data->USB_ONLINE = 1;            
		    usb_psy->type = POWER_SUPPLY_TYPE_USB;            
		}
		else
        {
    		usb_data->USB_ONLINE = 0;
        }  
    }
    else
    {
		usb_data->USB_ONLINE = 0;
    }   

    power_supply_changed(usb_psy); 
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////
//// Battery Temprature Parameters and functions
///////////////////////////////////////////////////////////////////////////////////////////
kal_bool pmic_chrdet_status(void)
{
    if( upmu_is_chr_det() == KAL_TRUE )    
    {
        return KAL_TRUE;
    }
    else
    {
        if(opchg_get_prop_fast_chg_started() == true){
			battery_xlog_printk(BAT_LOG_CRTI, "[pmic_chrdet_status] No charger,fast chg started\r\n");
			return KAL_TRUE;
		}else {
			battery_xlog_printk(BAT_LOG_CRTI, "[pmic_chrdet_status] No charger\r\n");
			return KAL_FALSE;
		}
        
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
//// Pulse Charging Algorithm 
///////////////////////////////////////////////////////////////////////////////////////////
kal_bool bat_is_charger_exist(void)
{
	return get_charger_detect_status();
}


kal_bool bat_is_charging_full(void)
{
	if((BMT_status.bat_full == KAL_TRUE) && (BMT_status.bat_in_recharging_state == KAL_FALSE))
		return KAL_TRUE;
	else
		return KAL_FALSE;
}


kal_uint32 bat_get_ui_percentage(void)
{
	//  for plugging out charger in recharge phase, using SOC as UI_SOC
	if(chr_wake_up_bat == KAL_TRUE)
		return BMT_status.SOC;
	else
		return bat_volt_check_point;
}

/* Full state --> recharge voltage --> full state */
kal_uint32 bat_is_recharging_phase(void)
{
	return (BMT_status.bat_in_recharging_state || BMT_status.bat_full == KAL_TRUE);
}


int get_bat_charging_current_level(void)
{
    CHR_CURRENT_ENUM charging_current;

	battery_charging_control(CHARGING_CMD_GET_CURRENT,&charging_current);

	return charging_current;
}

unsigned long BAT_Get_Battery_Voltage(int polling_mode)
{
    unsigned long ret_val = 0;

#if defined(CONFIG_POWER_EXT)
	ret_val = 4000;
#else	
    ret_val=battery_meter_get_battery_voltage(KAL_FALSE);
#endif    

    return ret_val;
}


static void mt_battery_average_method_init(kal_uint32 *bufferdata, kal_uint32 data, kal_int32 *sum)
{
	kal_uint32 i;
	static kal_bool batteryBufferFirst = KAL_TRUE;
	static kal_bool previous_charger_exist = KAL_FALSE;
	static kal_bool previous_in_recharge_state = KAL_FALSE;
	static kal_uint8 index=0;

	/* reset charging current window while plug in/out {*/
	if(BMT_status.charger_exist == KAL_TRUE)
	{
		if(previous_charger_exist == KAL_FALSE)
		{
			batteryBufferFirst = KAL_TRUE;
			previous_charger_exist = KAL_TRUE;
			if (BMT_status.charger_type == STANDARD_CHARGER) {
				data = AC_CHARGER_CURRENT / 100;
			} else if (BMT_status.charger_type == CHARGING_HOST) {
				data = CHARGING_HOST_CHARGER_CURRENT / 100;
			} else if (BMT_status.charger_type == NONSTANDARD_CHARGER)
				data = NON_STD_AC_CHARGER_CURRENT / 100;		//mA
			else	// USB
				data = USB_CHARGER_CURRENT / 100;		//mA
		}		
		else if((previous_in_recharge_state == KAL_FALSE) && (BMT_status.bat_in_recharging_state == KAL_TRUE))
		{
			batteryBufferFirst = KAL_TRUE;
			if (BMT_status.charger_type == STANDARD_CHARGER) {		
				data = AC_CHARGER_CURRENT / 100;
			} else if (BMT_status.charger_type == CHARGING_HOST) {
				data = CHARGING_HOST_CHARGER_CURRENT / 100;
			} else if (BMT_status.charger_type == NONSTANDARD_CHARGER)
				data = NON_STD_AC_CHARGER_CURRENT / 100;		//mA
			else	// USB
				data = USB_CHARGER_CURRENT / 100;		//mA
		}

		previous_in_recharge_state = BMT_status.bat_in_recharging_state;
	}
	else
	{
		if(previous_charger_exist == KAL_TRUE)
		{
			batteryBufferFirst = KAL_TRUE;
			previous_charger_exist = KAL_FALSE;
			data = 0;
		}
	}
	/* reset charging current window while plug in/out }*/

	battery_xlog_printk(BAT_LOG_FULL, "batteryBufferFirst =%d, data= (%d) \n", batteryBufferFirst, data);
	
	if(batteryBufferFirst == KAL_TRUE)
	{
		for (i=0; i<BATTERY_AVERAGE_SIZE; i++)
		{
            bufferdata[i] = data;            
		}

		*sum = data * BATTERY_AVERAGE_SIZE;
	}

	index++;
	if(index >= BATTERY_AVERAGE_DATA_NUMBER)
	{
		index = BATTERY_AVERAGE_DATA_NUMBER;
       	batteryBufferFirst = KAL_FALSE;	
	}	
}

static kal_uint32 mt_battery_average_method(kal_uint32 *bufferdata, kal_uint32 data, kal_int32 *sum, kal_uint8 batteryIndex)
{
	kal_uint32 avgdata;

	mt_battery_average_method_init(bufferdata, data, sum);

	*sum -=	bufferdata[batteryIndex];
	*sum +=  data;
	bufferdata[batteryIndex] = data;
    avgdata = (*sum)/BATTERY_AVERAGE_SIZE;

	battery_xlog_printk(BAT_LOG_FULL, "bufferdata[%d]= (%d) \n", batteryIndex,bufferdata[batteryIndex]);
	return avgdata;
}

void mt_battery_GetBatteryData(void)
{ 
	kal_uint32 bat_vol, charger_vol, Vsense, ZCV; 
	kal_int32 ICharging, temperature, temperatureR, temperatureV, SOC;
	static kal_int32 bat_sum, icharging_sum, temperature_sum;
	static kal_int32 batteryVoltageBuffer[BATTERY_AVERAGE_SIZE];
	static kal_int32 batteryCurrentBuffer[BATTERY_AVERAGE_SIZE];
	static kal_int32 batteryTempBuffer[BATTERY_AVERAGE_SIZE];
	static kal_uint8 batteryIndex = 0;
	static kal_int32 previous_SOC = -1;
	
	bat_vol = battery_meter_get_battery_voltage(KAL_TRUE);
	Vsense = battery_meter_get_VSense();
	if( pmic_chrdet_status() == KAL_TRUE ) {
		ICharging = battery_meter_get_charging_current();
		charger_vol = battery_meter_get_charger_voltage();
	} else {
		ICharging = 0;
		charger_vol = 0;
	}
	temperature = battery_meter_get_battery_temperature();
	temperatureV = battery_meter_get_tempV();
	temperatureR = battery_meter_get_tempR(temperatureV);

	if(bat_meter_timeout == KAL_TRUE || bat_spm_timeout == TRUE)
	{
		SOC = battery_meter_get_battery_percentage();
		if (bat_spm_timeout == true)
			BMT_status.UI_SOC = battery_meter_get_battery_percentage();
			
		bat_meter_timeout = KAL_FALSE;
		bat_spm_timeout = FALSE;
	}
	else
	{
		if (previous_SOC == -1)
			SOC = battery_meter_get_battery_percentage();
		else
			SOC = previous_SOC;		
	}

    
	ZCV = battery_meter_get_battery_zcv();

	BMT_status.ICharging = mt_battery_average_method(&batteryCurrentBuffer[0],ICharging, &icharging_sum, batteryIndex);	
#if 1
	if (previous_SOC == -1 && bat_vol <= V_0PERCENT_TRACKING)
	{
		battery_xlog_printk(BAT_LOG_CRTI, "battery voltage too low, use ZCV to init average data.\n");
		BMT_status.bat_vol = mt_battery_average_method(&batteryVoltageBuffer[0],ZCV, &bat_sum, batteryIndex);
	}
	else
	{
		BMT_status.bat_vol = mt_battery_average_method(&batteryVoltageBuffer[0],bat_vol, &bat_sum, batteryIndex);
	}
#else
	BMT_status.bat_vol = mt_battery_average_method(&batteryVoltageBuffer[0],bat_vol, &bat_sum, batteryIndex);
#endif
	BMT_status.temperature = temperature;
	//BMT_status.temperature = mt_battery_average_method(&batteryTempBuffer[0],temperature, &temperature_sum, batteryIndex);
	BMT_status.Vsense = Vsense;
	BMT_status.charger_vol = charger_vol;
	check_charger_off_vol = BMT_status.charger_vol;	
	BMT_status.temperatureV = temperatureV;
	BMT_status.temperatureR = temperatureR;
	BMT_status.SOC = SOC;	
	BMT_status.ZCV = ZCV;

	if(BMT_status.charger_exist == KAL_FALSE)
	{
		if(BMT_status.SOC > previous_SOC && previous_SOC >= 0)
			BMT_status.SOC = previous_SOC;
	}

	previous_SOC = BMT_status.SOC;
	
	batteryIndex++;
    if (batteryIndex >= BATTERY_AVERAGE_SIZE)
        batteryIndex = 0;
	

	if(g_battery_soc_ready == KAL_FALSE) {
		BMT_status.UI_SOC = BMT_status.SOC;
		g_battery_soc_ready = KAL_TRUE; 
	}
	battery_xlog_printk(BAT_LOG_CRTI, "AvgVbat=(%d),bat_vol=(%d),AvgI=(%d),I=(%d),VChr=(%d),AvgT=(%d),T=(%d),pre_SOC=(%d),SOC=(%d),ZCV=(%d)\n",
		BMT_status.bat_vol,bat_vol,BMT_status.ICharging,ICharging,BMT_status.charger_vol,BMT_status.temperature,temperature,previous_SOC,BMT_status.SOC,BMT_status.ZCV);
	
	
	BMT_status.ICharging = opchg_get_prop_current_now();// good
	BMT_status.bat_vol = opchg_get_prop_battery_voltage_now()/1000;	
	BMT_status.temperature = opchg_get_prop_batt_temp()/10;

	BMT_status.SOC = opchg_get_prop_batt_capacity();
	
	if (bat_volt_cp_flag == 0) 
	{
		bat_volt_cp_flag = 1;
		g_rtc_soc = get_rtc_spare_fg_value();
		if(((g_rtc_soc != 0) && ((abs(g_rtc_soc-BMT_status.SOC)) <= 20))  
		 || ((g_rtc_soc != 0) &&(g_boot_reason == BR_WDT_BY_PASS_PWK || g_boot_reason == BR_WDT || g_boot_reason == BR_TOOL_BY_PASS_PWK || g_boot_reason == BR_2SEC_REBOOT || g_boot_mode == RECOVERY_BOOT))) 
		{
			bat_volt_check_point = g_rtc_soc;
		}
		else
		{
			bat_volt_check_point = BMT_status.SOC;
		}
		BMT_status.UI_SOC= bat_volt_check_point;
	}		
	
}



static PMU_STATUS mt_battery_CheckChargerVoltage(void)
{
	PMU_STATUS status = PMU_STATUS_OK;


    if( BMT_status.charger_exist == KAL_TRUE)
    {
        #if (V_CHARGER_ENABLE == 1)
        if (BMT_status.charger_vol <= V_CHARGER_MIN )
        {
           battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY]Charger under voltage!!\r\n");                    
            BMT_status.bat_charging_state = CHR_ERROR;
            status = PMU_STATUS_FAIL;        
        }
        #endif        

        if ( BMT_status.charger_vol >= V_CHARGER_MAX )
        {
            battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY]Charger over voltage !!\r\n");                    
            BMT_status.charger_protect_status = charger_OVER_VOL;
            BMT_status.bat_charging_state = CHR_ERROR;
            status = PMU_STATUS_FAIL;        
        }            
    }

	return status;
}


static PMU_STATUS mt_battery_CheckChargingTime(void)
{
    PMU_STATUS status = PMU_STATUS_OK;

    if( (g_battery_thermal_throttling_flag==2) || (g_battery_thermal_throttling_flag==3) )
    {
		battery_xlog_printk(BAT_LOG_FULL, "[TestMode] Disable Safty Timer. bat_tt_enable=%d, bat_thr_test_mode=%d, bat_thr_test_value=%d\n", 
			g_battery_thermal_throttling_flag, battery_cmd_thermal_test_mode, battery_cmd_thermal_test_mode_value);
		
    }
    else
    {    
        /* Charging OT */
        if(BMT_status.total_charging_time >= MAX_CHARGING_TIME)
        {
   		    battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Charging Over Time. \n");
			   			
            status = PMU_STATUS_FAIL;
        }
    }    

	return status;
        
}

#if defined(STOP_CHARGING_IN_TAKLING)
static PMU_STATUS mt_battery_CheckCallState(void)
{
	PMU_STATUS status = PMU_STATUS_OK;
	
	if((g_call_state == CALL_ACTIVE) && (BMT_status.bat_vol > V_CC2TOPOFF_THRES))
		status = PMU_STATUS_FAIL;

	return status;
}
#endif


static void mt_battery_notify_flag_check(void)
{
	if(g_BatteryNotifyCode & (1 << Notify_Chging_OverTime))
	{
		g_NotifyFlag = Notify_Chging_OverTime;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Charger_Over_Vol))
	{
		g_NotifyFlag = Notify_Charger_Over_Vol;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Charger_Low_Vol))
	{
		g_NotifyFlag = Notify_Charger_Low_Vol;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Bat_Over_Temp))
	{
		g_NotifyFlag = Notify_Bat_Over_Temp;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Bat_Low_Temp))
	{
		g_NotifyFlag = Notify_Bat_Low_Temp;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Bat_Not_Connect))
	{
		g_NotifyFlag = Notify_Bat_Not_Connect;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Bat_Over_Vol))
	{
		g_NotifyFlag = Notify_Bat_Over_Vol;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Bat_Full_Pre_High_Temp))
	{
		g_NotifyFlag = Notify_Bat_Full_Pre_High_Temp;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Bat_Full_Pre_Low_Temp2))
	{
		g_NotifyFlag = Notify_Bat_Full_Pre_Low_Temp2;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Bat_Full_THIRD_BATTERY))
	{
		g_NotifyFlag = Notify_Bat_Full_THIRD_BATTERY;
	}
	else if(g_BatteryNotifyCode & (1 << Notify_Bat_Full))
	{
		g_NotifyFlag = Notify_Bat_Full;
	}
	else
	{
		g_NotifyFlag = 0;
	}
}

static void mt_battery_notify_TotalChargingTime_check(void)
{
#if defined(BATTERY_NOTIFY_CASE_0005_TOTAL_CHARGINGTIME)
	#if 0
    if( (g_battery_thermal_throttling_flag==2) || (g_battery_thermal_throttling_flag==3) )
    {
        battery_xlog_printk(BAT_LOG_FULL, "[TestMode] Disable Safty Timer : no UI display\n");
    }
    else
    {
        if(BMT_status.total_charging_time >= MAX_CHARGING_TIME)
        //if(BMT_status.total_charging_time >= 60) //test
        {
            g_BatteryNotifyCode |= 0x0010;
            battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Charging Over Time\n");
        }
        else
        {
            g_BatteryNotifyCode &= ~(0x0010);
        }
    }
	#endif
	if(1 == g_Charging_Over_Time)
	{
		g_BatteryNotifyCode |= 1 << Notify_Chging_OverTime;
		if (Enable_BATDRV_LOG == 1)
		{
			  printk("[BATTERY] Charging is OverTime!Notify \n");
		}
	}
    
    //battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] BATTERY_NOTIFY_CASE_0005_TOTAL_CHARGINGTIME (%x)\n", g_BatteryNotifyCode);
#endif
}


static void mt_battery_notify_VBat_check(void)
{
#if defined(BATTERY_NOTIFY_CASE_0004_VBAT)

    if(KAL_TRUE == struExternalBMT.bOverBatVol)
	{
		g_BatteryNotifyCode |= 1 << Notify_Bat_Over_Vol;
		printk("[BATTERY] Battery is over VOL! Notify \n");
	}
	else
	{
		//nBatVol = BAT_Get_Battery_Voltage(0);
		
	
		if ((BMT_status.bat_full)&&(BMT_status.bat_vol<4500))
		{
			if(struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_HIGH_TEMP)
			{
				g_BatteryNotifyCode |=  1 << Notify_Bat_Full_Pre_High_Temp;
			}
			//else if((struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP2) || (struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP))
//			else if((struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0) || (struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_0_12))
		//modified by PengNan for the NEW_CHARGER_STARDARD_V2_8 2015.12.3
			else if(struExternalBMT.enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0)
			{
				g_BatteryNotifyCode |=  1 << Notify_Bat_Full_Pre_Low_Temp2;
			}
			else if(!oppo_high_battery_status)
			{
				g_BatteryNotifyCode |=  1 << Notify_Bat_Full_THIRD_BATTERY;
			}
			else
			{
				if(bat_volt_check_point == 100)
					g_BatteryNotifyCode |=  1 << Notify_Bat_Full;
			}
			
			if (Enable_BATDRV_LOG == 1)
			{
				printk("[BATTERY] BATTERY_FULL,NOTIFY!\n");
			}
		}
	}

    battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] BATTERY_NOTIFY_CASE_0004_VBAT (%x)\n", g_BatteryNotifyCode);

#endif
}


static void mt_battery_notify_ICharging_check(void)
{
#if defined(BATTERY_NOTIFY_CASE_0003_ICHARGING)
    //do nothing

    //battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] BATTERY_NOTIFY_CASE_0003_ICHARGING (%x)\n", g_BatteryNotifyCode);
        
#endif
}


static void mt_battery_notify_VBatTemp_check(void)
{
	if(BATTERY_STATUS__HIGH_TEMP == struExternalBMT.enBatStatus)
	{
		if(BMT_status.charger_exist)
		{
				g_BatteryNotifyCode |= 1 << Notify_Bat_Over_Temp;
				if (Enable_BATDRV_LOG == 1)
				{
				  printk("[BATTERY] bat_temp(%d) > 55'C\n", BMT_status.temperature);
				}
		}
	}
	
	if(BATTERY_STATUS__LOW_TEMP == struExternalBMT.enBatStatus )
	{
		if(BMT_status.charger_exist)
		{
			g_BatteryNotifyCode |= 1 << Notify_Bat_Low_Temp;
			if (Enable_BATDRV_LOG == 1)
			{
			  printk("[BATTERY] bat_temp(%d) < -10'C\n", BMT_status.temperature);
			}
		}
	}

	
	if(BATTERY_STATUS__REMOVED == struExternalBMT.enBatStatus)
	{
		g_BatteryNotifyCode |= 1 << Notify_Bat_Not_Connect;
		if (Enable_BATDRV_LOG == 1)
		{
		  printk("[BATTERY] bat_temp(%d) < -40'C\n", BMT_status.temperature);	
		}
	}

    //battery_xlog_printk(BAT_LOG_FULL, "[BATTERY] BATTERY_NOTIFY_CASE_0002_VBATTEMP (%x)\n", g_BatteryNotifyCode);
        
}


static void mt_battery_notify_VCharger_check(void)
{
#if defined(BATTERY_NOTIFY_CASE_0001_VCHARGER)

    if(CHARGER_STATUS__VOL_HIGH == struExternalBMT.enChgrStatus)
	{
		g_BatteryNotifyCode |= 1 << Notify_Charger_Over_Vol;
		if (Enable_BATDRV_LOG == 1)
		{
			printk("[BATTERY] check_charger_off_vol(%d) > 5800mV\n", check_charger_off_vol);
		}
	}
			
	if(	CHARGER_STATUS__VOL_LOW == struExternalBMT.enChgrStatus)
	{
		g_BatteryNotifyCode |= 1 << Notify_Charger_Low_Vol;
		if (Enable_BATDRV_LOG == 1)
		{
			printk("[BATTERY] check_charger_off_vol(%d) < 4500mV\n", check_charger_off_vol);
		}
	}
    //if (g_BatteryNotifyCode !=0x0000)
    //battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] BATTERY_NOTIFY_CASE_0001_VCHARGER (%x)\n", g_BatteryNotifyCode);
#endif	
}


static void mt_battery_notify_UI_test(void)
{
	if(g_BN_TestMode == 0x0001)
    {
        g_BatteryNotifyCode = 0x0001;
        battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY_TestMode] BATTERY_NOTIFY_CASE_0001_VCHARGER\n");
    }
    else if(g_BN_TestMode == 0x0002)
    {
        g_BatteryNotifyCode = 0x0002;
        battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY_TestMode] BATTERY_NOTIFY_CASE_0002_VBATTEMP\n");
    }
    else if(g_BN_TestMode == 0x0003)
    {
        g_BatteryNotifyCode = 0x0004;
        battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY_TestMode] BATTERY_NOTIFY_CASE_0003_ICHARGING\n");
    }
    else if(g_BN_TestMode == 0x0004)
    {
        g_BatteryNotifyCode = 0x0008;
        battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY_TestMode] BATTERY_NOTIFY_CASE_0004_VBAT\n");
    }
    else if(g_BN_TestMode == 0x0005)
    {
        g_BatteryNotifyCode = 0x0010;
        battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY_TestMode] BATTERY_NOTIFY_CASE_0005_TOTAL_CHARGINGTIME\n");
    }
    else
    {
        battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Unknown BN_TestMode Code : %x\n", g_BN_TestMode);
    }
}


void mt_battery_notify_check(void)
{
    g_BatteryNotifyCode = 0x0000;

	if(g_BN_TestMode == 0x0000)	/* for normal case */
    {
        battery_xlog_printk(BAT_LOG_FULL, "[BATTERY] mt_battery_notify_check\n");

	    mt_battery_notify_VCharger_check();

		mt_battery_notify_VBatTemp_check();

		mt_battery_notify_ICharging_check();

		mt_battery_notify_VBat_check();

		mt_battery_notify_TotalChargingTime_check();
		
		 mt_battery_notify_flag_check();
    }	
	else  /* for UI test */
	{
		mt_battery_notify_UI_test();
	}
	
}

static void mt_battery_thermal_check(void)
{
	if( (g_battery_thermal_throttling_flag==1) || (g_battery_thermal_throttling_flag==3) )
    {
        if(battery_cmd_thermal_test_mode == 1){
            BMT_status.temperature = battery_cmd_thermal_test_mode_value;
            battery_xlog_printk(BAT_LOG_FULL, "[Battery] In thermal_test_mode , Tbat=%d\n", BMT_status.temperature);
        }
    
#if defined(MTK_JEITA_STANDARD_SUPPORT)
        //ignore default rule
#else    
		if(BMT_status.temperature >= 60)
        {
            #if defined(CONFIG_POWER_EXT)
            battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] CONFIG_POWER_EXT, no update battery update power down.\n");
            #else
            {
                if( (g_platform_boot_mode==META_BOOT) || (g_platform_boot_mode==ADVMETA_BOOT) || (g_platform_boot_mode==ATE_FACTORY_BOOT) )
                {
                    battery_xlog_printk(BAT_LOG_FULL, "[BATTERY] boot mode = %d, bypass temperature check\n", g_platform_boot_mode);
                }
                else
                {
                    struct battery_data *bat_data = &battery_main;
                    struct power_supply *bat_psy = &bat_data->psy;

                    battery_xlog_printk(BAT_LOG_CRTI, "[Battery] Tbat(%d)>=60, system need power down.\n", BMT_status.temperature);

                    //bat_data->BAT_CAPACITY = 0;

                    power_supply_changed(bat_psy); 

                    if( BMT_status.charger_exist == KAL_TRUE )
                    {
                        // can not power down due to charger exist, so need reset system
                        battery_charging_control(CHARGING_CMD_SET_PLATFORM_RESET,NULL);
                    }
                    //avoid SW no feedback
                    battery_charging_control(CHARGING_CMD_SET_POWER_OFF,NULL);
                    //mt_power_off();
                }
            }
            #endif
        }
#endif
        
    }

}

void mt_battery_update_status(void)
{
#if defined(CONFIG_POWER_EXT)
    battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] CONFIG_POWER_EXT, no update Android.\n");
#else
	{
		//wireless_update(&wireless_main);
		battery_update(&battery_main);	
		ac_update(&ac_main);
		usb_update(&usb_main);					
	}

#endif	
}

CHARGER_TYPE mt_charger_type_detection(void)
{
    CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;

    mutex_lock(&charger_type_mutex);
    
#if defined(MTK_WIRELESS_CHARGER_SUPPORT)
    battery_charging_control(CHARGING_CMD_GET_CHARGER_TYPE,&CHR_Type_num);
    BMT_status.charger_type = CHR_Type_num;	
#else
    if(BMT_status.charger_type == CHARGER_UNKNOWN)
    {
        battery_charging_control(CHARGING_CMD_GET_CHARGER_TYPE,&CHR_Type_num);
        BMT_status.charger_type = CHR_Type_num;	
		usb_main.type_identify = CHR_Type_num;
	}
#endif		
    mutex_unlock(&charger_type_mutex);
    return BMT_status.charger_type;
}

///////////////////////////////////////////////////////////////////////////////////////////
//// PMIC AUXADC Related APIs
///////////////////////////////////////////////////////////////////////////////////////////
#define AUXADC_BATTERY_VOLTAGE_CHANNEL  0x10
#define AUXADC_REF_CURRENT_CHANNEL     	0x11
#define AUXADC_CHARGER_VOLTAGE_CHANNEL  0x17
#define AUXADC_TEMPERATURE_CHANNEL     	0x13

//#define AUXADC_BATTERY_VOLTAGE_CHANNEL  0
//#define AUXADC_REF_CURRENT_CHANNEL     	1
//#define AUXADC_CHARGER_VOLTAGE_CHANNEL  2
//#define AUXADC_TEMPERATURE_CHANNEL     	3

#define VOLTAGE_FULL_RANGE 	1200
#define ADC_PRECISE 		1024 // 10 bits


static DEFINE_MUTEX(pmic_adc_mutex);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
extern int IMM_IsAdcInitReady(void);

int get_bat_sense_volt(int times)
{
    return PMIC_IMM_GetOneChannelValue(AUXADC_REF_CURRENT_CHANNEL,times,1);
}


int get_charger_volt(int times)
{
    int charger_vol = PMIC_IMM_GetOneChannelValue(AUXADC_CHARGER_VOLTAGE_CHANNEL,times,1);
	charger_vol = (((R_CHARGER_1+R_CHARGER_2)*100*charger_vol)/R_CHARGER_2)/100;
	return charger_vol;
}

int g_Get_I_Charging(void)
{

	int ICharging=0;	

	ICharging = opchg_get_prop_current_now();



	if (Enable_BATDRV_LOG == 1) {
		printk("g_Get_I_Charging :%d",ICharging);
		}
	
	return ICharging;
}

//PengNan@Drv.CHG remove  selfadpat  2015/06/15
void pchr_turn_on_charging_bq24196_without_selfadapt (void)
{
	TBatStatus enBatStatus = struExternalBMT.enBatStatus;
	UINT32 nFullVol = 0;
	if(bq27541_di->alow_reading == false) {
		printk("mcu alow_reading false, pchr_turn_on_charging_bq24196 return\r\n");
		return;
	}
	if ( BMT_status.bat_charging_state == CHR_ERROR ) 
	{
		if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY:bq24196] Charger Error, turn OFF charging !\r\n");
		}
		pchr_turn_off_charging_bq24196();
	}
	//else if( (get_boot_mode()==META_BOOT) || (get_boot_mode()==ADVMETA_BOOT) ||(get_boot_mode()==FACTORY_BOOT) )
	else if( (get_boot_mode()==META_BOOT) || (get_boot_mode()==ADVMETA_BOOT))  //modify for 12009 ftm battery
	{
		if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY:bq24196] In meta,ftm mode or advanced meta mode, disable charging.\r\n");
		}
		pchr_turn_off_charging_bq24196();
	}
	else if(at_test_chg_on==0) 
	{
	    if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY] at_test_chg_on==0, disable charging.\r\n");
		}
		pchr_turn_off_charging_bq24196();
	}
	#ifdef CALL_MODE_CHARGE_PAUSE
	else if(call_pause_mode!=0) 
	{
	   
    	if (Enable_BATDRV_LOG == 1) {
    		printk("[BATTERY] in call_pause_mode, pause charging.\r\n");
    	}
    	pchr_turn_off_charging_bq24196();
	}
	#endif
	else
	{
	   	bq24196_hardware_init();
		bq24196_charging_current_set();
//		bq24196_input_current_limit_set();	//PengNan@Drv.CHG remove for selfadpat time 2015/06/15
		//bq24196_kick_wdt();
		bq24196_enable_charging();
    	
		if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY:bq24196] charger enable !\r\n");
					
	    }
    }
}

void pchr_turn_on_charging_bq24196 (void)
{

    TBatStatus enBatStatus = struExternalBMT.enBatStatus;
	UINT32 nFullVol = 0;

	if(bq27541_di->alow_reading == false) {
		printk("mcu alow_reading false, pchr_turn_on_charging_bq24196 return\r\n");
		return;
	}
	if ( BMT_status.bat_charging_state == CHR_ERROR ) 
	{
		if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY:bq24196] Charger Error, turn OFF charging !\r\n");
		}
		pchr_turn_off_charging_bq24196();
	}
	//else if( (get_boot_mode()==META_BOOT) || (get_boot_mode()==ADVMETA_BOOT) ||(get_boot_mode()==FACTORY_BOOT) )
	else if( (get_boot_mode()==META_BOOT) || (get_boot_mode()==ADVMETA_BOOT))  //modify for 12009 ftm battery
	{
		if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY:bq24196] In meta,ftm mode or advanced meta mode, disable charging.\r\n");
		}
		pchr_turn_off_charging_bq24196();
	}
	else if(at_test_chg_on==0) 
	{
	    if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY] at_test_chg_on==0, disable charging.\r\n");
		}
		pchr_turn_off_charging_bq24196();
	}
	#ifdef CALL_MODE_CHARGE_PAUSE
	else if(call_pause_mode!=0) 
	{
	   
    	if (Enable_BATDRV_LOG == 1) {
    		printk("[BATTERY] in call_pause_mode, pause charging.\r\n");
    	}
    	pchr_turn_off_charging_bq24196();
	}
	#endif
	else
	{
	   	bq24196_hardware_init();
		bq24196_charging_current_set();
		bq24196_input_current_limit_set();
		//bq24196_kick_wdt();
		if((struExternalBMT.nChgStopVoterMask & CHG_STOP_VOTER__BATTTEMP_ABNORMAL) != CHG_STOP_VOTER__BATTTEMP_ABNORMAL){
			bq24196_enable_charging();
		}
		if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY:bq24196] charger enable !\r\n");
					
	    }
    }
}

void pchr_turn_off_charging_bq24196(void)
{
	if (Enable_BATDRV_LOG == 1) {
		printk("[BATTERY] pchr_turn_off_charging_bq24196 !\r\n");
	}
	if(bq27541_di->alow_reading == false) {
		printk("mcu alow_reading false, pchr_turn_on_charging_bq24196 return\r\n");
		return;
	}
	//bq24196_kick_wdt();
	bq24196_disable_charging();

}

TBatStatus get_battery_temp_status(void)
{
	//printk("get_battery_temp_status---------enBatStatus = %d\r\n", struExternalBMT.enBatStatus);
	return struExternalBMT.enBatStatus;
}
CHARGER_TYPE get_battery_charger_type(void)
{
	//printk("get_battery_charger_type---------type = %d\r\n", BMT_status.charger_type);
	return BMT_status.charger_type;
}


static UINT32 KernelVendorGetReChgingVol(void)
{
	TBatStatus enBatStatus = struExternalBMT.enBatStatus;
	UINT32 nReChgingVol = 0;

    if(BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0  == enBatStatus)
    {
        nReChgingVol = D_PRE_COLD_TEMP_RECHGING_VOL_NEG3_0;
    } 
//    else if(BATTERY_STATUS__PRE_LOW_TEMP_0_12 == enBatStatus)
	else if(BATTERY_STATUS__PRE_LOW_TEMP_5_12 == enBatStatus)
	{
		nReChgingVol = D_PRE_COLD_TEMP_RECHGING_VOL_5_12;
	}
	else if(BATTERY_STATUS__PRE_LOW_TEMP_0_5 == enBatStatus)
	{
		nReChgingVol = D_PRE_COLD_TEMP_RECHGING_VOL_0_5;
	}
	else if(BATTERY_STATUS__PRE_LOW_TEMP_12_16 == enBatStatus)
	{
		nReChgingVol = D_PRE_COLD_TEMP_RECHGING_VOL_12_16;
	}
	else if(BATTERY_STATUS__PRE_HIGH_TEMP == enBatStatus)
	{
		nReChgingVol = D_PRE_WARM_TEMP_RECHGING_VOL;
	}
	else
	{
		nReChgingVol = D_NORMAL_TEMP_RECHGING_VOL;
	}
	#ifdef OPPO_BATTERY_ENCRPTION
	if((!oppo_high_battery_status) && (nReChgingVol > D_THIRD_BAT_RECHGING_VOL))
	{
		nReChgingVol = D_THIRD_BAT_RECHGING_VOL;
	}		
	#endif
	
	return nReChgingVol;
}



PMU_STATUS BAT_BatteryStatusFailAction(void)
{
    if (Enable_BATDRV_LOG == 1) {
    	xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[BATTERY] BAD Battery status... Charging Stop !!\n\r");            
    }

    BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.POSTFULL_charging_time = 0;
	BMT_status.bat_full = KAL_FALSE;

    /*  Disable charger */

    /*  Disable charger */
   // pchr_turn_off_charging();

	#ifdef NEW_CHARGE_FULL_STOP
    bq24156a_post_vol_count=0;
    bq24156a_post_status=0;
    bq24156a_post_vol_time=0;
    bq24156a_post_current_count=0;
    #endif
	gFG_can_reset_flag = 0;
	
	fastchg_present_flag = 0;
	fastchg_present_wait_count = 0;
    return PMU_STATUS_OK;
}
PMU_STATUS BAT_BatteryFullAction(void) //not modify for rechging ,dengzy
{
	if (Enable_BATDRV_LOG == 1) {    
    	printk(  "[BATTERY] Battery full !!\n\r");            
	}  
    BMT_status.bat_full = KAL_TRUE;
    BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.CC_charging_time = 0;
	BMT_status.TOPOFF_charging_time = 0;
	BMT_status.POSTFULL_charging_time = 0;
	
	g_HW_Charging_Done = 1;
	g_Calibration_FG = 1;
	if( gForceADCsolution == 0 )
	{
		if(gFG_can_reset_flag == 1)
		{
			if((BMT_status.bat_vol > 4200) && (bat_volt_check_point == 100))
			{   
				gFG_can_reset_flag = 0;

				if (Enable_BATDRV_LOG >= 1) {
					xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[BATTERY] Battery real full. Call battery_meter_reset.\n");
				}
				battery_meter_reset();
			}
		}
		//fg_qmax_update_for_aging();
	}
	//g_Calibration_FG = 0;
	//The battery has been fully charged, so check for re-charging
	UINT32 nReChgingVol = KernelVendorGetReChgingVol();
	UINT32 nBatVol = BMT_status.bat_vol;

	static UINT32 rechging_cnt_vol=0, rechging_cnt_soc=0;
	
	if(nBatVol <= nReChgingVol)
	{
		rechging_cnt_vol++;
	}
	else
	{
		rechging_cnt_vol=0;
	}
	

	//if((rechging_cnt_soc>D_RECHGING_CNT) || (rechging_cnt_vol>D_RECHGING_CNT))
	if(rechging_cnt_vol>D_RECHGING_CNT)
	{

		b_notify_first=TRUE;
		xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] Battery rechg begin! bat_volt_check_point=%d, BMT_status.SOC=%ld, nBatVol = %d\r\n", bat_volt_check_point, BMT_status.SOC, nBatVol);
		if(bq27541_di->alow_reading == true)
		{
			bq24196_unsuspend_charger();
		}
		KernelVendorVoteToStopCharging(CHG_CMD_DISABLE, CHG_STOP_VOTER__FULL);//now rechging!
		BMT_status.bat_in_recharging_state = KAL_TRUE;
		#ifdef NEW_CHARGE_FULL_STOP
        bq24156a_post_vol_count=0;
        bq24156a_post_status=0;
        bq24156a_post_vol_time=0;
        bq24156a_post_current_count=0;
        #endif
	}


    
    return PMU_STATUS_OK;
}



static void KernelVendorVoteToStopCharging(TChgCmd cmd, TChgStopVoter voter)
{
	UINT32 nBatChgingSta = 0;

	
	switch(cmd)
	{
		case CHG_CMD_ENABLE:
			struExternalBMT.nChgStopVoterMask |= (UINT32)voter;
			struExternalBMT.bChgingOn = KAL_FALSE;
			if(bq27541_di->alow_reading == true)
			{
				bq24196_suspend_charger();
			}
			if(CHG_STOP_VOTER__FULL == voter){
				printk("KernelVendorVoteToStopCharging,CHG_CMD_ENABLE,not disable the bq24196\r\n");
			}
			else{
				pchr_turn_off_charging_bq24196();
			}
		
			break;

		case CHG_CMD_DISABLE:
			struExternalBMT.nChgStopVoterMask &= ~((UINT32)voter);

//			if(((UINT32)0 == struExternalBMT.nChgStopVoterMask) && (KAL_FALSE == struExternalBMT.bChgingOn))
			if(KAL_FALSE == struExternalBMT.bChgingOn)
			{
				struExternalBMT.bChgingOn = KAL_TRUE;
				pchr_turn_on_charging_bq24196();
				bq24196_unsuspend_charger();
			}
			break;

		default:
			
			break;
	}

	switch(voter)
	{
		case CHG_STOP_VOTER__FULL:
		//case CHG_STOP_VOTER_MAX_TOPOFF_TIME:
			if(CHG_CMD_ENABLE == cmd)
			{

				nBatChgingSta = BQ24196_CHR_FULL;
				BAT_BatteryFullAction();
			}
			else
			{
				nBatChgingSta = BQ24196_CHR_CCCV;
				pchr_turn_on_charging_bq24196();	
				g_bat_full_user_view=KAL_TRUE;
				g_HW_Charging_Done = 0;
			}
			break;
		
		case CHG_STOP_VOTER__VCHG_ABNORMAL:
		case CHG_STOP_VOTER__BATTTEMP_ABNORMAL:
			if(CHG_CMD_ENABLE == cmd)
			{

				nBatChgingSta = BQ24196_CHR_FAIL;
			}
			else
			{
				nBatChgingSta = BQ24196_CHR_CCCV;
				pchr_turn_on_charging_bq24196();	
				g_bat_full_user_view=KAL_FALSE;
				g_HW_Charging_Done = 0;
			}
			break;
		case CHG_STOP_VOTER__VBAT_TOO_HIGH:
		case CHG_STOP_VOTER__MAX_CHGING_TIME:

		case CHG_STOP_VOTER__MAX_CC3_CHGING_TIME:
		
			if(CHG_CMD_ENABLE == cmd)
			{

				nBatChgingSta = BQ24196_CHR_FAIL;
				BAT_BatteryStatusFailAction();

				if(CHG_STOP_VOTER__MAX_CHGING_TIME == voter)
				{
					g_Charging_Over_Time = 1;
				}

				if(CHG_STOP_VOTER__VBAT_TOO_HIGH == voter)
				{
					struExternalBMT.bOverBatVol = KAL_TRUE;
				}
			}
			break;

		default:
			break;
	}

	if(0 != nBatChgingSta)
	{
		BMT_status.bat_charging_state = nBatChgingSta;
	}
}

CHARGER_TYPE mt_get_charger_type(void)
{
	return BMT_status.charger_type;	
}
void do_chrdet_MMI_resume_charger(struct battery_data *bat_data)
{
	bat_data->BAT_MMI_CHG = 1;
	at_test_chg_on = 1;
	printk("[%s]\n",__func__);	
}

void do_chrdet_Temp_abnormal(void)
{
	
	if( BMT_status.charger_exist == KAL_TRUE )
    {
		#ifdef FEATURE_BAT_TEMP_PROTECT
		if(KAL_FALSE == KernelVendorBatTempIsGood())
		{
			printk("KernelVendorBatTempIsGood func ,false!");
			KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__BATTTEMP_ABNORMAL);
		}
		else
		{
	//		if(struExternalBMT.nChgStopVoterMask == CHG_STOP_VOTER__BATTTEMP_ABNORMAL)
			if((struExternalBMT.nChgStopVoterMask & CHG_STOP_VOTER__BATTTEMP_ABNORMAL) == CHG_STOP_VOTER__BATTTEMP_ABNORMAL)			
			{
				printk("KernelVendorBatTempIsGood func ,true! To Normal\n");
	  			KernelVendorVoteToStopCharging(CHG_CMD_DISABLE, CHG_STOP_VOTER__BATTTEMP_ABNORMAL);
			}
		}
		#endif
	}
//	mt_battery_notify_check();
	
	printk("%s------\n",__func__);
}

static void mt_battery_charger_detect_check(void)
{
    int i;
	static int OPPO_LED_PRE = 0;
	static int pluge_in_open_tp = 0;
	static int plug_out_fastcharg_in_count = 0;
	if( pmic_chrdet_status() == KAL_TRUE )
    {
        wake_lock(&battery_suspend_lock);
		
        BMT_status.charger_exist = KAL_TRUE;		
		if(BMT_status.charger_type == CHARGER_UNKNOWN)
		{
			KernelVendorChgrInInitVariables();
			mt_charger_type_detection();
			
			ac_update(&ac_main);
			usb_update(&usb_main);
			
			if((BMT_status.charger_type==STANDARD_HOST) || (BMT_status.charger_type==CHARGING_HOST) )
	        {
	           	mt_usb_connect();
			}
			
			BMT_status.charger_vol = battery_meter_get_charger_voltage();
			check_charger_off_vol = BMT_status.charger_vol;
			#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
			//pengnan  2015/5/1 add for tp avoid charge disturb 			
			is_oppo_fast_charger = 0;
			charge_plug_tp_avoid_distrub(1,is_oppo_fast_charger);
			#endif 
			pluge_in_open_tp = 1;
			OPPO_LED_PRE = OPPO_LED_ON;
			plug_out_fastcharg_in_count = 0;
			plug_in_flag_set_charging_current = 1;
			plug_in_Temp_abnomal = 1;
			do_chrdet_MMI_resume_charger(&battery_main);
			KernelVendorBatTempIsGood();
			do_chrdet_Temp_abnormal();
			pchr_turn_on_charging_bq24196();
			plug_in_flag_selfadapt = 0;
			
			//KernelVendorGetChgingOnVol();	
			battery_xlog_printk(BAT_LOG_CRTI, "[BAT_thread]Cable in, CHR_Type_num=%d\r\n", BMT_status.charger_type);	
		}
		else
		{
			if(struExternalBMT.bChgingOn == KAL_TRUE)
			{
				//KernelVendorGetChgingOnVol();
				charge_on_cnt++;
				if(charge_on_cnt>=4)  //5s*4
				{
				    charge_on_cnt=0;
					OPPO_LED_PRE = OPPO_LED_ON;

					plug_in_flag_set_charging_current = 0;
					pchr_turn_on_charging_bq24196_without_selfadapt();
    				//KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__INTERMITTENT_CHGING);			
    				//msleep(500);  //charging 20s, stop 0.5s
    				//KernelVendorGetChgingOffVol();
    				//check_charger_off_vol = BMT_status.charger_vol;   				
    				//KernelVendorVoteToStopCharging(CHG_CMD_DISABLE, CHG_STOP_VOTER__INTERMITTENT_CHGING);
    			}
    			else
    			{
    				if(bq27541_di->alow_reading == true)
					{
						if(plug_in_flag_selfadapt == 1)
						{
							pchr_turn_on_charging_bq24196();
							plug_in_flag_selfadapt = 0;	
							printk("plug_in but charger_type is not unknown,pchr_turn_on_charging manually!!\n");
						}
						if(TempIsChanged == KAL_FALSE && at_test_chg_on  == 1){
							bq24196_float_voltage_set();  //add by PengNan for protect battery at CV 2015/04/07
							bq24196_charging_current_set();//modified by PengNan for the new charger standard V2.8 2015.12.3
						}
					}
					if(TempIsChanged == KAL_TRUE)
					{
						TempIsChanged = KAL_FALSE;	
						pchr_turn_on_charging_bq24196();
						printk("Temp is changed!! pchr_turn_on_charging!!\n");
					}
    			    //KernelVendorGetChgingOffVol();    			    
    			}
			}
			else
			{
				#ifdef OPPO_CHARGER_RESUME
				BMT_status.charger_vol = battery_meter_get_charger_voltage();
				check_charger_off_vol = BMT_status.charger_vol;
				#endif
						
				//KernelVendorGetChgingOffVol();
				//KernelVendorGetChgingOnVol();
			}
			
		}
    	
    }
    else 
    {
		wake_unlock(&battery_suspend_lock);
		
		 mt_usb_disconnect(); 
		KernelVendorChgrOutResetVariables();
		plug_in_flag_selfadapt = 0;//PengNan@Drv.CHG add for insure selfadpat when plug in 2015/06/17
		if(opchg_get_prop_fast_switch_to_normal() == true || 
				opchg_get_fast_normal_to_warm() == true)
		{
			plug_out_fastcharg_in_count++;
			if(plug_out_fastcharg_in_count > 1){
				plug_out_fastcharg_in_count = 0;
				printk("fastcharging status is still true when plug_out!!\n");
				do_chrdet_int_task();
			}
		}
		if( gForceADCsolution == 1 )
		{
			g_bat_full_user_view = KAL_FALSE;
		}
		/*Use gas gauge*/
		else
		{
			if(bat_volt_check_point != 100) {
	        	g_bat_full_user_view = KAL_FALSE;
				if (Enable_BATDRV_LOG == 1) {
					xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery_Only] Set g_bat_full_user_view=KAL_FALSE\r\n");
				}
			}
		}
		for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
		   batteryCurrentBuffer[i] = 0;
	   	}
	   	batteryCurrentSum = 0;
		plug_in_flag_set_charging_current = 0;
		if(pchr_turn_off_plug_out == 1){
			pchr_turn_off_charging_bq24196();
			pchr_turn_off_plug_out = 0;
		}
		plug_in_Temp_abnomal = 0;
		if(pluge_in_open_tp ==1){
			#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
			//pengnan  2015/5/1 add for tp avoid charge disturb 
			charge_plug_tp_avoid_distrub(0,is_oppo_fast_charger);
			#endif 
			pluge_in_open_tp = 0;
		}		
		battery_xlog_printk(BAT_LOG_CRTI, "[BAT_thread]Cable out ,pluge_in_open_tp= %d\r\n", pluge_in_open_tp);
				
                
    }
}

#define SELFADAPT_DELAY_15MIN			180
//#define PLUG_IN_SET_CURRENT_20S			4
void opchg_selfadapt_repeatedly()
{
	
	if(BMT_status.charger_exist == KAL_FALSE || BMT_status.bat_full == KAL_TRUE || struExternalBMT.enBatStatus != BATTERY_STATUS__GOOD
		|| bat_volt_check_point > 85 || opchg_get_prop_fast_chg_started() == true)
	{
		selfadapt_delay_count = 0;
		return 0;
	}
	
	if(selfadapt_delay_count > SELFADAPT_DELAY_15MIN){
		selfadapt_delay_count = 0;
		if(bq27541_di->alow_reading == true){
			bq24196_input_current_limit_set();
		}
	} else {
		selfadapt_delay_count++;
	}
}
void opchg_selfadapt_work()
{	
	opchg_selfadapt_repeatedly();
	if(BMT_status.charger_exist == KAL_TRUE)
	{
		if(struExternalBMT.bChgingOn == KAL_TRUE)
		{
			if(bq27541_di->alow_reading == true){
				bq24196_input_current_limit_set_LED();	//modified by PengNan for OPPO_LED_Current 2015/03/28
			}
		}
	}
}

static void mt_kpoc_power_off_check(void)
{
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT)
	{
		if( (pmic_chrdet_status() == KAL_FALSE)  && (BMT_status.charger_vol < 2500))	//vbus < 2.5V
		{
			if((opchg_get_prop_fast_switch_to_normal() == KAL_FALSE) && (opchg_get_fast_normal_to_warm() == false))
			{
				battery_xlog_printk(BAT_LOG_CRTI, "[pmic_thread_kthread] Unplug Charger/USB In Kernel Power Off Charging Mode!  Shutdown OS!\r\n");
				primary_display_suspend();
				battery_charging_control(CHARGING_CMD_SET_POWER_OFF,NULL);
			}		
		}
	}
#endif	
}

void update_battery_2nd_info(int status_2nd, int capacity_2nd, int present_2nd)
{
    #if defined(CONFIG_POWER_VERIFY)
    battery_xlog_printk(BAT_LOG_CRTI, "Power/Battery", "[update_battery_2nd_info] no support\n");
    #else
    g_status_2nd = status_2nd;
    g_capacity_2nd = capacity_2nd;
    g_present_2nd = present_2nd;
    battery_xlog_printk(BAT_LOG_CRTI, "[update_battery_2nd_info] get status_2nd=%d,capacity_2nd=%d,present_2nd=%d\n",
        status_2nd, capacity_2nd, present_2nd);

    wake_up_bat();
    g_smartbook_update = 1;
    #endif
}

void do_chrdet_int_task(void)
{
    if(opchg_get_prop_fast_chg_started() == true){
		battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] opchg_get_prop_fast_chg_started = true!\n");
		return;
	}
	if(g_bat_init_flag == KAL_TRUE)
    {
        if( pmic_chrdet_status() == KAL_TRUE )
        {
            battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] charger exist!\n");
            BMT_status.charger_exist = KAL_TRUE;
			pchr_turn_off_plug_out = 1;
			plug_in_flag_selfadapt = 1;
			if(bq27541_di->alow_reading == true)
			{
				//bq24196_reset_charger();
				bq24196_unsuspend_charger();
			}
            wake_lock(&battery_suspend_lock);

            #if defined(CONFIG_POWER_EXT)
            mt_usb_connect();
     	    battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] call mt_usb_connect() in EVB\n");
#elif defined(MTK_POWER_EXT_DETECT)
				if(KAL_TRUE == bat_is_ext_power())
				{
					mt_usb_connect();
					battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] call mt_usb_connect() in EVB\n");
					return;
				}
#endif
			if(battery_suspended == KAL_TRUE)
				msleep(700);
        }
        else
        {
       	    battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] charger NOT exist!\n");
            BMT_status.charger_exist = KAL_FALSE;
			usb_main.type_identify = 0;
			#ifdef OPPO_USE_FAST_CHARGER
			reset_fastchg_after_usbout();
			#endif


#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
            if(g_platform_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_platform_boot_mode == LOW_POWER_OFF_CHARGING_BOOT)
            {
                battery_xlog_printk(BAT_LOG_CRTI, "[pmic_thread_kthread] Unplug Charger/USB In Kernel Power Off Charging Mode!  Shutdown OS!\r\n");
				primary_display_suspend();
    			battery_charging_control(CHARGING_CMD_SET_POWER_OFF,NULL);
    			//mt_power_off();
            }
#endif

            wake_unlock(&battery_suspend_lock);
			if(bq27541_di->alow_reading == true)
			{
				//bq24196_reset_charger();
				bq24196_unsuspend_charger();
				bq24196_input_current_limit_set();
				bq24196_float_voltage_write(4350);
			}
            #if defined(CONFIG_POWER_EXT)
            mt_usb_disconnect();
            battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] call mt_usb_disconnect() in EVB\n");
			#elif defined(MTK_POWER_EXT_DETECT)
				if(KAL_TRUE == bat_is_ext_power())
				{
					mt_usb_disconnect();
					battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] call mt_usb_disconnect() in EVB\n");
					return;
				}
    		#endif
			#if defined(MTK_PUMP_EXPRESS_SUPPORT) || defined(MTK_PUMP_EXPRESS_PLUS_SUPPORT)
				 is_ta_connect = KAL_FALSE;    
				 ta_check_chr_type = KAL_TRUE;
				 ta_cable_out_occur = KAL_TRUE;
			#endif

        }
         
        wake_up_bat();
    }    
    else
   	{
        battery_xlog_printk(BAT_LOG_CRTI, "[do_chrdet_int_task] battery thread not ready, will do after bettery init.\n");    
   	}

}
int g_temp_CC_value = 0;
 kal_uint32 g_bcct_flag=0;
 kal_uint32 g_usb_state = USB_UNCONFIGURED;
void BATTERY_SetUSBState(int usb_state_value)
{
#if defined(CONFIG_POWER_EXT)
	battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY_SetUSBState] in FPGA/EVB, no service\r\n");
#else
    if ( (usb_state_value < USB_SUSPEND) || ((usb_state_value > USB_CONFIGURED))){
        battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] BAT_SetUSBState Fail! Restore to default value\r\n");    
        usb_state_value = USB_UNCONFIGURED;
    } else {
        battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] BAT_SetUSBState Success! Set %d\r\n", usb_state_value);    
        g_usb_state = usb_state_value;    
    }
#endif	
}

kal_uint32 set_bat_charging_current_limit(int current_limit)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] set_bat_charging_current_limit (%d)\r\n", current_limit);

    if(current_limit != -1)
    {
        g_bcct_flag=1;
        
        if(current_limit < 70)         g_temp_CC_value=CHARGE_CURRENT_0_00_MA;
        else if(current_limit < 200)   g_temp_CC_value=CHARGE_CURRENT_70_00_MA;
        else if(current_limit < 300)   g_temp_CC_value=CHARGE_CURRENT_200_00_MA;
        else if(current_limit < 400)   g_temp_CC_value=CHARGE_CURRENT_300_00_MA;
        else if(current_limit < 450)   g_temp_CC_value=CHARGE_CURRENT_400_00_MA;
        else if(current_limit < 550)   g_temp_CC_value=CHARGE_CURRENT_450_00_MA;
        else if(current_limit < 650)   g_temp_CC_value=CHARGE_CURRENT_550_00_MA;
        else if(current_limit < 700)   g_temp_CC_value=CHARGE_CURRENT_650_00_MA;
        else if(current_limit < 800)   g_temp_CC_value=CHARGE_CURRENT_700_00_MA;
        else if(current_limit < 900)   g_temp_CC_value=CHARGE_CURRENT_800_00_MA;
        else if(current_limit < 1000)  g_temp_CC_value=CHARGE_CURRENT_900_00_MA;
        else if(current_limit < 1100)  g_temp_CC_value=CHARGE_CURRENT_1000_00_MA;
        else if(current_limit < 1200)  g_temp_CC_value=CHARGE_CURRENT_1100_00_MA;
        else if(current_limit < 1300)  g_temp_CC_value=CHARGE_CURRENT_1200_00_MA;
        else if(current_limit < 1400)  g_temp_CC_value=CHARGE_CURRENT_1300_00_MA;
        else if(current_limit < 1500)  g_temp_CC_value=CHARGE_CURRENT_1400_00_MA;
        else if(current_limit < 1600)  g_temp_CC_value=CHARGE_CURRENT_1500_00_MA;
        else if(current_limit == 1600) g_temp_CC_value=CHARGE_CURRENT_1600_00_MA;
        else                           g_temp_CC_value=CHARGE_CURRENT_450_00_MA;
    }
    else
    {
        //change to default current setting
        g_bcct_flag=0;
    }
    
    wake_up_bat();

    return g_bcct_flag;
} 

static void KernelVendorChgrInInitVariables(void)
{
	BMT_status.bat_exist = KAL_TRUE;
	BMT_status.bat_full = KAL_FALSE;
	BMT_status.bat_charging_state=BQ24196_CHR_CCCV;
//	if(at_test_chg_on == 0)
//		BMT_status.bat_charging_state=BQ24196_CHR_FAIL;
	
	BMT_status.charger_exist = KAL_TRUE;
	BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.charger_type = CHARGER_UNKNOWN;
	BMT_status.bat_in_recharging_state = KAL_FALSE;
    check_charger_off_vol = 5000;
	TempIsChanged = KAL_FALSE;
	
	struExternalBMT.bChgingOn = KAL_TRUE;
	struExternalBMT.nIntermittentOnTime = 0;
	struExternalBMT.nIntermittentOffTime = 0;
	struExternalBMT.nChgStopVoterMask = 0x00;
	struExternalBMT.nCC1ModeChgingTime = 0;
	struExternalBMT.nCC2ModeChgingTime = 0;
	struExternalBMT.nCC3ModeChgingTime = 0;
	
	struExternalBMT.topoff_mode_chging_time=0;  //added
	
	struExternalBMT.enChgingCurSet = Cust_CC_200MA;
	struExternalBMT.enBatStatus = BATTERY_STATUS__GOOD;
	struExternalBMT.enChgrStatus = CHARGER_STATUS__GOOD;
	struExternalBMT.bOverBatVol = KAL_FALSE;

	struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
	struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
	struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
	struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
	struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
	struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
	struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;

	g_HW_Charging_Done = 0;
	g_Charging_Over_Time = 0;
	g_soc_sync_time=0;
	charge_on_cnt=0;
	get_current_offset_flag=1;

	#ifdef CALL_MODE_CHARGE_PAUSE
	call_pause_mode=0;
	#endif

	#ifdef NEW_CHARGE_FULL_STOP
    bq24156a_post_vol_count=0;
    bq24156a_post_status=0;
    bq24156a_post_vol_time=0;
    bq24156a_post_current_count=0;
    #endif
	#ifdef OPPO_CHARGER_RESUME
    over_charger_error_count = 0;
	#endif
	#ifdef OPPO_USE_FAST_CHARGER_RESET_MCU
	fast_charger_reset_sign = KAL_FALSE;
	fast_charger_reset_count = 0;
	#endif
	gFG_can_reset_flag = 1;
	
	fastchg_present_flag = 0;
	fastchg_present_wait_count = 0;
	vol_count_temp5_12 = 0;
	vol_count_flag = 0;

}

static void KernelVendorChgrOutResetVariables(void)
{
	BMT_status.bat_full = KAL_FALSE;
	//BMT_status.bat_charging_state = CHR_ERROR;
	
	BMT_status.bat_charging_state=BQ24196_CHR_FAIL;	

	BMT_status.charger_exist = KAL_FALSE;
	BMT_status.charger_type = CHARGER_UNKNOWN;
	BMT_status.total_charging_time = 0;
	BMT_status.PRE_charging_time = 0;
	BMT_status.bat_in_recharging_state = KAL_FALSE;
	TempIsChanged = KAL_FALSE;
	
	struExternalBMT.bChgingOn = KAL_FALSE;
	struExternalBMT.nIntermittentOnTime = 0;
	struExternalBMT.nIntermittentOffTime = 0;
	struExternalBMT.nChgStopVoterMask = 0x00;
	struExternalBMT.nCC1ModeChgingTime = 0;
	struExternalBMT.nCC2ModeChgingTime = 0;
	struExternalBMT.nCC3ModeChgingTime = 0;
	
	struExternalBMT.topoff_mode_chging_time=0;//  2012.1.3 added
	
	struExternalBMT.enChgingCurSet = Cust_CC_0MA;
	struExternalBMT.enChgrStatus = CHARGER_STATUS__INVALID;
	struExternalBMT.bOverBatVol = KAL_FALSE;

	struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
	struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
	struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
	struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
	struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
	struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
	struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	g_usb_state = USB_UNCONFIGURED;
	g_HW_Charging_Done = 0;
	g_Charging_Over_Time = 0;
	g_Calibration_FG = 0;
	
	//add by dengzy
	g_Battery_Fail = KAL_FALSE;


	g_bat_full_user_view=FALSE;
	g_NotifyFlag=0;
	//soc_sync_time=0;
	charge_on_cnt=0;
	get_current_offset_flag=1;

	#ifdef CALL_MODE_CHARGE_PAUSE
	call_pause_mode=0;
	#endif

	#ifdef NEW_CHARGE_FULL_STOP
    bq24156a_post_vol_count=0;
    bq24156a_post_status=0;
    bq24156a_post_vol_time=0;
    bq24156a_post_current_count=0;
    #endif
	#ifdef OPPO_CHARGER_RESUME
    over_charger_error_count = 0;
	#endif
	#ifdef OPPO_USE_FAST_CHARGER_RESET_MCU
	fast_charger_reset_sign = KAL_FALSE;
	fast_charger_reset_count = 0;
	#endif
	#ifdef NEW_CHGING_TIME
	max_charging_time_kernel = 10*60*60;//s
	#endif
	#ifdef OPPO_BATTERY_ENCRPTION
	oppo_high_battery_check_counts = 0;
	#endif
	gFG_can_reset_flag = 1;
	
	fastchg_present_flag = 0;
	fastchg_present_wait_count = 0;

	vol_count_temp5_12 = 0;
	vol_count_flag = 0;
}

#ifdef NEW_CHARGE_FULL_STOP
#define ABNORMAL_TEMP_LOW_POST_VOL_NEG3_0 							3950
#define ABNORMAL_TEMP_PRE_LOW_POST_VOL__0_12 						4150
#define ABNORMAL_TEMP_PRE_LOW_POST_VOL__12_16  						4300
#define ABNORMAL_TEMP_HIGH_POST_VOL  								4050
#define NORMAL_TEMP_POST_VOL    									4300

#define POST_VOL_COUNTS 20
#define POST_VOL_TIMES 180
#define POST_CURRENT 120
#define POST_CURRENT_COUNTS 20
static kal_uint32 kernel_vendor_get_post_vol(void)
{
	TBatStatus enBatStatus = struExternalBMT.enBatStatus;
	UINT32 post_vol = 0;

    if(BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0 == enBatStatus)
    {
        post_vol = ABNORMAL_TEMP_LOW_POST_VOL_NEG3_0;
    }
	else if((BATTERY_STATUS__PRE_LOW_TEMP_0_5 == enBatStatus) || (BATTERY_STATUS__PRE_LOW_TEMP_5_12 == enBatStatus))
	{
		post_vol = ABNORMAL_TEMP_PRE_LOW_POST_VOL__0_12;
	}
	else if(BATTERY_STATUS__PRE_LOW_TEMP_12_16 == enBatStatus)
	{
		post_vol = ABNORMAL_TEMP_PRE_LOW_POST_VOL__12_16;
	}
	else if(BATTERY_STATUS__PRE_HIGH_TEMP == enBatStatus)
	{
		post_vol = ABNORMAL_TEMP_HIGH_POST_VOL;
	}
	else
	{
		post_vol = NORMAL_TEMP_POST_VOL;
	}

	return post_vol;
}
#endif

int bq24196_check_status(void)
{
    UINT8 reg_val=0;
    UINT8 fault_reasion=0;
	if(bq27541_di->alow_reading == false)
	{
		reg_val = 0;
	}
	else
	{
		if((fastchg_present_flag == 1)&&(fastchg_present_wait_count<=12))
		{
			bq24196_float_voltage_write(4350);
			reg_val = 0;
			fastchg_present_wait_count++;
			printk("bq24196_check_status,fastchg_present_flag = %d,fastchg_present_wait_count = %d\r\n",fastchg_present_flag,fastchg_present_wait_count);
		}
		else
		{
			reg_val = bq24196_registers_read_full();
			fastchg_present_flag = 0;
			fastchg_present_wait_count = 0;
		}
		
	}
    printk("%s,reg_val=0x%x,bat_vol=%d,BMT_status.bat_charging_state = %d\n",__func__,reg_val,BMT_status.bat_vol,BMT_status.bat_charging_state);
    
    #ifdef NEW_CHARGE_FULL_STOP
    if((reg_val == 1) || (bq24156a_post_status == 1) || (BMT_status.bat_charging_state == BQ24196_CHR_FULL))
	#else
	if((reg_val == 1)|| (BMT_status.bat_charging_state == BQ24196_CHR_FULL))
    {
        reg_val = BQ24196_STATUS_DONE;
    }
	else if(BMT_status.bat_charging_state == BQ24196_CHR_FAIL)
	{
		reg_val = BQ24196_STATUS_FAULT;
	}
	else
	{
		reg_val = BQ24196_STATUS_IN_PROGESS;
	}

    #endif

    switch(reg_val)
    {
        
        case BQ24196_STATUS_IN_PROGESS:
            BMT_status.bat_charging_state=BQ24196_CHR_CCCV;

            #ifdef NEW_CHARGE_FULL_STOP
            if(bq24156a_post_vol_count <= POST_VOL_COUNTS)
            {
                if(BMT_status.bat_vol > kernel_vendor_get_post_vol())
                {
                    bq24156a_post_vol_count++;
                }
                else
                {
                    bq24156a_post_vol_count = 0;
                }
            }
            else
            {
                bq24156a_post_vol_time++;
                
                if(BMT_status.ICharging < POST_CURRENT)
                {
                    bq24156a_post_current_count++;
                }
                else
                {
                    bq24156a_post_current_count =0;
                }
                if(bq24156a_post_current_count > POST_CURRENT_COUNTS)
                {
                    bq24156a_post_status = 1;//full
                }
            }
            
            if(bq24156a_post_vol_time > POST_VOL_TIMES)
            {
                bq24156a_post_status = 1;//full
            }
            printk("BMT_status.bat_vol = %d, BMT_status.ICharging = %d \r\n",BMT_status.bat_vol,BMT_status.ICharging);
            printk("post_vol_count=%d, post_current_count=%d, post_vol_time=%d,post_status=%d\r\n",bq24156a_post_vol_count,bq24156a_post_current_count,bq24156a_post_vol_time,bq24156a_post_status);
            #endif
            break;
        case BQ24196_STATUS_DONE:
            KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__FULL);
            break;
        case BQ24196_STATUS_FAULT:
            BMT_status.bat_charging_state=BQ24196_CHR_FAIL;
            printk("BMT_status.bat_charging_state=BQ24196_CHR_FAIL\r\n");
            BAT_BatteryStatusFailAction();
            break;
    }
    
}

static void bq24196_chr_cccv_mode_action(void)
{	
	if(Enable_BATDRV_LOG)
	{
	    printk("%s\n",__func__);
	}
	//do nothing
}


static kal_bool KernelVendorIsChgingOverTime(void)
{
	kal_bool bRtnVal;
	#ifdef NEW_CHGING_TIME
	if(max_charging_time_kernel <= BMT_status.total_charging_time)
	#else
	if(MAX_CHARGING_TIME <= BMT_status.total_charging_time)
	#endif
	{
		bRtnVal = KAL_TRUE;
	}
	else
	{
		bRtnVal = KAL_FALSE;
	}

	return bRtnVal;
}

static kal_bool KernelVendorVbatIsGood(void)
{
	//UINT32 nBatVol = 0;
	//nBatVol = BAT_Get_Battery_Voltage(0);

	kal_bool bRtnVal = KAL_TRUE;

	if(D_VBAT_TOO_HIGH_VOL <= BMT_status.bat_vol)
	{
		bRtnVal = KAL_FALSE;
	}
	else
	{
		bRtnVal = KAL_TRUE;
	}

	return bRtnVal;
}

static kal_bool KernelVendorVbatIsFull(void)
{
	kal_bool bRtnVal;

	if(D_VBAT_FULL_VOL <= BMT_status.bat_vol)
	{
		bRtnVal = KAL_TRUE;
	}
	else
	{
		bRtnVal = KAL_FALSE;
	}

	return bRtnVal;
}

static kal_bool KernelVendorVchgIsGood(void)
{
	#ifdef OPPO_CHARGER_RESUME
	static kal_bool bRtnVal = KAL_TRUE;
	#else
	kal_bool bRtnVal = KAL_TRUE;
	#endif
	UINT32 nVchg = check_charger_off_vol;
	TChgrStatus enChgrStatus = struExternalBMT.enChgrStatus;

	#define VCHG_CNT 2

	if(D_VCHG_PRE_HIGH < nVchg || D_VCHG_PRE_LOW > nVchg)
	{
		vchg_cnt++;
		if(vchg_cnt>VCHG_CNT)
		{
			vchg_cnt=0;
			bRtnVal = KAL_FALSE;

			if(D_VCHG_PRE_HIGH < nVchg)
			{
				enChgrStatus = CHARGER_STATUS__VOL_HIGH;
			}
			else
			{
				enChgrStatus = CHARGER_STATUS__VOL_LOW;
			}
		}
	}
	else
	{
		vchg_cnt=0;
		
		bRtnVal = KAL_TRUE;

		enChgrStatus = CHARGER_STATUS__GOOD;
	}

	if(enChgrStatus != struExternalBMT.enChgrStatus)
	{
		struExternalBMT.enChgrStatus = enChgrStatus;
	}

	return bRtnVal;
}

void BatteryTempBoundHandle(TBatStatus enBatStatus)
{
	if(enBatStatus == BATTERY_STATUS__HIGH_TEMP)
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH - HYSTERISIS_DECIDEGC;
	}
	else if(enBatStatus == BATTERY_STATUS__LOW_TEMP)
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW + HYSTERISIS_DECIDEGC;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	}
	else if(enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0)
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0 + HYSTERISIS_DECIDEGC;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	}
	else if(enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_0_5)
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5 + HYSTERISIS_DECIDEGC;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	}
	else if(enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_5_12)
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12 + HYSTERISIS_DECIDEGC;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	}
	else if(enBatStatus == BATTERY_STATUS__PRE_LOW_TEMP_12_16)
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16 + HYSTERISIS_DECIDEGC;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	}
	else if(enBatStatus == BATTERY_STATUS__GOOD)
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	}
	else if(enBatStatus == BATTERY_STATUS__PRE_HIGH_TEMP)
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH - HYSTERISIS_DECIDEGC;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	}
	else//removed
	{
		struBatteryTempBound.mBatteryTempBound_LOW = D_BAT_TEMP_LOW;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 = D_BAT_TEMP_PRE_LOW_0;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 = D_BAT_TEMP_PRE_LOW_5;
		struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 = D_BAT_TEMP_PRE_LOW_12;
		struBatteryTempBound.mBatteryTempBound_LOW_16 = D_BAT_TEMP_PRE_LOW_16;
		struBatteryTempBound.mBatteryTempBound_PRE_HIGH = D_BAT_TEMP_PRE_HIGH;
		struBatteryTempBound.mBatteryTempBound_HIGH = D_BAT_TEMP_HIGH;
	}
}
static kal_bool KernelVendorBatTempIsGood(void)
{
	static kal_bool bRtnVal = KAL_TRUE;
	INT32 nBatTemp = BMT_status.temperature;
	TBatStatus enBatStatus = struExternalBMT.enBatStatus;

	static int temp_cnt=0;
	#define TEMP_CNT 2

    if(BMT_status.charger_exist==KAL_TRUE)
    {
		if(struBatteryTempBound.mBatteryTempBound_HIGH < nBatTemp || struBatteryTempBound.mBatteryTempBound_LOW > nBatTemp)
		{
			temp_cnt++;
			if(temp_cnt>TEMP_CNT || plug_in_Temp_abnomal == 1)
			{
				temp_cnt=0;
				plug_in_Temp_abnomal = 0;
				bRtnVal = KAL_FALSE;

				if(D_BAT_REMOVED_TEMP >= nBatTemp)
				{
					enBatStatus = BATTERY_STATUS__REMOVED;
				}
				else if(struBatteryTempBound.mBatteryTempBound_HIGH < nBatTemp)
				{
					enBatStatus = BATTERY_STATUS__HIGH_TEMP;
				}
				else 
				{
					enBatStatus = BATTERY_STATUS__LOW_TEMP;
				}
			}
		}
		else
		{
			temp_cnt=0;
			
			bRtnVal = KAL_TRUE;

			if(struBatteryTempBound.mBatteryTempBound_PRE_HIGH <= nBatTemp)
			{
				enBatStatus = BATTERY_STATUS__PRE_HIGH_TEMP;
			}
			else if((struBatteryTempBound.mBatteryTempBound_LOW <= nBatTemp)&&(struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 > nBatTemp))
			{
				enBatStatus = BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0;
			}	
			else if((struBatteryTempBound.mBatteryTempBound_PRE_LOW_0 <= nBatTemp)&&(struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 > nBatTemp))
			{
				enBatStatus = BATTERY_STATUS__PRE_LOW_TEMP_0_5;
			}
			else if((struBatteryTempBound.mBatteryTempBound_PRE_LOW_5 <= nBatTemp)&&(struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 > nBatTemp))
			{
				enBatStatus = BATTERY_STATUS__PRE_LOW_TEMP_5_12;
			}
			else if((struBatteryTempBound.mBatteryTempBound_PRE_LOW_12 <= nBatTemp)&&(struBatteryTempBound.mBatteryTempBound_LOW_16 > nBatTemp))
			{
				enBatStatus = BATTERY_STATUS__PRE_LOW_TEMP_12_16;
			}
			else
			{
				enBatStatus = BATTERY_STATUS__GOOD;
			}
		}

		if(BATTERY_STATUS__REMOVED == enBatStatus)
		{
			BMT_status.bat_exist = KAL_FALSE;
		}
		else
		{
			BMT_status.bat_exist = KAL_TRUE;
		}

		if(enBatStatus != struExternalBMT.enBatStatus)
		{
			struExternalBMT.enBatStatus = enBatStatus;
			TempIsChanged = KAL_TRUE;
			BatteryTempBoundHandle(enBatStatus);

			//bq24156a_start_charging();
		}
    }
	else
	{
		bRtnVal=KAL_TRUE;
	}

	return bRtnVal;
}

int kernel_get_bat_health(void)
{
	int bat_health= POWER_SUPPLY_HEALTH_GOOD;
	TBatStatus enBatStatus = struExternalBMT.enBatStatus ;

	if((enBatStatus==BATTERY_STATUS__REMOVED)
	||(enBatStatus==BATTERY_STATUS__HIGH_TEMP)
	||(enBatStatus==BATTERY_STATUS__LOW_TEMP))
	{
		bat_health=POWER_SUPPLY_HEALTH_DEAD;
	}
	else if(enBatStatus==BATTERY_STATUS__PRE_HIGH_TEMP)
	{
		bat_health=POWER_SUPPLY_HEALTH_OVERHEAT;
	}
	//else if((enBatStatus==BATTERY_STATUS__PRE_LOW_TEMP)||(enBatStatus==BATTERY_STATUS__PRE_LOW_TEMP2))
	else if(enBatStatus==BATTERY_STATUS__PRE_LOW_TEMP_NEG3_0)
	{
		bat_health=POWER_SUPPLY_HEALTH_COLD;
	}
	else 
	{
		bat_health=POWER_SUPPLY_HEALTH_GOOD;
	}

	return bat_health;
}



static void KernelVendorProtectionCheck(void)
{
	
#ifdef FEATURE_BAT_TEMP_PROTECT
	if(KAL_FALSE == KernelVendorBatTempIsGood())
	{
		printk("KernelVendorBatTempIsGood func ,false!");
		KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__BATTTEMP_ABNORMAL);
	}
	else
	{
//		if(struExternalBMT.nChgStopVoterMask == CHG_STOP_VOTER__BATTTEMP_ABNORMAL)
		if((struExternalBMT.nChgStopVoterMask & CHG_STOP_VOTER__BATTTEMP_ABNORMAL) == CHG_STOP_VOTER__BATTTEMP_ABNORMAL)			
		{
			printk("KernelVendorBatTempIsGood func ,true! To Normal\n");
  			KernelVendorVoteToStopCharging(CHG_CMD_DISABLE, CHG_STOP_VOTER__BATTTEMP_ABNORMAL);
		}
	}
#endif
	
#ifdef FEATURE_VCHG_PROTECT
#ifndef OPPO_CHARGER_RESUME
	if(KAL_FALSE == KernelVendorVchgIsGood())
	{
		printk("KernelVendorVchgIsGood func ,false!");
		KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__VCHG_ABNORMAL);
	}
#else
	if(bq27541_di->fast_chg_started == false)
	{
		if((KAL_FALSE == KernelVendorVchgIsGood()) || (over_charger_error_count &0x7f) > OVER_CHARGER_RESUME_COUNTS)
		{
			if(struExternalBMT.enChgrStatus == CHARGER_STATUS__VOL_HIGH)
			{
				bq24196_charging_CurrentForOvp();
				printk("bq24196_charging_CurrentForOvp func,Vcharg is high!!\n");
			}
			KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__VCHG_ABNORMAL);
	        if((over_charger_error_count&0x80) == 0)
	        {
	            over_charger_error_count++;
	        }
			over_charger_error_count |= 0x80;
			printk("KernelVendorVchgIsGood func ,false! error_count = 0x%x\r\n", over_charger_error_count);
		}
		else
		{
			printk("KernelVendorVchgIsGood func ,charger is good!  error_count = 0x%x\r\n", over_charger_error_count);
		    if(((over_charger_error_count & 0x80) == 0x80) && (over_charger_error_count &0x7f) < OVER_CHARGER_RESUME_COUNTS)
		    {
	            over_charger_error_count &= 0x7f;
		        KernelVendorVoteToStopCharging(CHG_CMD_DISABLE, CHG_STOP_VOTER__VCHG_ABNORMAL); 
		        printk("KernelVendorVchgIsGood func ,over charger resume!  error_count = 0x%x\r\n", over_charger_error_count);
		    }
		}
	}
#endif
#endif
	
#ifdef FEATURE_VBAT_PROTECT
	if(KAL_FALSE == KernelVendorVbatIsGood())
	{
		printk("KernelVendorVbatIsGood func ,false!");
		KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__VBAT_TOO_HIGH);
	}

	if(KAL_TRUE == KernelVendorVbatIsFull())
	{
		printk("KernelVendorVbatIsFull func ,true!");
		KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__FULL);
	}
#endif
	
#ifdef FEATURE_MAX_CHGING_TIME_PROTECT
	//Fanhong.Kong@BaiscDrv.CHG, modified 2012/5/19 overtime in all temp
	//if(KAL_TRUE == KernelVendorIsChgingOverTime()&& TRUE==kernel_is_battery_temp_good()) //not restrict in pre temp	
	if(KAL_TRUE == KernelVendorIsChgingOverTime())
	{
		printk("KernelVendorIsChgingOverTime func ,true!");
		KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__MAX_CHGING_TIME);
	}
#endif
}

void mt_battery_charging_switch()
{
	switch(BMT_status.bat_charging_state)
	{
		case BQ24196_CHR_CCCV:
			bq24196_chr_cccv_mode_action();
			break;
		case BQ24196_CHR_FULL:
			BAT_BatteryFullAction();
			break;
		case BQ24196_CHR_FAIL:
			BAT_BatteryStatusFailAction();
			break;
	}
}

static void KernelVendorPrintLog(void)
{
#ifdef FEATURE_PRINT_CHGR_LOG
	printk("[VENDOR CHGR] chgr_exit = %d, chgr_type = %d,chgr_vol = %d, chgr_off_vol = %d\n",
		BMT_status.charger_exist, BMT_status.charger_type,BMT_status.charger_vol, check_charger_off_vol);
#endif

#ifdef FEATURE_PRINT_BAT_LOG
	printk("[VENDOR BAT] bat_exit = %d, bat_full = %d, temp = %d, over_bat_vol = %d, bat_vol = %d, battery_status = %d\n",
		BMT_status.bat_exist,BMT_status.bat_full,BMT_status.temperature,struExternalBMT.bOverBatVol,BMT_status.bat_vol, oppo_high_battery_status);
#endif

#ifdef FEATURE_PRINT_ICHGING_LOG
	printk("[VENDOR ICHGING] V_BAT_SENSE = %d, ICHGING = %d\n",
		BMT_status.ADC_BAT_SENSE, BMT_status.ICharging);
#endif

#ifdef FEATURE_PRINT_VOTE_LOG
	printk("[VENDOR VOTE] voter_mask = 0x%x, chging_on = %d, total_time = %d,  nCurSet = %d, g_temp_CC_value=%d,  SOC = %d,bat_volt_check_point = %d\n",
		struExternalBMT.nChgStopVoterMask, struExternalBMT.bChgingOn, BMT_status.total_charging_time, struExternalBMT.enChgingCurSet, g_temp_CC_value ,BMT_status.SOC,bat_volt_check_point);
#endif

#ifdef FEATURE_PRINT_STATUS_LOG
	printk("[VENDOR STATUS] chging_sta = 0x%x, bat_sta = %d, chgr_sta = %d,recharging_state = %d, otg_switch =%d, charge_on_cnt = %d, OPPO_LED_ON = %d, fast_reset_count = %d, plug_in_set_current=%d, selfadapt_delay_count=%d, plug_in_selfadapt=%d, vol_count_temp5_12=%d, vol_count_flag=%d\n",
		BMT_status.bat_charging_state,struExternalBMT.enBatStatus,struExternalBMT.enChgrStatus,BMT_status.bat_in_recharging_state, usb_main.otg_switch, charge_on_cnt, OPPO_LED_ON, fast_charger_reset_count, plug_in_flag_set_charging_current, selfadapt_delay_count, plug_in_flag_selfadapt,vol_count_temp5_12,vol_count_flag);
#endif

#ifdef FEATURE_PRINT_TIME_LOG
#endif

#ifdef FEATURE_PRINT_INTERMITTENT_LOG
#endif

#ifdef FEATURE_PRINT_OTHER_LOG
	printk("[VENDOR GLOBAL] g_HW_Charging_Done=%d, g_boot_reason = %d, g_rtc_soc = %d, g_point_by_v = %d,battery_suspended = %d,g_hw_version = %d\r\n",g_HW_Charging_Done, g_boot_reason, g_rtc_soc, g_point_by_v,battery_suspended, g_hw_version);
#endif

#ifdef FEATURE_PRINT_FASTCHG_LOG	
	KernelVendorFastchgLog();
#endif
	
}
int opchg_get_charger_type(void)
{
	return BMT_status.charger_type;
}

bool is_alow_fast_chg()
{
	bool auth = false;
	int temp = 0;
	int cap = 0;
	int chg_type = 0;
	bool low_temp_full = 0;
	int batt_vol = 0;
	
	auth = opchg_get_prop_authenticate();
	temp = opchg_get_prop_batt_temp();
	cap = bat_volt_check_point;
	chg_type = opchg_get_charger_type();
	low_temp_full = opchg_get_fast_low_temp_full();
//	batt_vol = opchg_get_prop_battery_voltage_now();
	printk("%s auth:%d,temp:%d,cap:%d,chg_type:%d,low_temp_full:%d\n",__func__,auth,temp,cap,chg_type,low_temp_full);

	if(auth == false)
		return false;
	if((chg_type != STANDARD_CHARGER) && (chg_type != NONSTANDARD_CHARGER) && (chg_type != APPLE_2_1A_CHARGER) && (chg_type != APPLE_1_0A_CHARGER))
		return false;
#if 1//ndef CONFIG_OPPO_DEVICE_FIND7OP
/* jingchun.wang@Onlinerd.Driver, 2014/02/25  Modify for use different temp range of 14001 */
#if 0
	if(temp < 150)
		return false;
	if((temp < 155) && (low_temp_full == 1)){
		return false;
	}
#else
	if(temp < 165){
		return false;
	}
#endif	
#else /*CONFIG_OPPO_DEVICE_FIND7OP*/
	if(temp < 205)
		return false;
#endif /*CONFIG_OPPO_DEVICE_FIND7OP*/
	if(temp > 430){
		return false;
	}
	if(cap < 1)
		return false;
	//if(cap > 96)
	if(cap > 85){
		return false;
	}
#if 0
	if(batt_vol < 3450*1000){
		return false;
	}
#endif
	if(opchg_get_prop_fast_switch_to_normal() == true){
		printk("%s fast_switch_to_noraml is true\n",__func__);
		return false;
	}
	if(battery_main.BAT_MMI_CHG == 0){
		printk("%s MMI fastcharger is false\n",__func__);
		return false;
	}
	return true;
}


static void switch_fast_chg()
{
	int ret = 0;
	//chek if fast charging
	if(mt_get_gpio_out(gpio_oppo_vooc_sw_ctrl)==1)
	{
		printk("fast_chg gpio is high,return\n");
		return;
	}
	//check fast 	
	if(opchg_get_prop_fast_chg_allow() == false){
		if(is_alow_fast_chg() == true) {
			// add reset mcu 
			ret =opchg_set_reset_active();
			// set switch
			ret =opchg_set_switch_mode(VOOC_CHARGER_MODE);
			
			if(ret){
				pr_err("%s switch fast error %d\n", __func__, ret);
			}
			opchg_set_fast_chg_allow(true);
		}
	}
	printk("%s end,allow_fast_chg:%d\n",__func__,opchg_get_prop_fast_chg_allow());
}

void mt_fast_chg_switch_check(void)
{
	int ret = 0;
	if((BMT_status.charger_type==STANDARD_CHARGER) || (BMT_status.charger_type==NONSTANDARD_CHARGER) ||(BMT_status.charger_type==APPLE_2_1A_CHARGER) || (BMT_status.charger_type==APPLE_1_0A_CHARGER))
	{
		if(opchg_get_prop_fast_chg_started() == true) {
			switch_fast_chg();	
			#ifdef OPPO_USE_FAST_CHARGER_RESET_MCU
			fast_charger_reset_sign = KAL_TRUE;
			fast_charger_reset_count = 0;
			#endif
			if((struExternalBMT.bChgingOn == 1) && (opchg_get_fast_chg_ing() == 1)){
				pchr_turn_off_charging_bq24196();
			}			
		}
		else
		{
			switch_fast_chg();
			#ifdef OPPO_USE_FAST_CHARGER_RESET_MCU
			if(is_alow_fast_chg() == true){
			
				if(fast_charger_reset_sign == KAL_FALSE)
				{
					fast_charger_reset_count++;
					if(fast_charger_reset_count > 4)
					{
						//add reset mcu
						 ret = opchg_set_reset_active();
						//set switch	
						 ret = opchg_set_switch_mode(VOOC_CHARGER_MODE);
						fast_charger_reset_sign = KAL_TRUE;
						fast_charger_reset_count = 0;
						if(ret){
							pr_err("%s switch fast error %d,when reset\n", __func__, ret);
						}
						printk("%s switch fast,when reset\n", __func__);
					}
				}			
			}
		#endif
		}
	}
}

void BAT_thread(void)
{
    static kal_bool  battery_meter_initilized = KAL_FALSE;
    
    if(battery_meter_initilized == KAL_FALSE)
    {
        battery_meter_initial();	//move from battery_probe() to decrease booting time
       	BMT_status.nPercent_ZCV = battery_meter_get_battery_nPercent_zcv();
        battery_meter_initilized = KAL_TRUE;
		#ifdef IPOD_CHARGING_STATUS
		if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT)
			g_ipod_charging = KAL_TRUE;
		#endif
    }

    mt_battery_charger_detect_check();
	opchg_selfadapt_work();
    mt_battery_GetBatteryData();

  	//mt_battery_thermal_check();
 
    mt_battery_notify_check(); 
	if(bq27541_di->alow_reading == true) {
		bq24196_kick_wdt();
	}
    if( BMT_status.charger_exist == KAL_TRUE )
    {
        //mt_battery_CheckBatteryStatus();	
        //mt_battery_charging_algorithm();
		bq24196_check_status();	  
		KernelVendorProtectionCheck();
		mt_fast_chg_switch_check();
    }
	printk("%s charge_mode,mt_get_gpio_in(%d):%d\n",__func__,gpio_oppo_vooc_sw_ctrl,mt_get_gpio_in(gpio_oppo_vooc_sw_ctrl));
	mt_battery_charging_switch();
	BMT_status.total_charging_time += BAT_TASK_PERIOD;
	KernelVendorPrintLog();
	
	
//	if(bq27541_di->alow_reading == true){
	if((bq27541_di->alow_reading == true) && (Enable_BATDRV_LOG == 2)) {
		bq24196_dumps();
	}	
    mt_battery_update_status();
	mt_kpoc_power_off_check();
}

extern volatile kal_bool chargin_hw_init_done_bq24196;
volatile kal_bool chargin_hw_init_done_vooc = KAL_FALSE;
extern volatile kal_bool chargin_hw_init_done_bq27541;
///////////////////////////////////////////////////////////////////////////////////////////
//// Internal API
///////////////////////////////////////////////////////////////////////////////////////////
int bat_thread_kthread(void *x)
{
    ktime_t ktime = ktime_set(3, 0);  // 10s, 10* 1000 ms	
    
    /* Run on a process content */  
    while (1) {               
        mutex_lock(&bat_mutex);
          
		if((chargin_hw_init_done_bq24196 == KAL_TRUE) && (battery_suspended == KAL_FALSE) && (chargin_hw_init_done_vooc == KAL_TRUE)  && (chargin_hw_init_done_bq27541 == KAL_TRUE))
		{
			#ifdef IPOD_CHARGING_STATUS
			if(g_ipod_charging && OPPO_LED_ON && g_ipod_init_done)
			{
				//BAT_thread();
				battery_xlog_printk(BAT_LOG_FULL, "******** MT6320 battery : no bat_thread_kthread : g_ipod_charging =%d, OPPO_LED_ON = %d,g_ipod_init_done = %d********\n",g_ipod_charging,OPPO_LED_ON,g_ipod_init_done);
				
			}
			else
			{
				BAT_thread();
			}
			#else
			BAT_thread();
			#endif                    
		}
        mutex_unlock(&bat_mutex);
    
        battery_xlog_printk(BAT_LOG_FULL, "wait event \n" );

		wait_event(bat_thread_wq, (bat_thread_timeout == KAL_TRUE));
	
        bat_thread_timeout = KAL_FALSE;
        hrtimer_start(&battery_kthread_timer, ktime, HRTIMER_MODE_REL);   
        ktime = ktime_set(BAT_TASK_PERIOD, 0);  // 10s, 10* 1000 ms
        if( chr_wake_up_bat == KAL_TRUE && g_smartbook_update != 1)	// for charger plug in/ out
        {
            g_smartbook_update = 0;
            battery_meter_reset();
            chr_wake_up_bat = KAL_FALSE;
			            
            battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Charger plug in/out, Call battery_meter_reset. (%d)\n", BMT_status.UI_SOC);
        }
        
    }

    return 0;
}

void bat_thread_wakeup(void)
{
    battery_xlog_printk(BAT_LOG_FULL, "******** battery : bat_thread_wakeup  ********\n" );
    
    bat_thread_timeout = KAL_TRUE;
    bat_meter_timeout = KAL_TRUE;
#ifdef MTK_ENABLE_AGING_ALGORITHM
    suspend_time = 0;
#endif
    wake_up(&bat_thread_wq);    
}
///////////////////////////////////////////////////////////////////////////////////////////
//// fop API 
///////////////////////////////////////////////////////////////////////////////////////////
static long adc_cali_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int *user_data_addr;
    int *naram_data_addr;
    int i = 0;
    int j = 0;
    int ret = 0;
	int adc_in_data[2] = {1,1};
	int adc_out_data[2] = {1,1};

    static int at_i_sense=0,at_current_offset=0,at_charger_off_vol = 5000 * 5;
    mutex_lock(&bat_mutex);

    switch(cmd)
    {
        case TEST_ADC_CALI_PRINT :
            g_ADC_Cali = KAL_FALSE;
            break;
        
        case SET_ADC_CALI_Slop:            
            naram_data_addr = (int *)arg;
            ret = copy_from_user(adc_cali_slop, naram_data_addr, 36);
            g_ADC_Cali = KAL_FALSE; /* enable calibration after setting ADC_CALI_Cal */            
            /* Protection */
            for (i=0;i<14;i++) 
            { 
                if ( (*(adc_cali_slop+i) == 0) || (*(adc_cali_slop+i) == 1) ) {
                    *(adc_cali_slop+i) = 1000;
                }
            }
            for (i=0;i<14;i++) battery_xlog_printk(BAT_LOG_CRTI, "adc_cali_slop[%d] = %d\n",i , *(adc_cali_slop+i));
            battery_xlog_printk(BAT_LOG_FULL, "**** unlocked_ioctl : SET_ADC_CALI_Slop Done!\n");            
            break;    
            
        case SET_ADC_CALI_Offset:            
            naram_data_addr = (int *)arg;
            ret = copy_from_user(adc_cali_offset, naram_data_addr, 36);
            g_ADC_Cali = KAL_FALSE; /* enable calibration after setting ADC_CALI_Cal */
            for (i=0;i<14;i++) battery_xlog_printk(BAT_LOG_CRTI, "adc_cali_offset[%d] = %d\n",i , *(adc_cali_offset+i));
            battery_xlog_printk(BAT_LOG_FULL, "**** unlocked_ioctl : SET_ADC_CALI_Offset Done!\n");            
            break;
            
        case SET_ADC_CALI_Cal :            
            naram_data_addr = (int *)arg;
            ret = copy_from_user(adc_cali_cal, naram_data_addr, 4);
            g_ADC_Cali = KAL_TRUE;
            if ( adc_cali_cal[0] == 1 ) {
                g_ADC_Cali = KAL_TRUE;
            } else {
                g_ADC_Cali = KAL_FALSE;
            }            
            for (i=0;i<1;i++) battery_xlog_printk(BAT_LOG_CRTI, "adc_cali_cal[%d] = %d\n",i , *(adc_cali_cal+i));
            battery_xlog_printk(BAT_LOG_FULL, "**** unlocked_ioctl : SET_ADC_CALI_Cal Done!\n");            
            break;    

        case ADC_CHANNEL_READ:            
            //g_ADC_Cali = KAL_FALSE; /* 20100508 Infinity */
            user_data_addr = (int *)arg;
            ret = copy_from_user(adc_in_data, user_data_addr, 8); /* 2*int = 2*4 */
          
            if( adc_in_data[0] == 0 ) // I_SENSE
            {
            	pchr_turn_on_charging_bq24196();
            	msleep(10);
				
				while(j < 3)
                {

                    j++;
            	    at_i_sense =  opchg_get_prop_current_now() * adc_in_data[1];
                    printk("ioctl read I_SENSE,adc_in_data[1] = %d,opchg_get_prop_current_now = %d\r\n",adc_in_data[1], at_i_sense);

                	if((abs(at_i_sense) > 1250) && (abs(at_i_sense) < 2500)) 
                	{
                        printk("ioctl opchg_get_prop_current_now,----------break---------------------while  j = %d\r\n",j);
						j = 0;
                	    break;
                	}
                }
                j = 0;
				adc_out_data[0]= abs(at_i_sense);
            	printk("ioctl read I_SENSE,adc_out_data[0]=%d\n",adc_out_data[0]);
            	
            }
			else if( adc_in_data[0] == 1 ) // BAT_SENSE
			{ 					
                pchr_turn_off_charging_bq24196();
            	msleep(10);
            	adc_out_data[0] = get_bat_sense_volt(adc_in_data[1]) * adc_in_data[1];
				adc_out_data[0] -=  10 * adc_in_data[1]; 
            	printk("ioctl read BAT_SENSE, adc_out_data[0]=%d\n",adc_out_data[0]);
            	//at_test_chg_on=0;
			}
			else if( adc_in_data[0] == 3 ) // V_Charger
			{
				adc_out_data[0] = at_charger_off_vol;
				printk("at_charger_off_vol = %d\r\n",at_charger_off_vol);
			}
#ifdef VENDOR_EDIT//Fanhong.Kong@ProDrv.CHG, add 2013/7/24 for AT test
			else if( adc_in_data[0] == 4 ) //CHARGER_TYPE
			{				 
				adc_out_data[0] =  mt_charger_type_detection();
				printk("mt_charger_type_detection-------------adc_out_data[0] = %d\r\n", adc_out_data[0]);
			}
#endif/*VENDOR_EDIT*/	
			else if( adc_in_data[0] == 30 ) // V_Bat_temp magic number
			{
				adc_out_data[0] = BMT_status.temperature * adc_in_data[1];				
			}
			else if( adc_in_data[0] == 66 ) 
			{
				adc_out_data[0] = (gFG_current)/10;
				
				if (gFG_Is_Charging == KAL_TRUE) 
			    {			    	
			        adc_out_data[0] = 0 - adc_out_data[0]; //charging
			    }			    				
			}
			else
			{
				adc_out_data[0] = PMIC_IMM_GetOneChannelValue(adc_in_data[0],adc_in_data[1], 1) * adc_in_data[1];
			}
            
            if (adc_out_data[0]<0)
                adc_out_data[1]=1; /* failed */
            else
                adc_out_data[1]=0; /* success */

            if( adc_in_data[0] == 30 )
                adc_out_data[1]=0; /* success */

            if( adc_in_data[0] == 66 )
                adc_out_data[1]=0; /* success */
                
            ret = copy_to_user(user_data_addr, adc_out_data, 8);
            battery_xlog_printk(BAT_LOG_CRTI, "**** unlocked_ioctl : Channel %d * %d times = %d\n", adc_in_data[0], adc_in_data[1], adc_out_data[0]);            
            break;

        case BAT_STATUS_READ:            
            user_data_addr = (int *)arg;
            ret = copy_from_user(battery_in_data, user_data_addr, 4); 
            /* [0] is_CAL */
            if (g_ADC_Cali) {
                battery_out_data[0] = 1;
            } else {
                battery_out_data[0] = 0;
            }
            ret = copy_to_user(user_data_addr, battery_out_data, 4); 
            battery_xlog_printk(BAT_LOG_CRTI, "**** unlocked_ioctl : CAL:%d\n", battery_out_data[0]);                        
            break;        

        case Set_Charger_Current: /* For Factory Mode*/
            user_data_addr = (int *)arg;
            ret = copy_from_user(charging_level_data, user_data_addr, 4);
            g_ftm_battery_flag = KAL_TRUE;            
            if( charging_level_data[0] == 0 ) {             charging_level_data[0] = CHARGE_CURRENT_70_00_MA;
            } else if ( charging_level_data[0] == 1  ) {    charging_level_data[0] = CHARGE_CURRENT_200_00_MA;
            } else if ( charging_level_data[0] == 2  ) {    charging_level_data[0] = CHARGE_CURRENT_400_00_MA;
            } else if ( charging_level_data[0] == 3  ) {    charging_level_data[0] = CHARGE_CURRENT_450_00_MA;
            } else if ( charging_level_data[0] == 4  ) {    charging_level_data[0] = CHARGE_CURRENT_550_00_MA;
            } else if ( charging_level_data[0] == 5  ) {    charging_level_data[0] = CHARGE_CURRENT_650_00_MA;
            } else if ( charging_level_data[0] == 6  ) {    charging_level_data[0] = CHARGE_CURRENT_700_00_MA;
            } else if ( charging_level_data[0] == 7  ) {    charging_level_data[0] = CHARGE_CURRENT_800_00_MA;
            } else if ( charging_level_data[0] == 8  ) {    charging_level_data[0] = CHARGE_CURRENT_900_00_MA;
            } else if ( charging_level_data[0] == 9  ) {    charging_level_data[0] = CHARGE_CURRENT_1000_00_MA;
            } else if ( charging_level_data[0] == 10 ) {    charging_level_data[0] = CHARGE_CURRENT_1100_00_MA;
            } else if ( charging_level_data[0] == 11 ) {    charging_level_data[0] = CHARGE_CURRENT_1200_00_MA;
            } else if ( charging_level_data[0] == 12 ) {    charging_level_data[0] = CHARGE_CURRENT_1300_00_MA;
            } else if ( charging_level_data[0] == 13 ) {    charging_level_data[0] = CHARGE_CURRENT_1400_00_MA;
            } else if ( charging_level_data[0] == 14 ) {    charging_level_data[0] = CHARGE_CURRENT_1500_00_MA;
            } else if ( charging_level_data[0] == 15 ) {    charging_level_data[0] = CHARGE_CURRENT_1600_00_MA;
            } else { 
                charging_level_data[0] = CHARGE_CURRENT_450_00_MA;
            }
            wake_up_bat();
            battery_xlog_printk(BAT_LOG_CRTI, "**** unlocked_ioctl : set_Charger_Current:%d\n", charging_level_data[0]);
            break;
	//add for meta tool-------------------------------
	case Get_META_BAT_VOL:
		user_data_addr = (int *)arg;
    		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = BMT_status.bat_vol;
		ret = copy_to_user(user_data_addr, adc_out_data, 8); 
    		//xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "**** unlocked_ioctl : BAT_VOL:%d\n", adc_out_data[0]);   
		break;
	case Get_META_BAT_SOC:
		user_data_addr = (int *)arg;
    		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = BMT_status.UI_SOC;
		ret = copy_to_user(user_data_addr, adc_out_data, 8); 
    		//xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "**** unlocked_ioctl : SOC:%d\n", adc_out_data[0]);   
		break;
		//add bing meta tool-------------------------------
	case Get_FakeOff_Param: /* For Factory Mode*/
		user_data_addr = (int *)arg;
		fakeoff_out_data[0] = bat_volt_check_point;
		fakeoff_out_data[1] = g_NotifyFlag;
		if(pmic_chrdet_status() == KAL_TRUE)
		{
			fakeoff_out_data[2] = 1;
		}
		else
		{
			fakeoff_out_data[2] = 0;
		}
		fakeoff_out_data[3] = opchg_get_prop_fast_chg_started();
		#ifdef IPOD_CHARGING_STATUS
		g_ipod_init_done = KAL_TRUE;
		#endif
		ret = copy_to_user(user_data_addr, fakeoff_out_data, 16);
		printk("ioctl : Get_FakeOff_Param:bat_volt_check_point:%d, g_NotifyFlag:%d,chr_det:%d,fast_chg = %d,g_ipod_init_done = %d\n",fakeoff_out_data[0],fakeoff_out_data[1],fakeoff_out_data[2],fakeoff_out_data[3],g_ipod_init_done);
		break; 
			
	case Get_Notify_Param: /* For Fakeoff Mode*/
		user_data_addr = (int *)arg;
		notify_out_data[0] = g_NotifyFlag;
		ret = copy_to_user(user_data_addr, notify_out_data, 4);
		printk("ioctl : Get_Notify_Param: g_NotifyFlag:%d\n",g_NotifyFlag); 
		break; 	
	case Turn_Off_Charging: /* For Turnoffcharging Mode*/
		pchr_turn_off_charging_bq24196();
		struExternalBMT.bChgingOn = KAL_FALSE;
		break;

	case K_AT_CHG_CHGR_IN:
		user_data_addr = (int *)arg;
		if(BMT_status.charger_exist)
		{
			auto_out_data[0]=1;
			if(BMT_status.charger_type==STANDARD_CHARGER)
			{
				auto_out_data[1]=0x01;
			}
			else if(BMT_status.charger_type==NONSTANDARD_CHARGER)
			{
				auto_out_data[1]=0x02;
			}
			else
			{
				auto_out_data[1]=0;//usb
			}
		}
		else
		{
			auto_out_data[0]=0;
			auto_out_data[1]=0;
		}
		ret = copy_to_user(user_data_addr, auto_out_data, 8);
		if(ret<0)
		{
			printk("AT_CHG_CHGR_IN,copy_to_user return val error\n");
		}
		

		printk("AT_CHG_CHGR_ON\N");
		for(i=0;i<3;i++)
		{
			 printk("auto_out_data[%d]=%d\n",i,auto_out_data[i]);
		}
		break;
	case K_AT_CHG_CHGR_OFF:
		user_data_addr = (int *)arg;
		if(BMT_status.charger_exist)
		{
			auto_out_data[0]=1;
		}
		else
		{
			auto_out_data[0]=0;
		}
		ret = copy_to_user(user_data_addr, auto_out_data, 4);
		if(ret<0)
		{
			printk("AT_CHG_CHGR_OFF,copy_to_user return val error\n");
		}
		
		printk("AT_CHG_CHGR_OFF\N");
		for(i=0;i<5;i++)
		{
			 printk("auto_out_data[%d]=%d\n",i,auto_out_data[i]);
		}
		break;
	case K_AT_CHG_ON:
		at_test_chg_on=1;
		#if 0
		at_charger_off_vol = get_charger_volt(5) * 5;
		#else
		at_charger_off_vol = battery_meter_get_charger_voltage() * 5;
		#endif
		pchr_turn_on_charging_bq24196();
		printk("AT_CHG_ON\N");
		break;
	case K_AT_CHG_OFF:
		at_test_chg_on=0;
		//pchr_turn_off_charging_smb358();
		bq24196_suspend_charger();
		printk("AT_CHG_OFF\N");
		break;
	case K_AT_CHG_INFO:
		user_data_addr = (int *)arg;	
		
		auto_out_data[0]=auto_test_revert_g_notify_flag();
		auto_out_data[1]=g_BatteryAverageCurrent;
		auto_out_data[2]=BMT_status.temperature;
		auto_out_data[3]=BMT_status.bat_vol;
		auto_out_data[4]=BMT_status.charger_vol;
		ret = copy_to_user(user_data_addr, auto_out_data, 20);
		printk("K_AT_CHG_INFO\N");
		for(i=0;i<5;i++)
		{
			 printk("auto_out_data[%d]=%d\n",i,auto_out_data[i]);
		}
		break;
	case SET_SPI_CS_LOW:
		//if(mt_set_gpio_out(83,GPIO_OUT_ZERO))
		//{printk("set gpio failed!! \n");}
		break;
	default:
		g_ADC_Cali = KAL_FALSE;
		break;
    }

    mutex_unlock(&bat_mutex);
    
    return 0;
}
#ifdef CONFIG_COMPAT
static long adc_cali_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = 0;
	
	void __user *arg64 = compat_ptr(arg);

	printk(KERN_ERR "%s cmd = 0x%04x", __FUNCTION__, cmd);

	if(!file->f_op || !file->f_op->unlocked_ioctl)
	{
		printk(KERN_ERR "file->f_op OR file->f_op->unlocked_ioctl is null!\n");
		return -ENOTTY;
	}

    switch(cmd)
    {
        case COMPAT_TEST_ADC_CALI_PRINT :
            ret = file->f_op->unlocked_ioctl(file, COMPAT_TEST_ADC_CALI_PRINT, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_TEST_ADC_CALI_PRINT is failed!\n");
			}
			break;
        
        case COMPAT_SET_ADC_CALI_Slop:            
            ret = file->f_op->unlocked_ioctl(file, COMPAT_SET_ADC_CALI_Slop, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_SET_ADC_CALI_Slop is failed!\n");
			}
			break;   
            
        case COMPAT_SET_ADC_CALI_Offset:            
            ret = file->f_op->unlocked_ioctl(file, COMPAT_SET_ADC_CALI_Offset, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_SET_ADC_CALI_Offset is failed!\n");
			}
			break;
            
        case COMPAT_SET_ADC_CALI_Cal :            
           ret = file->f_op->unlocked_ioctl(file, COMPAT_SET_ADC_CALI_Cal, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_SET_ADC_CALI_Cal is failed!\n");
			}
			break;   

        case COMPAT_ADC_CHANNEL_READ:            
            ret = file->f_op->unlocked_ioctl(file, COMPAT_ADC_CHANNEL_READ, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_ADC_CHANNEL_READ is failed!\n");
			}
			break;
        case COMPAT_BAT_STATUS_READ:            
            ret = file->f_op->unlocked_ioctl(file, COMPAT_BAT_STATUS_READ, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_BAT_STATUS_READ is failed!\n");
			}
			break;   

        case COMPAT_Set_Charger_Current: /* For Factory Mode*/
            ret = file->f_op->unlocked_ioctl(file, COMPAT_Set_Charger_Current, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_Set_Charger_Current is failed!\n");
			}
			break;
	//add for meta tool-------------------------------
		case COMPAT_Get_META_BAT_VOL:
			ret = file->f_op->unlocked_ioctl(file, COMPAT_Get_META_BAT_VOL, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_Get_META_BAT_VOL is failed!\n");
			}
			break;
		case COMPAT_Get_META_BAT_SOC:
			ret = file->f_op->unlocked_ioctl(file, COMPAT_Get_META_BAT_SOC, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_Get_META_BAT_SOC is failed!\n");
			}
			break;
			//add bing meta tool-------------------------------
		case COMPAT_Get_FakeOff_Param: /* For Factory Mode*/
			ret = file->f_op->unlocked_ioctl(file, COMPAT_Get_FakeOff_Param, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_Get_FakeOff_Param is failed!\n");
			}
			break;			
		case COMPAT_Get_Notify_Param: /* For Fakeoff Mode*/
			ret = file->f_op->unlocked_ioctl(file, COMPAT_Get_Notify_Param, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_Get_Notify_Param is failed!\n");
			}
			break;
		case COMPAT_Turn_Off_Charging: /* For Turnoffcharging Mode*/
			ret = file->f_op->unlocked_ioctl(file, COMPAT_Turn_Off_Charging, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_Turn_Off_Charging is failed!\n");
			}
			break;
		case COMPAT_K_AT_CHG_CHGR_IN:
			ret = file->f_op->unlocked_ioctl(file, COMPAT_K_AT_CHG_CHGR_IN, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_K_AT_CHG_CHGR_IN is failed!\n");
			}
			break;
		
		case COMPAT_K_AT_CHG_CHGR_OFF:
			ret = file->f_op->unlocked_ioctl(file, COMPAT_K_AT_CHG_CHGR_OFF, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_K_AT_CHG_CHGR_OFF is failed!\n");
			}
			break;
		case COMPAT_K_AT_CHG_ON:
			ret = file->f_op->unlocked_ioctl(file, COMPAT_K_AT_CHG_ON, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_K_AT_CHG_ON is failed!\n");
			}
			break;
		case COMPAT_K_AT_CHG_OFF:
			ret = file->f_op->unlocked_ioctl(file, COMPAT_K_AT_CHG_OFF, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_K_AT_CHG_OFF is failed!\n");
			}
			break;
		case COMPAT_K_AT_CHG_INFO:
			ret = file->f_op->unlocked_ioctl(file, COMPAT_K_AT_CHG_INFO, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_K_AT_CHG_INFO is failed!\n");
			}
			break;
		case COMPAT_SET_SPI_CS_LOW:
			ret = file->f_op->unlocked_ioctl(file, COMPAT_SET_SPI_CS_LOW, (unsigned long)arg64);
			if(ret < 0)
			{
				printk(KERN_ERR "COMPAT_SET_SPI_CS_LOW is failed!\n");
			}
			break;
		default:
			g_ADC_Cali = KAL_FALSE;
			break;
    }
    
    return 0;
}
#endif   

static int adc_cali_open(struct inode *inode, struct file *file)
{ 
   return 0;
}

static int adc_cali_release(struct inode *inode, struct file *file)
{
    return 0;
}


static struct file_operations adc_cali_fops = {
    .owner        = THIS_MODULE,
    .unlocked_ioctl    = adc_cali_unlocked_ioctl,
    .open        = adc_cali_open,
    .release    = adc_cali_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = adc_cali_compat_ioctl,
#endif    
};


void check_battery_exist(void)
{
#if defined(CONFIG_DIS_CHECK_BATTERY)
    battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Disable check battery exist.\n");
#else
    kal_uint32 baton_count = 0;
	kal_uint32 charging_enable = KAL_FALSE;
	kal_uint32 battery_status;
	kal_uint32 i;

	for(i=0;i<3;i++)
	{
		battery_charging_control(CHARGING_CMD_GET_BATTERY_STATUS,&battery_status);
		baton_count += battery_status;

	}
       
    if( baton_count >= 3)
    {
        if( (g_platform_boot_mode==META_BOOT) || (g_platform_boot_mode==ADVMETA_BOOT) || (g_platform_boot_mode==ATE_FACTORY_BOOT) )
        {
            battery_xlog_printk(BAT_LOG_FULL, "[BATTERY] boot mode = %d, bypass battery check\n", g_platform_boot_mode);
        }
        else
        {
            battery_xlog_printk(BAT_LOG_CRTI, "[BATTERY] Battery is not exist, power off FAN5405 and system (%d)\n", baton_count);
            
			battery_charging_control(CHARGING_CMD_ENABLE,&charging_enable);
            battery_charging_control(CHARGING_CMD_SET_POWER_OFF,NULL);    
        }
    }    
#endif
}


int charger_hv_detect_sw_thread_handler(void *unused)
{
    ktime_t ktime;
	kal_uint32 charging_enable;
	kal_uint32 hv_voltage = V_CHARGER_MAX*1000;
	kal_bool hv_status;	


 #ifndef VENDOR_EDIT
     //rendong.shi@BSP.drv 2014/09/23 remove for boot 
    do
    {
        ktime = ktime_set(BAT_TASK_PERIOD, 0);     

		if(chargin_hw_init_done_bq24196)
			battery_charging_control(CHARGING_CMD_SET_HV_THRESHOLD,&hv_voltage);
            
        wait_event_interruptible(charger_hv_detect_waiter, (charger_hv_detect_flag == KAL_TRUE));
    
       	if ((pmic_chrdet_status() == KAL_TRUE))
        {
            check_battery_exist();
        }
		
	 	charger_hv_detect_flag = KAL_FALSE;

		if(chargin_hw_init_done_bq24196)
			battery_charging_control(CHARGING_CMD_GET_HV_STATUS,&hv_status);

		if(hv_status == KAL_TRUE)
        {
            battery_xlog_printk(BAT_LOG_CRTI, "[charger_hv_detect_sw_thread_handler] charger hv\n");    
            
			charging_enable = KAL_FALSE;
			if(chargin_hw_init_done_bq24196)
				//battery_charging_control(CHARGING_CMD_ENABLE,&charging_enable);
				KernelVendorVoteToStopCharging(CHG_CMD_ENABLE, CHG_STOP_VOTER__BATTTEMP_ABNORMAL);
        }
        else
        {
            battery_xlog_printk(BAT_LOG_FULL, "[charger_hv_detect_sw_thread_handler] upmu_chr_get_vcdt_hv_det() != 1\n");    
        }

		if(chargin_hw_init_done_bq24196)
			battery_charging_control(CHARGING_CMD_RESET_WATCH_DOG_TIMER,NULL);
       
        hrtimer_start(&charger_hv_detect_timer, ktime, HRTIMER_MODE_REL);    
        
    } while (!kthread_should_stop());
    
    return 0;
	#else
		return 0;
	#endif
}

enum hrtimer_restart charger_hv_detect_sw_workaround(struct hrtimer *timer)
{
    charger_hv_detect_flag = KAL_TRUE; 
    wake_up_interruptible(&charger_hv_detect_waiter);

    battery_xlog_printk(BAT_LOG_FULL, "[charger_hv_detect_sw_workaround] \n");
    
    return HRTIMER_NORESTART;
}

void charger_hv_detect_sw_workaround_init(void)
{
    ktime_t ktime;

    ktime = ktime_set(0, BAT_MS_TO_NS(2000));
    hrtimer_init(&charger_hv_detect_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    charger_hv_detect_timer.function = charger_hv_detect_sw_workaround;    
    hrtimer_start(&charger_hv_detect_timer, ktime, HRTIMER_MODE_REL);

    charger_hv_detect_thread = kthread_run(charger_hv_detect_sw_thread_handler, 0, "mtk charger_hv_detect_sw_workaround");
    if (IS_ERR(charger_hv_detect_thread))
    {
        battery_xlog_printk(BAT_LOG_FULL, "[%s]: failed to create charger_hv_detect_sw_workaround thread\n", __FUNCTION__);
    }

    battery_xlog_printk(BAT_LOG_CRTI, "charger_hv_detect_sw_workaround_init : done\n" );
}


enum hrtimer_restart battery_kthread_hrtimer_func(struct hrtimer *timer)
{
    bat_thread_wakeup(); 
    
    return HRTIMER_NORESTART;
}

void battery_kthread_hrtimer_init(void)
{
    ktime_t ktime;

    ktime = ktime_set(1, 0);	// 3s, 10* 1000 ms
    hrtimer_init(&battery_kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    battery_kthread_timer.function = battery_kthread_hrtimer_func;    
    hrtimer_start(&battery_kthread_timer, ktime, HRTIMER_MODE_REL);

    battery_xlog_printk(BAT_LOG_CRTI, "battery_kthread_hrtimer_init : done\n" );
}


static void get_charging_control(void)
{
	battery_charging_control = chr_control_interface;
}

void init_hw_version(void)
{
	kal_uint32 hw_id_0 = 99;
	kal_uint32 hw_id_1 = 99;
	kal_uint32 hw_id_2 = 99;
	kal_uint32 hw_id_3 = 99;
	hw_id_0 = mt_get_gpio_in(GPIO152) & 0x01;
	hw_id_1 = mt_get_gpio_in(GPIO153) & 0x01;
	hw_id_2 = mt_get_gpio_in(GPIO151) & 0x01;
	hw_id_3 = mt_get_gpio_in(GPIO138) & 0x01;
	printk("mt_get_gpio_in(GPIO152) = %d mt_get_gpio_in(GPIO153) = %d mt_get_gpio_in(GPIO151) = %d mt_get_gpio_in(GPIO138) = %d\r\n",mt_get_gpio_in(GPIO152),mt_get_gpio_in(GPIO153),mt_get_gpio_in(GPIO151),mt_get_gpio_in(GPIO138));
	if((hw_id_0 == 1) && (hw_id_1 == 1) && (hw_id_2 == 1) && (hw_id_3 == 1))
	{
		g_hw_version = HW_VERSION__EVT;
	}
	else if((hw_id_0 == 1) && (hw_id_1 == 1) && (hw_id_2 == 1) && (hw_id_3 == 0))
	{
		g_hw_version = HW_VERSION__DVT;
	}
	else if((hw_id_0 == 1) && (hw_id_1 == 0) && (hw_id_2 == 1) && (hw_id_3 == 1))
	{
		g_hw_version = HW_VERSION__MCU;
	}
	else if((hw_id_0 == 1) && (hw_id_1 == 1) && (hw_id_2 == 0) && (hw_id_3 == 1))
	{
		g_hw_version = HW_VERSION__15041;
	}
	else
	{
		g_hw_version = HW_VERSION__UNKNOWN;
	}
	
	if(g_hw_version == HW_VERSION__EVT)
	{
		gpio_oppo_vooc_sw_ctrl = OPPO_VOOC_SW_CTRL_EVT;
	}
	else
	{
		gpio_oppo_vooc_sw_ctrl = OPPO_VOOC_SW_CTRL_DVT;
	}
	printk("%s,g_hw_version = %d,gpio_oppo_vooc_sw_ctrl = %d\r\n",__FUNCTION__,g_hw_version,gpio_oppo_vooc_sw_ctrl);		

}

static int battery_probe(struct platform_device *dev)    
{
    struct class_device *class_dev = NULL;
    int ret=0;
	
    battery_xlog_printk(BAT_LOG_CRTI, "******** battery driver probe!! ********\n" );
    /* Integrate with NVRAM */
    ret = alloc_chrdev_region(&adc_cali_devno, 0, 1, ADC_CALI_DEVNAME);
    if (ret) 
       battery_xlog_printk(BAT_LOG_CRTI, "Error: Can't Get Major number for adc_cali \n");
    adc_cali_cdev = cdev_alloc();
    adc_cali_cdev->owner = THIS_MODULE;
    adc_cali_cdev->ops = &adc_cali_fops;
    ret = cdev_add(adc_cali_cdev, adc_cali_devno, 1);
    if(ret)
       battery_xlog_printk(BAT_LOG_CRTI, "adc_cali Error: cdev_add\n");
    adc_cali_major = MAJOR(adc_cali_devno);
    adc_cali_class = class_create(THIS_MODULE, ADC_CALI_DEVNAME);
    class_dev = (struct class_device *)device_create(adc_cali_class, 
                                                   NULL, 
                                                   adc_cali_devno, 
                                                   NULL, 
                                                   ADC_CALI_DEVNAME);
    battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] adc_cali prepare : done !!\n ");

	get_charging_control();

    battery_charging_control(CHARGING_CMD_GET_PLATFORM_BOOT_MODE, &g_platform_boot_mode);
    battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] g_platform_boot_mode = %d\n ", g_platform_boot_mode);

	wake_lock_init(&battery_suspend_lock, WAKE_LOCK_SUSPEND, "battery suspend wakelock");    
	#if defined(MTK_PUMP_EXPRESS_SUPPORT) || defined(MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	wake_lock_init(&TA_charger_suspend_lock, WAKE_LOCK_SUSPEND, "TA charger suspend wakelock");  
	#endif

    /* Integrate with Android Battery Service */
    ret = power_supply_register(&(dev->dev), &ac_main.psy);
    if (ret)
    {            
        battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] power_supply_register AC Fail !!\n");                    
        return ret;
    }             
    battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] power_supply_register AC Success !!\n");

    ret = power_supply_register(&(dev->dev), &usb_main.psy);
    if (ret)
    {            
        battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] power_supply_register USB Fail !!\n");                    
        return ret;
    }             
    battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] power_supply_register USB Success !!\n");
    
    ret = power_supply_register(&(dev->dev), &wireless_main.psy);
    if (ret)
    {            
        battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] power_supply_register WIRELESS Fail !!\n");                    
        return ret;
    }             
    battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] power_supply_register WIRELESS Success !!\n");
    
    ret = power_supply_register(&(dev->dev), &battery_main.psy);
    if (ret)
    {
        battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] power_supply_register Battery Fail !!\n");
        return ret;
    }
    battery_xlog_printk(BAT_LOG_CRTI, "[BAT_probe] power_supply_register Battery Success !!\n");

    /*Lenovo-sw begin yexh1 add 2013-6-4,add for lenovo charging*/ 
	#ifdef X2_CHARGING_STANDARD_SUPPORT
 	if (ret = lenovo_battery_create_sys_file(battery_main.psy.dev))
	{
		printk( "%s,failed: lenovo device_create_file \n", __func__);
		return ret;
	}
	#endif
    /*Lenovo-sw end yexh1  */
#if !defined(CONFIG_POWER_EXT)

#ifdef MTK_POWER_EXT_DETECT
	if (KAL_TRUE == bat_is_ext_power())
	{
		battery_main.BAT_STATUS = POWER_SUPPLY_STATUS_FULL;  
		battery_main.BAT_HEALTH = POWER_SUPPLY_HEALTH_GOOD;
		battery_main.BAT_PRESENT = 1;
		battery_main.BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION;
		battery_main.BAT_CAPACITY = 100;
		battery_main.BAT_batt_vol = 4200;
		battery_main.BAT_batt_temp = 220;

		g_bat_init_flag = KAL_TRUE;
    	return 0;
	}
#endif
    /* For EM */
	{
	    int ret_device_file=0;
		
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Charger_Voltage);
	    
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_0_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_1_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_2_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_3_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_4_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_5_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_6_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_7_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_8_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_9_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_10_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_11_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_12_Slope);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_13_Slope);

	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_0_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_1_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_2_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_3_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_4_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_5_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_6_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_7_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_8_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_9_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_10_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_11_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_12_Offset);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_13_Offset);

	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ADC_Channel_Is_Calibration);

	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_Power_On_Voltage);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_Power_Off_Voltage);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_Charger_TopOff_Value);
	    
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_FG_Battery_CurrentConsumption);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_FG_SW_CoulombCounter);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_Charging_CallState);
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_Charger_Type);
	    #if defined(MTK_PUMP_EXPRESS_SUPPORT) || defined(MTK_PUMP_EXPRESS_PLUS_SUPPORT)
	    ret_device_file = device_create_file(&(dev->dev), &dev_attr_Pump_Express);
	    #endif
	}
	
	//battery_meter_initial();	//move to mt_battery_GetBatteryData() to decrease booting time
	
	/* Initialization BMT Struct */
    BMT_status.bat_exist = KAL_TRUE;       /* phone must have battery */
    BMT_status.charger_exist = KAL_FALSE;     /* for default, no charger */
    BMT_status.bat_vol = 0;
    BMT_status.ICharging = 0;
    BMT_status.temperature = 0;
    BMT_status.charger_vol = 0;
    BMT_status.total_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CC_charging_time = 0;
    BMT_status.TOPOFF_charging_time = 0;
    BMT_status.POSTFULL_charging_time = 0;
	BMT_status.SOC = 0;
	BMT_status.UI_SOC = 0;

    BMT_status.bat_charging_state = CHR_PRE;
	BMT_status.bat_in_recharging_state = KAL_FALSE;
	BMT_status.bat_full= KAL_FALSE;

	BMT_status.nPercent_ZCV = 0;
	BMT_status.nPrecent_UI_SOC_check_point= battery_meter_get_battery_nPercent_UI_SOC();
	
	//init_hw_version();
    //battery kernel thread for 10s check and charger in/out event
    /* Replace GPT timer by hrtime */
    battery_kthread_hrtimer_init();
	
    kthread_run(bat_thread_kthread, NULL, "bat_thread_kthread"); 
    battery_xlog_printk(BAT_LOG_CRTI, "[battery_probe] bat_thread_kthread Done\n");    
    

   	charger_hv_detect_sw_workaround_init();
//	mt6325_upmu_set_rg_vcdt_lv_vth(0);
		
    /*LOG System Set*/
    init_proc_log();

#else
    //keep HW alive
    charger_hv_detect_sw_workaround_init();
#endif   
	g_bat_init_flag = KAL_TRUE;
	
    return 0;
	
}

static void battery_timer_pause(void)
{
	struct timespec xts, tom;

    //battery_xlog_printk(BAT_LOG_CRTI, "******** battery driver suspend!! ********\n" );
#ifdef CONFIG_POWER_EXT
#else

#ifdef MTK_POWER_EXT_DETECT
	if (KAL_TRUE == bat_is_ext_power())
		return 0;
#endif
	mutex_lock(&bat_mutex);
    //cancel timer
    hrtimer_cancel(&battery_kthread_timer);
    hrtimer_cancel(&charger_hv_detect_timer);
    
    battery_suspended = KAL_TRUE;
    mutex_unlock(&bat_mutex);

    battery_xlog_printk(BAT_LOG_CRTI, "@bs=1@\n" );
#endif

    get_xtime_and_monotonic_and_sleep_offset(&xts, &tom, &g_bat_time_before_sleep);
}

static void battery_timer_resume(void)
{
#ifdef CONFIG_POWER_EXT
#else
	kal_bool is_pcm_timer_trigger = KAL_FALSE;
	struct timespec xts, tom, bat_time_after_sleep;
    ktime_t ktime, hvtime;

#ifdef MTK_POWER_EXT_DETECT
	if (KAL_TRUE == bat_is_ext_power())
		return 0;
#endif

    ktime = ktime_set(BAT_TASK_PERIOD, 0);  // 10s, 10* 1000 ms
    //hvtime = ktime_set(0, BAT_MS_TO_NS(2000));
	hvtime = ktime_set(BAT_TASK_PERIOD, 0);

	get_xtime_and_monotonic_and_sleep_offset(&xts, &tom, &bat_time_after_sleep);
	battery_charging_control(CHARGING_CMD_GET_IS_PCM_TIMER_TRIGGER,&is_pcm_timer_trigger);

	if(is_pcm_timer_trigger == KAL_TRUE || bat_spm_timeout)
	{	
		mutex_lock(&bat_mutex);
		BAT_thread();
		mutex_unlock(&bat_mutex);
	}
	else
	{
		BMT_status.SOC = opchg_get_prop_batt_capacity();
		if ( BMT_status.SOC < bat_volt_check_point ) 
		{		
			if(g_soc_sync_time>=g_soc_sync_down_times)
			{
				g_soc_sync_time=0;
				bat_volt_check_point--;
			}
			else
			{
				g_soc_sync_time+=1;
			}
	}
		battery_xlog_printk(BAT_LOG_CRTI, "battery resume NOT by pcm timer,soc = %d,bat_volt_check_point = %d!!\n",BMT_status.SOC,bat_volt_check_point);
	}

	if(g_call_state == CALL_ACTIVE && (bat_time_after_sleep.tv_sec - g_bat_time_before_sleep.tv_sec >= TALKING_SYNC_TIME))	// phone call last than x min
	{
		BMT_status.UI_SOC = battery_meter_get_battery_percentage();
		battery_xlog_printk(BAT_LOG_CRTI, "Sync UI SOC to SOC immediately\n" );
	}	

    mutex_lock(&bat_mutex);
    
    //restore timer
    hrtimer_start(&battery_kthread_timer, ktime, HRTIMER_MODE_REL);
    hrtimer_start(&charger_hv_detect_timer, hvtime, HRTIMER_MODE_REL);
        
    battery_suspended = KAL_FALSE;
    battery_xlog_printk(BAT_LOG_CRTI, "@bs=0@\n");
    mutex_unlock(&bat_mutex);
	
#endif
}

static int battery_remove(struct platform_device *dev)    
{
    battery_xlog_printk(BAT_LOG_CRTI, "******** battery driver remove!! ********\n" );

    return 0;
}

static void battery_shutdown(struct platform_device *dev)    
{
    battery_xlog_printk(BAT_LOG_CRTI, "******** battery driver shutdown!! ********\n" );

}

///////////////////////////////////////////////////////////////////////////////////////////
//// Battery Notify API 
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_BatteryNotify(struct device *dev,struct device_attribute *attr, char *buf)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[Battery] show_BatteryNotify : %x\n", g_BatteryNotifyCode);
    
    return sprintf(buf, "%u\n", g_BatteryNotifyCode);
}
static ssize_t store_BatteryNotify(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    char *pvalue = NULL;
    unsigned int reg_BatteryNotifyCode = 0;
    battery_xlog_printk(BAT_LOG_CRTI, "[Battery] store_BatteryNotify\n");
    if(buf != NULL && size != 0)
    {
        battery_xlog_printk(BAT_LOG_CRTI, "[Battery] buf is %s and size is %zd \n",buf,size);
        reg_BatteryNotifyCode = simple_strtoul(buf,&pvalue,16);
        g_BatteryNotifyCode = reg_BatteryNotifyCode;
        battery_xlog_printk(BAT_LOG_CRTI, "[Battery] store code : %x \n",g_BatteryNotifyCode);        
    }        
    return size;
}
static DEVICE_ATTR(BatteryNotify, 0664, show_BatteryNotify, store_BatteryNotify);

static ssize_t show_BN_TestMode(struct device *dev,struct device_attribute *attr, char *buf)
{
    battery_xlog_printk(BAT_LOG_CRTI, "[Battery] show_BN_TestMode : %x\n", g_BN_TestMode);
    return sprintf(buf, "%u\n", g_BN_TestMode);
}
static ssize_t store_BN_TestMode(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    char *pvalue = NULL;
    unsigned int reg_BN_TestMode = 0;
    battery_xlog_printk(BAT_LOG_CRTI, "[Battery] store_BN_TestMode\n");
    if(buf != NULL && size != 0)
    {
        battery_xlog_printk(BAT_LOG_CRTI, "[Battery] buf is %s and size is %zd \n",buf,size);
        reg_BN_TestMode = simple_strtoul(buf,&pvalue,16);
        g_BN_TestMode = reg_BN_TestMode;
        battery_xlog_printk(BAT_LOG_CRTI, "[Battery] store g_BN_TestMode : %x \n",g_BN_TestMode);        
    }        
    return size;
}
static DEVICE_ATTR(BN_TestMode, 0664, show_BN_TestMode, store_BN_TestMode);

static ssize_t show_ID_status(struct device *dev,struct device_attribute *attr, char *buf)
{
	if (Enable_BATDRV_LOG == 1) 
    {
	    xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] show_ID_status : %x\n", oppo_high_battery_status);
	}
	return sprintf(buf, "%u\n", oppo_high_battery_status);
}
static ssize_t store_ID_status(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	char *pvalue = NULL;
	int reg_ID_status = 0;
    
	printk("[Battery] store_ID_status\n");
	if(buf != NULL && size != 0)
	{
		xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] buf is %s and size is %d \n",buf,size);
		reg_ID_status = simple_strtoul(buf,&pvalue,16);
		oppo_high_battery_status = reg_ID_status;
		xlog_printk(ANDROID_LOG_INFO, "Power/Battery", "[Battery] store_ID_status : %x \n",oppo_high_battery_status);
	}		
	return size;
}
static DEVICE_ATTR(ID_status, 0664, show_ID_status, store_ID_status);


///////////////////////////////////////////////////////////////////////////////////////////
//// platform_driver API 
///////////////////////////////////////////////////////////////////////////////////////////
#if 0
static int battery_cmd_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    p += sprintf(p, "g_battery_thermal_throttling_flag=%d,\nbattery_cmd_thermal_test_mode=%d,\nbattery_cmd_thermal_test_mode_value=%d\n", 
        g_battery_thermal_throttling_flag, battery_cmd_thermal_test_mode, battery_cmd_thermal_test_mode_value);
    
    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;
    
    return len < count ? len  : count;
}
#endif

static ssize_t battery_cmd_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
    int len = 0, bat_tt_enable=0, bat_thr_test_mode=0, bat_thr_test_value=0;
    char desc[32];
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    if (sscanf(desc, "%d %d %d", &bat_tt_enable, &bat_thr_test_mode, &bat_thr_test_value) == 3)
    {
        g_battery_thermal_throttling_flag = bat_tt_enable;
        battery_cmd_thermal_test_mode = bat_thr_test_mode;
        battery_cmd_thermal_test_mode_value = bat_thr_test_value;
        
        battery_xlog_printk(BAT_LOG_CRTI, "bat_tt_enable=%d, bat_thr_test_mode=%d, bat_thr_test_value=%d\n", 
            g_battery_thermal_throttling_flag, battery_cmd_thermal_test_mode, battery_cmd_thermal_test_mode_value);
        
        return count;
    }
    else
    {
        battery_xlog_printk(BAT_LOG_CRTI, "  bad argument, echo [bat_tt_enable] [bat_thr_test_mode] [bat_thr_test_value] > battery_cmd\n");
    }
    
    return -EINVAL;
}

static int proc_utilization_show(struct seq_file *m, void *v)
{
    seq_printf(m, "=> g_battery_thermal_throttling_flag=%d,\nbattery_cmd_thermal_test_mode=%d,\nbattery_cmd_thermal_test_mode_value=%d\n", 
        g_battery_thermal_throttling_flag, battery_cmd_thermal_test_mode, battery_cmd_thermal_test_mode_value);
    
    //seq_printf(m, "=> get_usb_current_unlimited=%d, \ncmd_discharging = %d\n", get_usb_current_unlimited(), cmd_discharging);
            
    return 0;
}

static int proc_utilization_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_utilization_show, NULL);
}

static const struct file_operations battery_cmd_proc_fops = { 
    .open  = proc_utilization_open, 
    .read  = seq_read,
    .write = battery_cmd_write,
};

static ssize_t current_cmd_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
    int len = 0;
    char desc[32];
    int cmd_current_unlimited = false;
    U32 charging_enable = false;
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    if (sscanf(desc, "%d %d", &cmd_current_unlimited, &cmd_discharging) == 2) {
        //set_usb_current_unlimited(cmd_current_unlimited);
        if (cmd_discharging == 1) {
            charging_enable = false;
            adjust_power = -1;
        } else if (cmd_discharging == 0){
            charging_enable = true;
            adjust_power = -1;
        }
        battery_charging_control(CHARGING_CMD_ENABLE,&charging_enable);
        
        battery_xlog_printk(BAT_LOG_CRTI, "[current_cmd_write] cmd_current_unlimited=%d, cmd_discharging=%d\n", cmd_current_unlimited, cmd_discharging);        
        return count;
    } else {
        battery_xlog_printk(BAT_LOG_CRTI, "  bad argument, echo [enable] > current_cmd\n");
    }
    
    return -EINVAL;
}

static const struct file_operations current_cmd_proc_fops = { 
    .open  = proc_utilization_open, 
    .read  = seq_read,
    .write = current_cmd_write,
};

static ssize_t discharging_cmd_write(struct file *file, const char *buffer, size_t count, loff_t *data)
{
    int len = 0;
    char desc[32];
    U32 charging_enable = false;
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    if (sscanf(desc, "%d %d", &charging_enable, &adjust_power) == 2) {
        
        battery_xlog_printk(BAT_LOG_CRTI, "[current_cmd_write] adjust_power = %d\n", adjust_power);
        return count;
    } else {
        battery_xlog_printk(BAT_LOG_CRTI, "  bad argument, echo [enable] > current_cmd\n");
    }
    
    return -EINVAL;
}

static const struct file_operations discharging_cmd_proc_fops = { 
    .open  = proc_utilization_open, 
    .read  = seq_read,
    .write = discharging_cmd_write,
};

static int mt_batteryNotify_probe(struct platform_device *dev)    
{
    int ret_device_file = 0;
    //struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *battery_dir = NULL;

    battery_xlog_printk(BAT_LOG_CRTI, "******** mt_batteryNotify_probe!! ********\n" );


    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BatteryNotify);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BN_TestMode);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_ID_status);
	
    battery_dir = proc_mkdir("mtk_battery_cmd", NULL);
    if (!battery_dir)
    {
        pr_err("[%s]: mkdir /proc/mtk_battery_cmd failed\n", __FUNCTION__);
    }
    else
    {
        #if 1
        proc_create("battery_cmd", S_IRUGO | S_IWUSR, battery_dir, &battery_cmd_proc_fops);
        battery_xlog_printk(BAT_LOG_CRTI, "proc_create battery_cmd_proc_fops\n");
        
                proc_create("current_cmd", S_IRUGO | S_IWUSR, battery_dir, &current_cmd_proc_fops);
                battery_xlog_printk(BAT_LOG_CRTI, "proc_create current_cmd_proc_fops\n");
                proc_create("discharging_cmd", S_IRUGO | S_IWUSR, battery_dir, &discharging_cmd_proc_fops);
                battery_xlog_printk(BAT_LOG_CRTI, "proc_create discharging_cmd_proc_fops\n");
            
        #else
        entry = create_proc_entry("battery_cmd", S_IRUGO | S_IWUSR, battery_dir);
        if (entry)
        {
            entry->read_proc = battery_cmd_read;
            entry->write_proc = battery_cmd_write;
        }
        #endif
    }

    battery_xlog_printk(BAT_LOG_CRTI, "******** mtk_battery_cmd!! ********\n" );    
		
    return 0;

}


#if 0//#ifdef CONFIG_OF
static const struct of_device_id mt_battery_of_match[] = {
	{ .compatible = "mediatek,battery", },
	{},
};

MODULE_DEVICE_TABLE(of, mt_battery_of_match);
#endif

static int battery_pm_suspend(struct device *device)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(device);
	BUG_ON(pdev == NULL);

	return ret;
}

static int battery_pm_resume(struct device *device)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(device);
	BUG_ON(pdev == NULL);

	return ret;
}

static int battery_pm_freeze(struct device *device)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(device);
	BUG_ON(pdev == NULL);

	return ret;
}

static int battery_pm_restore(struct device *device)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(device);
	BUG_ON(pdev == NULL);

	return ret;
}

static int battery_pm_restore_noirq(struct device *device)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(device);
	BUG_ON(pdev == NULL);

	return ret;
}

struct dev_pm_ops battery_pm_ops = {
    .suspend = battery_pm_suspend,
    .resume = battery_pm_resume,
    .freeze = battery_pm_freeze,
    .thaw = battery_pm_restore,
    .restore = battery_pm_restore,
    .restore_noirq = battery_pm_restore_noirq,
};

static struct platform_driver battery_driver = {
    .probe         = battery_probe,
    .remove        = battery_remove,
    .shutdown      = battery_shutdown,
    .driver        = {
        .name = "battery",
        #if 0//#ifdef CONFIG_OF 
        .of_match_table = mt_battery_of_match,
        #endif
		.pm = &battery_pm_ops,
    },
};

//--------------------------------------------------------

#if 0//#ifdef CONFIG_OF
static const struct of_device_id mt_bat_notify_of_match[] = {
	{ .compatible = "mediatek,bat_notify", },
	{},
};

MODULE_DEVICE_TABLE(of, mt_bat_notify_of_match);
#else
#ifdef BATTERY_MODULE_INIT
struct platform_device battery_device = {
    .name   = "battery",
    .id        = -1,
};
#endif
struct platform_device MT_batteryNotify_device = {
    .name   = "mt-battery",
    .id        = -1,
};
#endif

static struct platform_driver mt_batteryNotify_driver = {
    .probe        = mt_batteryNotify_probe,
    .driver       = {
        .name = "mt-battery",
        #if 0//#ifdef CONFIG_OF
        .of_match_table = mt_bat_notify_of_match,    
        #endif
    },
};

//--------------------------------------------------------

static int battery_pm_event(struct notifier_block *notifier, unsigned long pm_event, void *unused)
{
    switch(pm_event) {
	case PM_HIBERNATION_PREPARE: /* Going to hibernate */
	case PM_RESTORE_PREPARE: /* Going to restore a saved image */
	case PM_SUSPEND_PREPARE: /* Going to suspend the system */
		pr_warn("[%s] pm_event %lu\n", __func__, pm_event);
		battery_timer_pause();
		return NOTIFY_DONE;
	case PM_POST_HIBERNATION: /* Hibernation finished */
	case PM_POST_SUSPEND: /* Suspend finished */
	case PM_POST_RESTORE: /* Restore failed */
		pr_warn("[%s] pm_event %lu\n", __func__, pm_event);
		battery_timer_resume();
		return NOTIFY_DONE;
    }
    return NOTIFY_OK;
}

static struct notifier_block battery_pm_notifier_block = {
    .notifier_call = battery_pm_event,
    .priority = 0,
};

static int __init battery_init(void)
{
    int ret;

    printk("battery_init\n");

    #if 0//#ifdef CONFIG_OF
    //
    #else
    
#ifdef BATTERY_MODULE_INIT
    ret = platform_device_register(&battery_device);    
#endif

    ret = platform_driver_register(&battery_driver);
    if (ret) {
        battery_xlog_printk(BAT_LOG_CRTI, "****[battery_driver] Unable to device register (%d)\n", ret);
        return ret;
    }   
    #endif   
    
    // battery notofy UI
    #if 0//#ifdef CONFIG_OF
    //
    #else        
    ret = platform_device_register(&MT_batteryNotify_device);
    if (ret) {
        battery_xlog_printk(BAT_LOG_CRTI, "****[mt_batteryNotify] Unable to device register(%d)\n", ret);
        return ret;
    }
    #endif    
    ret = platform_driver_register(&mt_batteryNotify_driver);
    if (ret) {
        battery_xlog_printk(BAT_LOG_CRTI, "****[mt_batteryNotify] Unable to register driver (%d)\n", ret);
        return ret;
    }

    ret = register_pm_notifier(&battery_pm_notifier_block);
    if (ret)
        printk("[%s] failed to register PM notifier %d\n", __func__, ret);

    battery_xlog_printk(BAT_LOG_CRTI, "****[battery_driver] Initialization : DONE !!\n");
    return 0;
}
#ifdef BATTERY_MODULE_INIT
late_initcall(battery_init);
#else
static void __exit battery_exit (void)
{
}

module_init(battery_init);
module_exit(battery_exit);
#endif

MODULE_AUTHOR("Oscar Liu");
MODULE_DESCRIPTION("Battery Device Driver");
MODULE_LICENSE("GPL");

