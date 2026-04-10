typedef enum
{
	Cust_CC_1600MA = 0x0,
	Cust_CC_1500MA = 0x1,
	Cust_CC_1400MA = 0x2,
	Cust_CC_1300MA = 0x3,
	Cust_CC_1200MA = 0x4,
	Cust_CC_1100MA = 0x5,
	Cust_CC_1000MA = 0x6,
	Cust_CC_900MA  = 0x7,
	Cust_CC_800MA  = 0x8,
	Cust_CC_700MA  = 0x9,
	Cust_CC_650MA  = 0xA,
	Cust_CC_550MA  = 0xB,
	Cust_CC_450MA  = 0xC,
	Cust_CC_400MA  = 0xD,
	Cust_CC_200MA  = 0xE,
	Cust_CC_70MA   = 0xF,
	Cust_CC_0MA	   = 0xDD
}cust_charging_current_enum;

typedef struct{	
	unsigned int BattVolt;
	unsigned int BattPercent;
}VBAT_TO_PERCENT;


/*Fanhong.Kong@ProDrv.CHG,modify 2012/7/26 for 12015 3000ma */
/* Battery Voltage and Percentage Mapping Table */
VBAT_TO_PERCENT Batt_VoltToPercent_Table[] = {
	/*BattVolt,BattPercent*/
	{3400,0},
	{3514,1},
	{3616,5},
	{3627,9},
	{3662,13},
	{3688,17},
	{3699,21},
	{3708,25},
	{3716,29},
	{3725,33},
	{3739,37},
	{3752,41},
	{3766,45},
	{3784,49},
	{3803,53},
	{3831,57},
	{3855,61},
	{3878,65},
	{3906,69},
	{3956,73},
	{3992,77},
	{4025,81},
	{4063,85},
	{4102,89},
	{4160,93},
	{4229,97},
	{4236,98},
	{4244,99},
	{4252,100},
};

#define MAX_CHARGE_TEMPERATURE    56//55
#define MIN_CHARGE_TEMPERATURE   -10
#define BAT_NOT_CONNECT          -20
#define PREHOT_CHARGE_TEMPERATURE 45
#define PRECOLD_CHARGE_TEMPERATURE 10
#define PRECOLD_CHARGE_TEMPERATURE2   0    // dengzy@oppo.com 2012/03/09 add,



/* Recharging Battery Voltage */
//#define RECHARGING_VOLTAGE      4110
#define RECHARGING_VOLTAGE      4200
#define OPPO_BAT_VOLT_100		4250

/* Charging Current Setting */
#define CONFIG_USB_IF 						0   
#define USB_CHARGER_CURRENT_SUSPEND			Cust_CC_0MA		// def CONFIG_USB_IF
#define USB_CHARGER_CURRENT_UNCONFIGURED	Cust_CC_200MA	// def CONFIG_USB_IF
#define USB_CHARGER_CURRENT_CONFIGURED		Cust_CC_450MA	// def CONFIG_USB_IF

/*charging current*/ // by dengzy@oppo.com
#define USB_CHARGER_PRECURRENT				Cust_CC_400MA
#define USB_CHARGER_PROCURRENT              Cust_CC_450MA//changing from 425ma to 450ma to 500ma because of CS_VTH 
#define AC_CHARGER_CCXCURRENT				Cust_CC_400MA//Cust_CC_300MA
#define AC_CHARGER_CC2CURRENT				Cust_CC_550MA

#define TEMP_CC1_CHARGER_CURRENT                Cust_CC_400MA
#define TEMP_CC2_CHARGER_CURRENT                Cust_CC_200MA // Cust_CC_200MA  modify for uew hw chging rules
#define TOPOFF_CHARGER_CURRENT              Cust_CC_200MA

/* Battery Meter Solution */
#define CONFIG_ADC_SOLUTION 	1


/* Precise Tunning */
//#define BATTERY_AVERAGE_SIZE 	30
#define BATTERY_AVERAGE_SIZE   30 //12

/* Common setting */
#define R_CURRENT_SENSE 2				// 0.2 Ohm
#define R_BAT_SENSE 4					// times of voltage
#define R_I_SENSE 4						// times of voltage
#define R_CHARGER_1 330
#define R_CHARGER_2 39
#define R_CHARGER_SENSE ((R_CHARGER_1+R_CHARGER_2)/R_CHARGER_2)	// times of voltage

#define V_CHARGER_MAX 5900				// 6 V
#define V_CHARGER_MIN 4300				// 4.4 V

#define V_CHARGER_ENABLE 0				// 1:ON , 0:OFF

#define FEATURE_BAT_TEMP_PROTECT		
#define FEATURE_VCHG_PROTECT
#define FEATURE_VBAT_PROTECT
#ifndef OPPO_CMCC_TEST//Fanhong.Kong@ProDrv.CHG, deleted 2012/11/26 for MTBF
#define FEATURE_MAX_CHGING_TIME_PROTECT
#endif /*VENDOR_EDIT*/

#ifdef OPPO_CMCC_TEST//PengNan@OPPO.com for limit input current 2015/04/04
#define CMCC_INPUT_CURRENT_LIMIT_PROTECT
#define CMCC_CHARGER_FULL_4250MV							4250
#define CMCC_CHARGER_FULL_4208MV							4208
#endif /*VENDOR_EDIT*/


#define FEATURE_PRINT_CHGR_LOG
#define FEATURE_PRINT_BAT_LOG
#define FEATURE_PRINT_STATUS_LOG
#define FEATURE_PRINT_TIME_LOG
#define FEATURE_PRINT_VOTE_LOG
#define FEATURE_PRINT_INTERMITTENT_LOG
#define FEATURE_PRINT_ICHGING_LOG
#define FEATURE_PRINT_OTHER_LOG
#define	FEATURE_PRINT_FASTCHG_LOG

#define BATTERY_NOTIFY_CASE_0001
#define BATTERY_NOTIFY_CASE_0002
#define BATTERY_NOTIFY_CASE_0003
#define BATTERY_NOTIFY_CASE_0004
#define BATTERY_NOTIFY_CASE_0005
#define BATTERY_NOTIFY_FLAG

#define D_BAT_THREAD_CB_TIME			(5000)

//60 secs 
#define D_PRINT_LOG_LOOP_TIME			(60) 

#define D_MAX_I_CHGING					(1000)



//#define RBAT_PULL_UP_VOLT          		(1800)
//#define TBAT_OVER_CRITICAL_LOW     		(68237)
//#define TBAT_PARALLEL_R					(24000)
#define BAT_TEMP_PROTECT_ENABLE    		(0)
//#define BAT_NTC_10 						(1)
//#define BAT_NTC_47 						(0)
#define D_BATTEMP_TBL_25_DEGREE_INDX	(13)
#define D_BATTEMP_TBL_FIRST_INDX		(0)
#define D_BATTEMP_TBL_LAST_INDX			(26)

#define HYSTERISIS_DECIDEGC       		(2)

#define D_BAT_TEMP_HIGH					(53)
#define D_BAT_TEMP_PRE_HIGH				(45)

#define D_BAT_TEMP_PRE_LOW_16			(16)
#define D_BAT_TEMP_PRE_LOW_12           (12)
#define D_BAT_TEMP_PRE_LOW_5           (5)

#define D_BAT_TEMP_PRE_LOW_0            (0)
#define D_BAT_TEMP_LOW					(-3)
#define D_BAT_REMOVED_TEMP				(-17)

#define D_VCHG_LOW						(3500)
#define D_VCHG_PRE_LOW					(3600)
#define D_VCHG_PRE_HIGH					(5800)
#define D_VCHG_HIGH						(6400)

#define D_VBAT_TOO_HIGH_VOL				(4500)
//#define D_VBAT_FULL_VOL					(4300)
#define D_VBAT_FULL_VOL					(4400)
#define D_VBAT_FULL_CHK_TIMES			(3)
#define D_VBAT_FULL_CHK_MASK			((1 << D_VBAT_FULL_CHK_TIMES) - 1)
#define D_VBAT_FULL						(D_VBAT_FULL_CHK_MASK)

#define D_CHGING_FULL_CNT				(20)

#define D_RECHGING_CNT					(5)

#define D_NORMAL_TEMP_FULL_VOL			(4200)
#define D_ABNORMAL_TEMP_FULL_VOL		(4100)
#define D_ABNORNAL_TEMP_FULL_VOL2       (4000)



#define D_NORMAL_TEMP_RECHGING_VOL				(4220)	//from 4250 for new standard 2015/03/18
#define D_PRE_WARM_TEMP_RECHGING_VOL			(4000)	
#define D_PRE_COLD_TEMP_RECHGING_VOL_NEG3_0  	(3700)	//from 3750 for new standard 2015/03/18
//modified by PengNan for the NEW_CHARGER_STARDARD_V2_8 2015.12.3
//#define D_PRE_COLD_TEMP_RECHGING_VOL_0_12		(4100)
#define D_PRE_COLD_TEMP_RECHGING_VOL_5_12		(4220)
#define D_PRE_COLD_TEMP_RECHGING_VOL_0_5		(4100)

#define D_PRE_COLD_TEMP_RECHGING_VOL_12_16		(4220)	//from 4250 for new standard 2015/03/18
#define D_THIRD_BAT_RECHGING_VOL				(3900)	



#define D_MAX_CC3_CHARGING_TIME			(3*60*60)
#define D_CHGING_FULL_CHK_TIMES			(D_CHGING_FULL_CNT)
#define D_CHGING_FULL_CHK_MASK			((1 << D_CHGING_FULL_CHK_TIMES) - 1)
#define D_CHGING_FULL					(D_CHGING_FULL_CHK_MASK)
#define D_RECHGING_CHK_TIMES			(D_RECHGING_CNT)
#define D_RECHGING_CHK_MASK				((1 << D_RECHGING_CHK_TIMES) - 1)
#define D_RECHGING						(D_RECHGING_CHK_MASK)

#define ABNORMAL_TEMP_TOPOFF_VOL2		4000
#define ABNORMAL_TEMP_TOPOFF_VOL		4100
#define NORMAL_TEMP_TOPOFF_VOL          4200


typedef enum
{
	CHG_CMD_ENABLE,
	CHG_CMD_DISABLE,
	CHG_CMD_MAX
}TChgCmd;

typedef enum
{
	CHG_STOP_VOTER__INTERMITTENT_CHGING		=	(1 << 0),
	CHG_STOP_VOTER__VCHG_ABNORMAL			=	(1 << 1),
	CHG_STOP_VOTER__BATTTEMP_ABNORMAL		= 	(1 << 2),
	CHG_STOP_VOTER__VBAT_TOO_HIGH			=	(1 << 3),
	CHG_STOP_VOTER__MAX_CHGING_TIME			=	(1 << 4),
	CHG_STOP_VOTER__MAX_CC3_CHGING_TIME		=	(1 << 5),
	CHG_STOP_VOTER__FULL					=	(1 << 6),

	CHG_STOP_VOTER_OVER_CURRENT				=	(1 << 7),

	CHG_STOP_VOTER_MAX_TOPOFF_TIME			=  	(1 << 9),
	CHG_STOP_VOTER_CURRENT_0MA				=	(1<<10)
}TChgStopVoter;

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

typedef enum
{
	CHARGER_STATUS__GOOD,
	CHARGER_STATUS__VOL_HIGH,
	CHARGER_STATUS__VOL_LOW,
	CHARGER_STATUS__INVALID
}TChgrStatus;

typedef struct
{
	kal_bool					bChgingOn;
	kal_bool					bOverBatVol;
	UINT32 						nIntermittentOnTime;
	UINT32						nIntermittentOffTime;
	UINT32						nChgStopVoterMask;
	UINT32						nCC1ModeChgingTime;
	UINT32						nCC2ModeChgingTime;
	UINT32						nCC3ModeChgingTime;
	cust_charging_current_enum	enChgingCurSet;
	TBatStatus					enBatStatus;
	TChgrStatus					enChgrStatus;

	UINT32 						topoff_mode_chging_time;
}TExternalBMT;

typedef struct
{
	short 	mBatteryTempBound_LOW;
	short 	mBatteryTempBound_PRE_LOW_0;
	short 	mBatteryTempBound_PRE_LOW_5;
	short 	mBatteryTempBound_PRE_LOW_12;
	short 	mBatteryTempBound_LOW_16;
	short 	mBatteryTempBound_PRE_HIGH;
	short 	mBatteryTempBound_HIGH;
}TBatteryTempBound;

