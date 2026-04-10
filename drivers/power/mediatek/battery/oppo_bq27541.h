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

#define OPPO_USE_FAST_CHARGER

#define OPPO_VOOC_RESET_MCU_EN		131//129
#define OPPO_VOOC_MCU_AP_CLK		129//131
#define OPPO_VOOC_MCU_AP_DATA		132//147
#define OPPO_VOOC_SW_CTRL_EVT		119//84
#define OPPO_VOOC_SW_CTRL_DVT		13//84
#define CUST_EINT_MCU_AP_DATA 		132//37

#define DRIVER_VERSION			"1.1.0"
#ifdef VENDOR_EDIT
#define CONFIG_GAUGE_BQ27411		1
#endif

/* Bq27541 standard data commands */
#define BQ27541_REG_CNTL		0x00
#define BQ27541_REG_AR			0x02
#define BQ27541_REG_ARTTE		0x04
#define BQ27541_REG_TEMP		0x06
#define BQ27541_REG_VOLT		0x08
#define BQ27541_REG_FLAGS		0x0A
#define BQ27541_REG_NAC			0x0C
#define BQ27541_REG_FAC			0x0e
#define BQ27541_REG_RM			0x10
#define BQ27541_REG_FCC			0x12
#define BQ27541_REG_AI			0x14
#define BQ27541_REG_TTE			0x16
#define BQ27541_REG_TTF			0x18
#define BQ27541_REG_SI			0x1a
#define BQ27541_REG_STTE		0x1c
#define BQ27541_REG_MLI			0x1e
#define BQ27541_REG_MLTTE		0x20
#define BQ27541_REG_AE			0x22
#define BQ27541_REG_AP			0x24
#define BQ27541_REG_TTECP		0x26

#define BQ27541_REG_INTTEMP		0x28
#define BQ27541_REG_CC			0x2a

#define BQ27541_REG_SOH			0x28
#define BQ27541_REG_SOC			0x2c
#define BQ27541_REG_NIC			0x2e
#define BQ27541_REG_ICR			0x30
#define BQ27541_REG_LOGIDX		0x32
#define BQ27541_REG_LOGBUF		0x34
#define BQ27541_REG_DOD0		0x36


#define BQ27541_FLAG_DSC		BIT(0)
#define BQ27541_FLAG_FC			BIT(9)

#define BQ27541_CS_DLOGEN		BIT(15)
#define BQ27541_CS_SS		    BIT(13)

/* Control subcommands */
#define BQ27541_SUBCMD_CTNL_STATUS  0x0000
#define BQ27541_SUBCMD_DEVCIE_TYPE  0x0001
#define BQ27541_SUBCMD_FW_VER  0x0002
#define BQ27541_SUBCMD_HW_VER  0x0003
#define BQ27541_SUBCMD_DF_CSUM  0x0004
#define BQ27541_SUBCMD_PREV_MACW   0x0007
#define BQ27541_SUBCMD_CHEM_ID   0x0008
#define BQ27541_SUBCMD_BD_OFFSET   0x0009
#define BQ27541_SUBCMD_INT_OFFSET  0x000a
#define BQ27541_SUBCMD_CC_VER   0x000b
#define BQ27541_SUBCMD_OCV  0x000c
#define BQ27541_SUBCMD_BAT_INS   0x000d
#define BQ27541_SUBCMD_BAT_REM   0x000e
#define BQ27541_SUBCMD_SET_HIB   0x0011
#define BQ27541_SUBCMD_CLR_HIB   0x0012
#define BQ27541_SUBCMD_SET_SLP   0x0013
#define BQ27541_SUBCMD_CLR_SLP   0x0014
#define BQ27541_SUBCMD_FCT_RES   0x0015
#define BQ27541_SUBCMD_ENABLE_DLOG  0x0018
#define BQ27541_SUBCMD_DISABLE_DLOG 0x0019
#define BQ27541_SUBCMD_SEALED   0x0020
#define BQ27541_SUBCMD_ENABLE_IT    0x0021
#define BQ27541_SUBCMD_DISABLE_IT   0x0023
#define BQ27541_SUBCMD_CAL_MODE  0x0040
#define BQ27541_SUBCMD_RESET   0x0041
#define ZERO_DEGREE_CELSIUS_IN_TENTH_KELVIN   (-2731)
#define BQ27541_INIT_DELAY   ((HZ)*1)

#ifdef CONFIG_GAUGE_BQ27411
/* Bq27411 standard data commands */
#define BQ27411_REG_CNTL				0x00
#define BQ27411_REG_TEMP				0x02
#define BQ27411_REG_VOLT				0x04
#define BQ27411_REG_FLAGS				0x06
#define BQ27411_REG_NAC					0x08
#define BQ27411_REG_FAC					0x0a
#define BQ27411_REG_RM					0x0c
#define BQ27411_REG_FCC					0x0e
#define BQ27411_REG_AI					0x10
#define BQ27411_REG_SI					0x12
#define BQ27411_REG_MLI					0x14
#define BQ27411_REG_AP					0x18
#define BQ27411_REG_SOC					0x1c
#define BQ27411_REG_INTTEMP				0x1e
#define BQ27411_REG_SOH					0x20

#define BQ27411_FLAG_DSC				BIT(0)
#define BQ27411_FLAG_FC					BIT(9)

#define BQ27411_CS_DLOGEN				BIT(15)
#define BQ27411_CS_SS		    		BIT(13)

/* Bq27411 sub commands */
#define BQ27411_SUBCMD_CNTL_STATUS  			0x0000
#define BQ27411_SUBCMD_DEVICE_TYPE  			0x0001
#define BQ27411_SUBCMD_FW_VER  					0x0002
#define BQ27411_SUBCMD_DM_CODE  				0x0004
#define BQ27411_SUBCMD_PREV_MACW   				0x0007
#define BQ27411_SUBCMD_CHEM_ID   				0x0008
#define BQ27411_SUBCMD_SET_HIB   				0x0011
#define BQ27411_SUBCMD_CLR_HIB   				0x0012
#define BQ27411_SUBCMD_SET_CFG	   				0x0013
#define BQ27411_SUBCMD_SEALED   				0x0020
#define BQ27411_SUBCMD_RESET   					0x0041
#define BQ27411_SUBCMD_SOFTRESET				0x0042
#define BQ27411_SUBCMD_EXIT_CFG					0x0043

#define BQ27411_SUBCMD_ENABLE_DLOG  			0x0018
#define BQ27411_SUBCMD_DISABLE_DLOG 			0x0019
#define BQ27411_SUBCMD_ENABLE_IT    			0x0021
#define BQ27411_SUBCMD_DISABLE_IT   			0x0023

#define BQ27541_BQ27411_CMD_INVALID				0xFF
#endif

#define BQ27541_BQ27411_REG_CNTL				0x00
#define BQ27541_BQ27411_CS_DLOGEN				BIT(15)
#define BQ27541_BQ27411_CS_SS		    		BIT(13)
#define BQ27541_BQ27411_SUBCMD_CTNL_STATUS		0x0000
#define BQ27541_BQ27411_SUBCMD_ENABLE_IT		0x0021
#define BQ27541_BQ27411_SUBCMD_ENABLE_DLOG		0x0018
#define BQ27541_BQ27411_SUBCMD_DEVICE_TYPE		0x0001
#define BQ27541_BQ27411_SUBCMD_FW_VER			0x0002
#define BQ27541_BQ27411_SUBCMD_DISABLE_DLOG		0x0019
#define BQ27541_BQ27411_SUBCMD_DISABLE_IT		0x0023


#define CAPACITY_SALTATE_COUNTER 4
#define CAPACITY_SALTATE_COUNTER_NOT_CHARGING	20
#define CAPACITY_SALTATE_COUNTER_80		30
#define CAPACITY_SALTATE_COUNTER_90		40
#define CAPACITY_SALTATE_COUNTER_95		60
#define CAPACITY_SALTATE_COUNTER_FULL		120

struct qpnp_battery_gauge {
	int (*get_battery_mvolts) (void);
	int (*get_battery_temperature) (void);
	int (*is_battery_present) (void);
	int (*is_battery_temp_within_range) (void);
	int (*is_battery_id_valid) (void);
	int (*get_battery_status)(void);
	int (*get_batt_remaining_capacity) (void);
	int (*monitor_for_recharging) (void);
	int (*get_battery_soc) (void);
	int (*get_average_current) (void);
	int (*is_battery_authenticated) (void);//wangjc add for authentication
	//lfc add for fastchg
	int	(*fast_chg_started) (void);
	int (*fast_switch_to_normal) (void);
	int (*set_switch_to_noraml_false) (void);
	int (*set_switch_to_noraml_true) (void);
	int (*set_fast_chg_allow) (int enable);
	int (*get_fast_chg_allow) (void);
	int (*fast_normal_to_warm)	(void);	
	int (*set_normal_to_warm_false)	(void);
	int	(*get_fast_chg_ing)	(void);
	int	(*get_fast_low_temp_full)	(void);
	int	(*set_low_temp_full_false)	(void);
	//lfc add for fastchg end
};


#define OPCHG_DEFAULT_BATT_CAPACITY            50
#define OPCHG_SOC_DEFAULT_COUNT					6
#define OPCHG_SOC_QUICKLY_COUNT					4



extern 	int opchg_get_prop_fast_chg_started(void);
extern	int opchg_get_prop_fast_chg_allow();
extern	int opchg_set_fast_chg_allow(int enable);
extern	int opchg_get_fast_low_temp_full();
extern	int opchg_get_prop_fast_switch_to_normal();
extern	int opchg_get_fast_chg_ing();
extern	int opchg_set_fast_normal_to_warm_false();
extern	int opchg_set_fast_switch_to_normal_false();
extern	int opchg_set_fast_switch_to_normal_true();

extern	int opchg_get_fast_normal_to_warm();	
extern void reset_fastchg_after_usbout(void);

extern	int opchg_get_prop_authenticate();
extern	int opchg_get_prop_current_now();
extern	bool opchg_get_prop_batt_present();
extern	int opchg_backup_ocv_soc(int soc);
extern int opchg_get_prop_batt_temp(void);
extern int opchg_get_prop_batt_capacity(void);
extern int opchg_set_switch_mode(u8 mode);
extern int opchg_get_prop_bq24196_full_status(void);

extern int opchg_set_reset_active();
extern int opchg_set_clock_active();
extern int opchg_set_clock_sleep();
extern int opchg_set_data_active();
extern int opchg_set_data_sleep();

extern struct opchg_bms_charger *opchg_pinctrl_chip;
extern int opchg_get_prop_battery_voltage_now();
extern void KernelVendorFastchgLog(void);

#ifdef CONFIG_GAUGE_BQ27411
struct cmd_address {
//bq27411 standard cmds
	u8	reg_cntl;
	u8	reg_temp;
	u8 	reg_volt;
	u8	reg_flags;
	u8	reg_nac;
	u8 	reg_fac;
	u8	reg_rm;
	u8	reg_fcc;
	u8	reg_ai;
	u8	reg_si;
	u8	reg_mli;
	u8	reg_ap;
	u8	reg_soc;
	u8	reg_inttemp;
	u8	reg_soh;
	u16	flag_dsc;
	u16	flag_fc;
	u16	cs_dlogen;
	u16 cs_ss;
	
//bq27541 external standard cmds
	u8	reg_ar;
	u8	reg_artte;
	u8	reg_tte;
	u8	reg_ttf;
	u8	reg_stte;
	u8	reg_mltte;
	u8	reg_ae;
	u8	reg_ttecp;
	u8	reg_cc;
	u8	reg_nic;
	u8	reg_icr;
	u8	reg_logidx;
	u8	reg_logbuf;
	u8	reg_dod0;


//bq27411 sub cmds
	u16 subcmd_cntl_status;
	u16 subcmd_device_type;
	u16 subcmd_fw_ver;
	u16 subcmd_dm_code;
	u16 subcmd_prev_macw;
	u16 subcmd_chem_id;
	u16 subcmd_set_hib;
	u16 subcmd_clr_hib;
	u16 subcmd_set_cfg;
	u16 subcmd_sealed;
	u16 subcmd_reset;
	u16 subcmd_softreset;
	u16 subcmd_exit_cfg;
	u16 subcmd_enable_dlog;
	u16 subcmd_disable_dlog;
	u16 subcmd_enable_it;
	u16 subcmd_disable_it;

//bq27541 external sub cmds
	u16 subcmd_hw_ver;
	u16 subcmd_df_csum;
	u16 subcmd_bd_offset;
	u16 subcmd_int_offset;
	u16 subcmd_cc_ver;
	u16 subcmd_ocv;
	u16 subcmd_bat_ins;
	u16 subcmd_bat_rem;
	u16 subcmd_set_slp;
	u16 subcmd_clr_slp;
	u16 subcmd_fct_res;
	u16 subcmd_cal_mode;
	
};
#endif


struct bms_bq27541 {
	struct i2c_client					*client;
	struct device						*dev;	
	
	struct work_struct		counter;
	/* 300ms delay is needed after bq27541 is powered up
	 * and before any successful I2C transaction
	 */
	struct  delayed_work		hw_config;
	int soc_pre;
	int temp_pre;
	int batt_vol_pre;
	int current_pre;
	int cc_pre;
	int soh_pre;
	int fcc_pre;
	int saltate_counter;
	bool is_authenticated;	//wangjc add for authentication
	bool fast_chg_started;
	bool fast_switch_to_normal;
	bool fast_normal_to_warm;	//lfc add for fastchg over temp
	int battery_type;			//lfc add for battery type
	struct power_supply		*batt_psy;
	int irq;
	struct work_struct fastcg_work;
	bool alow_reading;
	struct timer_list watchdog;
	struct wake_lock fastchg_wake_lock;
	bool fast_chg_allow;
	bool fast_low_temp_full;
	int retry_count;
	unsigned long rtc_resume_time;
	unsigned long rtc_suspend_time;
	atomic_t suspended;
	bool fast_chg_ing;

	
	int								opchg_swtich1_gpio;
	int								opchg_swtich2_gpio;
	int								opchg_reset_gpio;
	int								opchg_clock_gpio;
	int								opchg_data_gpio;

	struct pinctrl 						*pinctrl;
	struct pinctrl_state 					*gpio_switch1_act_switch2_act;
	struct pinctrl_state 					*gpio_switch1_sleep_switch2_sleep;
	struct pinctrl_state 					*gpio_switch1_act_switch2_sleep;
	struct pinctrl_state 					*gpio_switch1_sleep_switch2_act;
	
	struct pinctrl_state 					*gpio_clock_active;
	struct pinctrl_state 					*gpio_clock_sleep;
	struct pinctrl_state 					*gpio_data_active;
	struct pinctrl_state 					*gpio_data_sleep;
	struct pinctrl_state 					*gpio_reset_active;
	struct pinctrl_state 					*gpio_reset_sleep;

#ifdef CONFIG_GAUGE_BQ27411
	int device_type;
	struct cmd_address						cmd_addr;
#endif
	
};


