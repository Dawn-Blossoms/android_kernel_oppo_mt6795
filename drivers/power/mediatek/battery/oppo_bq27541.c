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
#include <asm/unaligned.h>

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>
#include <linux/xlog.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#include <linux/power_supply.h>

#include <linux/wakelock.h>
#include <linux/gpio.h>
#include "oppo_bq27541.h"
#include "oppo_vooc.h"
#include <mach/battery_meter.h>
#include "mt65xx_battery.h"
#include <mach/charging.h>
#include <mach/battery_common.h>
#include <mach/eint.h>
#include <cust_eint.h>
#include "oppo_bq24196.h"

#define bq24261_SLAVE_ADDR_WRITE   0xAA
#define bq24261_SLAVE_ADDR_Read    0xAB

#define OPPO_BQ27541_PAR
struct opchg_bms_charger *opchg_pinctrl_chip = NULL;

static struct i2c_client *new_client = NULL;
static const struct i2c_device_id bms_bq27541[] = {{"bq27541_i2c",0},{}};  
static struct i2c_board_info __initdata i2c_bq27541={ I2C_BOARD_INFO("bq27541_i2c", (bq24261_SLAVE_ADDR_WRITE >> 1))};

volatile kal_bool chargin_hw_init_done_bq27541 = 0;

static int bq27541_driver_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int bq27541_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);
int opchg_set_switch_mode(u8 mode);
static struct workqueue_struct *fastchg_wq = NULL;
#ifdef OPPO_USE_FAST_CHARGER

unsigned char *vooc_firmware_data = NULL;
struct opchg_fast_charger *opchg_fast_charger_chip = NULL;
int (*vooc_fw_update)(struct opchg_fast_charger *,bool) = NULL;

int vooc_fw_ver_count = 0;
int vooc_need_to_up_fw = 0;
int vooc_have_updated = 0;

extern volatile kal_bool chargin_hw_init_done_vooc;
extern int fast_charger_reset_count;

extern void mt_battery_update_status(void);
extern kal_bool g_bat_init_flag;
extern 	bool check_batt_exist(void);
extern int fastchg_present_flag;
//int opchg_bq27541_gpio_pinctrl_init(struct bms_bq27541 *di);
//int opchg_bq27541_parse_dt(struct bms_bq27541 *di);
//pengnan  2015/5/1 add for tp avoid charge disturb 
#define CHARGE_PLUG_IN_TP_AVOID_DISTURB
#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
extern int charge_plug_tp_avoid_distrub(int enable,int is_fast_charge);
extern int is_oppo_fast_charger;
#endif 
//end
extern void do_chrdet_int_task(void);
extern int gpio_oppo_vooc_sw_ctrl;

extern int at_test_chg_on;


int opchg_set_gpio_val(int gpio , u8 val);
int opchg_get_gpio_val(int gpio);
int opchg_set_gpio_dir_output(int gpio , u8 val);
int opchg_set_gpio_dir_intput(int gpio);

int opchg_set_clock_active();
int opchg_set_clock_sleep();
int opchg_set_data_active();
int opchg_set_data_sleep();

int opchg_set_switch_fast_charger(void);
int opchg_set_switch_normal_charger(void);
int opchg_set_switch_earphone(void);

int opchg_set_reset_active();
static void bq27541_reset(struct i2c_client *client);
#endif



struct i2c_driver bq27541_i2c_driver = {                       
    .probe = bq27541_driver_probe,                                       
    .detect = bq27541_driver_detect,                           
    .driver.name = "bq27541_i2c",                 
    .id_table = bms_bq27541, 
	.shutdown	= bq27541_reset,
};


struct bms_bq27541 *bq27541_di;
struct qpnp_battery_gauge *qpnp_batt_gauge = NULL;
static DEFINE_MUTEX(bq27541_i2c_access);

/**********************************************************
  *
  *   [I2C Function For Read/Write bq27541] 
  *
  *********************************************************/
  #if 0
 //static int bq27541_read_i2c(u8 reg, int *rt_value, int b_single,struct bms_bq27541 *di)
  kal_uint32 bq27541_read_i2c(kal_uint8 cmd, kal_uint8 *returnData)
{
	struct i2c_msg msg[2];
	unsigned char data[2];
	int err;

	if (!new_client->adapter)
		return -ENODEV;
	
	mutex_lock(&bq27541_i2c_access);
	
	/* Write register */
	msg[0].addr = new_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = data;

	data[0] = cmd;

	/* Read data */
	msg[1].addr = new_client->addr;
	msg[1].flags = I2C_M_RD;
	//if (!b_single)
		msg[1].len = 2;
	//else
	//	msg[1].len = 1;
	msg[1].buf = data;

	err = i2c_transfer(new_client->adapter, msg, 2);
	if (err >= 0) {
		//if (!b_single)
			*returnData = get_unaligned_le16(data);
		//else
		//	*rt_value = data[0];

		mutex_unlock(&bq27541_i2c_access);

		return 0;
	}

	mutex_unlock(&bq27541_i2c_access);

	return err;
}

/*
 * i2c specific code
 */
static int bq27541_i2c_txsubcmd(u8 reg, unsigned short subcmd)
{
	struct i2c_msg msg;
	unsigned char data[3];
	int ret;

	if (!new_client)
		return -ENODEV;

	memset(data, 0, sizeof(data));
	data[0] = reg;
	data[1] = subcmd & 0x00FF;
	data[2] = (subcmd & 0xFF00) >> 8;

	msg.addr = new_client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	ret = i2c_transfer(new_client->adapter, &msg, 1);
	if (ret < 0)
		return -EIO;

	return 0;
}


kal_uint32 bq27541_read_i2c(kal_uint8 cmd, kal_uint8 *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&bq27541_i2c_access);
    
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], (1<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;

        mutex_unlock(&bq27541_i2c_access);
        return ret;
    }
    
    readData = cmd_buf[0];
    *returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    
    mutex_unlock(&bq27541_i2c_access);    
    return 0;
}

kal_uint32 bq27541_i2c_txsubcmd(kal_uint8 cmd, kal_uint8 writeData)
{
    char    write_data[2] = {0};
    int     ret=0;
    
    mutex_lock(&bq27541_i2c_access);
    
    write_data[0] = cmd;
    write_data[1] = writeData;
    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&bq27541_i2c_access);
        return ret;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&bq27541_i2c_access);
    return 0;
}

#endif

kal_uint32 bq27541_read_i2c(int cmd, int *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int      ret=0;

#ifdef CONFIG_GAUGE_BQ27411
	if(cmd == BQ27541_BQ27411_CMD_INVALID)
		return 0;
#endif

    mutex_lock(&bq27541_i2c_access);
    
    *returnData = i2c_smbus_read_word_data(new_client,cmd);
    
    mutex_unlock(&bq27541_i2c_access); 
	//printk("bq27541_read_i2c,cmd = 0x%x, returnData = 0x%x\r\n",cmd,*returnData)  ; 
    return 0;
}

kal_uint32 bq27541_i2c_txsubcmd(kal_uint8 cmd, int writeData)
{

    int     ret=0;
	
#ifdef CONFIG_GAUGE_BQ27411
		if(cmd == BQ27541_BQ27411_CMD_INVALID)
			return 0;
#endif
    mutex_lock(&bq27541_i2c_access);
    i2c_smbus_write_word_data(new_client,cmd,writeData); 
    mutex_unlock(&bq27541_i2c_access);
    return 0;
}


#ifdef OPPO_USE_FAST_CHARGER
int opchg_get_prop_fast_chg_started()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->fast_chg_started)
		return qpnp_batt_gauge->fast_chg_started();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}

int opchg_get_prop_fast_chg_allow()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->get_fast_chg_allow)
		return qpnp_batt_gauge->get_fast_chg_allow();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}

int opchg_set_fast_chg_allow(int enable)
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->set_fast_chg_allow)
		return qpnp_batt_gauge->set_fast_chg_allow(enable);
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}

int opchg_set_fast_switch_to_normal_false()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->set_switch_to_noraml_false)
		return qpnp_batt_gauge->set_switch_to_noraml_false();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}
int opchg_set_fast_switch_to_normal_true()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->set_switch_to_noraml_true)
		return qpnp_batt_gauge->set_switch_to_noraml_true();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}

int opchg_set_fast_normal_to_warm_false()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->set_normal_to_warm_false)
		return qpnp_batt_gauge->set_normal_to_warm_false();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}

int opchg_get_fast_normal_to_warm()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->fast_normal_to_warm)
		return qpnp_batt_gauge->fast_normal_to_warm();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}

int opchg_get_fast_low_temp_full()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->get_fast_low_temp_full)
		return qpnp_batt_gauge->get_fast_low_temp_full();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}
int opchg_get_prop_fast_switch_to_normal()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->fast_switch_to_normal)
		return qpnp_batt_gauge->fast_switch_to_normal();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}

int opchg_get_fast_chg_ing()
{
	if (qpnp_batt_gauge && qpnp_batt_gauge->get_fast_chg_ing)
		return qpnp_batt_gauge->get_fast_chg_ing();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}
#endif


/*
 * Return the battery temperature in tenths of degree Celsius
 * Or < 0 if something fails.
 */
static int bq27541_battery_temperature()
{
	int ret;
	int temp = 0;
	static int count = 0;
	//return 250;
	if(atomic_read(&bq27541_di->suspended) == 1) {
		return bq27541_di->temp_pre + ZERO_DEGREE_CELSIUS_IN_TENTH_KELVIN;
	}

	if(bq27541_di->alow_reading == true) {
		ret = bq27541_read_i2c(bq27541_di->cmd_addr.reg_temp, &temp);
		if (ret) {
			count++;
			dev_err(bq27541_di, "error reading temperature\n");
			if(count > 1) {
				count = 0;
				
				bq27541_di->temp_pre = -400 - ZERO_DEGREE_CELSIUS_IN_TENTH_KELVIN;
				return -400;
			} else {
				return bq27541_di->temp_pre + ZERO_DEGREE_CELSIUS_IN_TENTH_KELVIN;
			}
		}
		count = 0;
	} else {
		return bq27541_di->temp_pre + ZERO_DEGREE_CELSIUS_IN_TENTH_KELVIN;	
	}

	bq27541_di->temp_pre = temp;

	return temp + ZERO_DEGREE_CELSIUS_IN_TENTH_KELVIN;
}


/* OPPO 2013-08-24 wangjc Add begin for add adc interface. */
static int bq27541_battery_cc()//sjc20150105
{
	int ret;
	int cc = 0;

	if (atomic_read(&bq27541_di->suspended) == 1)
		return bq27541_di->cc_pre;

	if (bq27541_di->alow_reading == true) {
		ret = bq27541_read_i2c(bq27541_di->cmd_addr.reg_cc, &cc);
		if (ret) {
			dev_err(bq27541_di->dev, "error reading cc.\n");
			return ret;
		}
	} else {
		if(bq27541_di->cc_pre)
			return bq27541_di->cc_pre;
		else
			return 0;
	}
	
	bq27541_di->cc_pre = cc;
	return cc;
}

static int bq27541_battery_fcc()//sjc20150105
{
	int ret;
	int fcc = 0;

	if (atomic_read(&bq27541_di->suspended) == 1)
		return bq27541_di->fcc_pre;

	if (bq27541_di->alow_reading == true) {
		ret = bq27541_read_i2c(bq27541_di->cmd_addr.reg_fcc, &fcc);
		if (ret) {
			dev_err(bq27541_di->dev, "error reading fcc.\n");
			return ret;
		}
	} else {
		if(bq27541_di->fcc_pre)
			return bq27541_di->fcc_pre;
		else
			return 0;
	}

	bq27541_di->fcc_pre = fcc;
	return fcc;
}

static int bq27541_battery_soh()//sjc20150105
{
	int ret;
	int soh = 0;

	if (atomic_read(&bq27541_di->suspended) == 1)
		return bq27541_di->soh_pre;

	if (bq27541_di->alow_reading == true) {
		ret = bq27541_read_i2c(bq27541_di->cmd_addr.reg_soh, &soh);
		if (ret) {
			dev_err(bq27541_di->dev, "error reading fcc.\n");
			return ret;
		}
	} else {
		if(bq27541_di->soh_pre)
			return bq27541_di->soh_pre;
		else
			return 0;
	}

	bq27541_di->soh_pre = soh;
	return soh;
}


static int bq27541_remaining_capacity()
{
	int ret;
	int cap = 0;

	if(bq27541_di->alow_reading == true) {
		ret = bq27541_read_i2c(bq27541_di->cmd_addr.reg_rm, &cap);
		if (ret) {
			dev_err(bq27541_di->dev, "error reading capacity.\n");
			return ret;
		}
	}

	return cap;
}

static int bq27541_soc_calibrate(int soc)
{
	union power_supply_propval ret = {0,};
	unsigned int soc_calib;
	//int counter_temp = 0;
	
	if(!bq27541_di->batt_psy){	
		bq27541_di->batt_psy = power_supply_get_by_name("battery");
		bq27541_di->soc_pre = soc;
	}
	soc_calib = soc;
	#if 0
	if(bq27541_di->batt_psy){
		bq27541_di->batt_psy->get_property(bq27541_di->batt_psy,POWER_SUPPLY_PROP_STATUS, &ret);
	
		if(ret.intval == POWER_SUPPLY_STATUS_CHARGING || ret.intval == POWER_SUPPLY_STATUS_FULL) { // is charging
			if(abs(soc - bq27541_di->soc_pre) >= 2) {
				bq27541_di->saltate_counter++;
				if(bq27541_di->saltate_counter < CAPACITY_SALTATE_COUNTER)
					return bq27541_di->soc_pre;
				else
					bq27541_di->saltate_counter = 0;
			}
			else
				bq27541_di->saltate_counter = 0;
		
			if(soc > bq27541_di->soc_pre) {
				soc_calib = bq27541_di->soc_pre + 1;
			} else if(soc < (bq27541_di->soc_pre - 2)) {
				
				soc_calib = bq27541_di->soc_pre - 1;
			} else {
				soc_calib = bq27541_di->soc_pre;
			}
			
			
			if(ret.intval == POWER_SUPPLY_STATUS_FULL) {
				if(soc > 94) {
					soc_calib = 100;
				}
			}
		}
		else{   // not charging
			if((abs(soc - bq27541_di->soc_pre) >= 2) || (bq27541_di->soc_pre > 80)) {
				bq27541_di->saltate_counter++;
				if(bq27541_di->soc_pre == 100) {
					counter_temp = CAPACITY_SALTATE_COUNTER_FULL;//6
				} else if (bq27541_di->soc_pre > 95) {
					counter_temp = CAPACITY_SALTATE_COUNTER_95;///3
				} else if (bq27541_di->soc_pre > 90) {
					counter_temp = CAPACITY_SALTATE_COUNTER_90;///2
				} else if(bq27541_di->soc_pre > 80) {
					counter_temp = CAPACITY_SALTATE_COUNTER_80;///1.5
				} else {
					counter_temp = CAPACITY_SALTATE_COUNTER_NOT_CHARGING;///1
				}
				if(bq27541_di->saltate_counter < counter_temp)
					return bq27541_di->soc_pre;
				else
					bq27541_di->saltate_counter = 0;
			}
			else
				bq27541_di->saltate_counter = 0;
			
			if(soc < bq27541_di->soc_pre)
				soc_calib = bq27541_di->soc_pre - 1;
			else
				soc_calib = bq27541_di->soc_pre;
		}
	}	
	else{
		soc_calib = soc;
	}
	#endif
	if(soc >= 100)
		soc_calib = 100;
	else if(soc < 0)
		soc_calib = 0;
	bq27541_di->soc_pre = soc_calib;
	//pr_info("soc:%d, soc_calib:%d\n", soc, soc_calib);
	return soc_calib;
}

static int bq27541_battery_soc(bool raw)
{
	int ret;
	int soc = 0;

	if(atomic_read(&bq27541_di->suspended) == 1) {
		return bq27541_di->soc_pre;
	}

	if(bq27541_di->alow_reading == true) {
		ret = bq27541_read_i2c(bq27541_di->cmd_addr.reg_soc, &soc);
		if (ret) {
			dev_err(bq27541_di->dev, "error reading soc.ret:%d\n",ret);
			goto read_soc_err;
		}
	} else {
		if(bq27541_di->soc_pre)
			return bq27541_di->soc_pre;
		else
			return 0;
	}
	ret = bq27541_battery_cc();
	ret = bq27541_battery_fcc();
	ret = bq27541_battery_soh();

	if(raw == true) {
		if(soc > 90) {
			soc += 2;
		}
		if(soc <= bq27541_di->soc_pre) {
			bq27541_di->soc_pre = soc;
		}
	}
	soc = bq27541_soc_calibrate(soc);
	return soc;
	
read_soc_err:
	if(bq27541_di->soc_pre)
		return bq27541_di->soc_pre;
	else
		return 0;
}

static int bq27541_average_current()
{
	int ret;
	int curr = 0;

	if(atomic_read(&bq27541_di->suspended) == 1) {
		return -bq27541_di->current_pre;
	}

	if(bq27541_di->alow_reading == true) {
		ret = bq27541_read_i2c(bq27541_di->cmd_addr.reg_ai, &curr);
		if (ret) {
			dev_err(bq27541_di->dev, "error reading current.\n");
			return ret;
		}
	} else {
		return -bq27541_di->current_pre;
	}
	// negative current
	if(curr&0x8000)
		curr = -((~(curr-1))&0xFFFF);
	bq27541_di->current_pre = curr;
	return -curr;
}

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
static int bq27541_battery_voltage()
{
	int ret;
	int volt = 0;

	if(atomic_read(&bq27541_di->suspended) == 1) {
		return bq27541_di->batt_vol_pre;
	}

	if(bq27541_di->alow_reading == true) {
		ret = bq27541_read_i2c(bq27541_di->cmd_addr.reg_volt, &volt);
		if (ret) {
			dev_err(bq27541_di->dev, "error reading voltage,ret:%d\n",ret);
			return ret;
		}
	} else {
		return bq27541_di->batt_vol_pre;
	}

	bq27541_di->batt_vol_pre = volt * 1000;

	return volt * 1000;
}
extern volatile kal_bool chargin_hw_init_done_bq24196;
static int bq27541_get_bq24196_full_status()
{
	int reg_val;
	static int reg_val_pre = 0;
	if (chargin_hw_init_done_bq24196 == KAL_TRUE) {
		if(bq27541_di->alow_reading == true) {
			reg_val =bq24196_registers_read_full();
		} else {
			return reg_val_pre;
		}
	}
	else {
		pr_err("chargin_hw_init_done_bq24196 is not ok\n");
	}

	reg_val_pre = reg_val;

	return reg_val;
}


static void bq27541_cntl_cmd(int subcmd)
{
	bq27541_i2c_txsubcmd(BQ27541_BQ27411_REG_CNTL, subcmd);
}


void qpnp_battery_gauge_register(struct qpnp_battery_gauge *batt_gauge)
{
	if (qpnp_batt_gauge) {
		qpnp_batt_gauge = batt_gauge;
		pr_err("qpnp-charger %s multiple battery gauge called\n",
								__func__);
	} else {
		qpnp_batt_gauge = batt_gauge;
	}
}
EXPORT_SYMBOL(qpnp_battery_gauge_register);

void qpnp_battery_gauge_unregister(struct qpnp_battery_gauge *batt_gauge)
{
	qpnp_batt_gauge = NULL;
}
EXPORT_SYMBOL(qpnp_battery_gauge_unregister);

#if 0
static void bq27541_coulomb_counter_work(struct work_struct *work)
{
	int value = 0, temp = 0, index = 0, ret = 0;
	struct bms_bq27541 *di;
	unsigned long flags;
	int count = 0;

	di = container_of(work, struct bms_bq27541, counter);

	/* retrieve 30 values from FIFO of coulomb data logging buffer
	 * and average over time
	 */
	do {
		ret = bq27541_read(BQ27541_REG_LOGBUF, &temp, 0, di);
		if (ret < 0)
			break;
		if (temp != 0x7FFF) {
			++count;
			value += temp;
		}
		/* delay 66uS, waiting time between continuous reading
		 * results
		 */
		udelay(66);
		ret = bq27541_read(BQ27541_REG_LOGIDX, &index, 0, di);
		if (ret < 0)
			break;
		udelay(66);
	} while (index != 0 || temp != 0x7FFF);

	if (ret < 0) {
		dev_err(di->dev, "Error reading datalog register\n");
		return;
	}

	if (count) {
		spin_lock_irqsave(&lock, flags);
		coulomb_counter = value/count;
		spin_unlock_irqrestore(&lock, flags);
	}
}
#endif

static int bq27541_get_battery_mvolts(void)
{
	return bq27541_battery_voltage();
}

static int bq27541_get_battery_temperature(void)
{
	return bq27541_battery_temperature();
}
static int bq27541_is_battery_present(void)
{
	return 1;
}
static int bq27541_is_battery_temp_within_range(void)
{
	return 1;
}
static int bq27541_is_battery_id_valid(void)
{
	return 1;
}

static int bq27541_get_batt_remaining_capacity(void)
{
	return bq27541_remaining_capacity();
}

static int bq27541_get_battery_soc(void)
{
	return bq27541_battery_soc(false);
}


static int bq27541_get_average_current(void)
{
	return bq27541_average_current();
}

//wangjc add for authentication
static int bq27541_is_battery_authenticated(void)
{
	if(bq27541_di) {
		return bq27541_di->is_authenticated;
	}
	return false;
}

static int bq27541_fast_chg_started(void)
{
	if(bq27541_di) {
		return bq27541_di->fast_chg_started;
	}
	return false;
}

static int bq27541_fast_switch_to_normal(void)
{
	if(bq27541_di) {
		//pr_err("%s fast_switch_to_normal:%d\n",__func__,bq27541_di->fast_switch_to_normal);
		return bq27541_di->fast_switch_to_normal;
	}
	return false;
}

static int bq27541_set_switch_to_noraml_false(void)
{
	if(bq27541_di) {
		bq27541_di->fast_switch_to_normal = false;
	}

	return 0;
}
static int bq27541_set_switch_to_noraml_true(void)
{
	if(bq27541_di) {
		bq27541_di->fast_switch_to_normal = true;
	}

	return 0;
}


static int bq27541_get_fast_low_temp_full(void)
{
	if(bq27541_di) {
		return bq27541_di->fast_low_temp_full;
	}
	return false;
}

static int bq27541_set_fast_low_temp_full_false(void)
{
	if(bq27541_di) {
		return bq27541_di->fast_low_temp_full = false;
	}
	return 0;
}

static int bq27541_fast_normal_to_warm(void)
{
	if(bq27541_di) {
		//pr_err("%s fast_switch_to_normal:%d\n",__func__,bq27541_di->fast_switch_to_normal);
		return bq27541_di->fast_normal_to_warm;
	}
	return 0;
}

static int bq27541_set_fast_normal_to_warm_false(void)
{
	if(bq27541_di) {
		bq27541_di->fast_normal_to_warm = false;
	}

	return 0;
}

static int bq27541_set_fast_chg_allow(int enable)
{
	if(bq27541_di) {
		bq27541_di->fast_chg_allow = enable;
	}
	return 0;
}

static int bq27541_get_fast_chg_allow(void)
{
	if(bq27541_di) {
		return bq27541_di->fast_chg_allow;
	}
	return 0;
}

static int bq27541_get_fast_chg_ing(void)
{
	if(bq27541_di) {
			return bq27541_di->fast_chg_ing;
		}
	return 0;
}

static struct qpnp_battery_gauge bq27541_batt_gauge = {
	.get_battery_mvolts		= bq27541_get_battery_mvolts,
	.get_battery_temperature	= bq27541_get_battery_temperature,
	.is_battery_present		= bq27541_is_battery_present,
	.is_battery_temp_within_range	= bq27541_is_battery_temp_within_range,
	.is_battery_id_valid		= bq27541_is_battery_id_valid,
	.get_batt_remaining_capacity = bq27541_get_batt_remaining_capacity,
	.get_battery_soc			= bq27541_get_battery_soc,
	.get_average_current		= bq27541_get_average_current,
	.is_battery_authenticated	= bq27541_is_battery_authenticated,
	.fast_chg_started			= bq27541_fast_chg_started,
	.fast_switch_to_normal		= bq27541_fast_switch_to_normal,
	.set_switch_to_noraml_false	= bq27541_set_switch_to_noraml_false,
	.set_switch_to_noraml_true	= bq27541_set_switch_to_noraml_true,
	.set_fast_chg_allow			= bq27541_set_fast_chg_allow,
	.get_fast_chg_allow			= bq27541_get_fast_chg_allow,
	.fast_normal_to_warm		= bq27541_fast_normal_to_warm,
	.set_normal_to_warm_false	= bq27541_set_fast_normal_to_warm_false,
	.get_fast_chg_ing			= bq27541_get_fast_chg_ing,
	.get_fast_low_temp_full		= bq27541_get_fast_low_temp_full,
	.set_low_temp_full_false	= bq27541_set_fast_low_temp_full_false,

};

static int bq27541_hw_init()
{
	int flags = 0, ret = 0;

	bq27541_cntl_cmd(BQ27541_BQ27411_SUBCMD_CTNL_STATUS);
	udelay(66);
	ret = bq27541_read_i2c(BQ27541_BQ27411_REG_CNTL, &flags);
	if (ret < 0) {
		dev_err(bq27541_di->dev, "error reading register %02x ret = %d\n",
			 BQ27541_BQ27411_REG_CNTL, ret);
		return ret;
	}
	udelay(66);

	bq27541_cntl_cmd(BQ27541_BQ27411_SUBCMD_ENABLE_IT);
	udelay(66);

	if (!(flags & BQ27541_BQ27411_CS_DLOGEN)) {
		bq27541_cntl_cmd(BQ27541_BQ27411_SUBCMD_ENABLE_DLOG);
		udelay(66);
	}

	return 0;
}


#define BATTERY_2700MA		0
#define BATTERY_3000MA		1
#define TYPE_INFO_LEN		8

#ifdef CONFIG_GAUGE_BQ27411
#define DEVICE_TYPE_BQ27541			0x0541
#define DEVICE_TYPE_BQ27411			0x0421
#define DEVICE_BQ27541				0
#define DEVICE_BQ27411				1

static void gauge_set_cmd_addr(int device_type)
{
	if(device_type == DEVICE_BQ27541){
		bq27541_di->cmd_addr.reg_cntl = BQ27541_BQ27411_REG_CNTL;
		bq27541_di->cmd_addr.reg_temp = BQ27541_REG_TEMP;
		bq27541_di->cmd_addr.reg_volt = BQ27541_REG_VOLT;
		bq27541_di->cmd_addr.reg_flags = BQ27541_REG_FLAGS;
		bq27541_di->cmd_addr.reg_nac = BQ27541_REG_NAC;
		bq27541_di->cmd_addr.reg_fac = BQ27541_REG_FAC;
		bq27541_di->cmd_addr.reg_rm = BQ27541_REG_RM;
		bq27541_di->cmd_addr.reg_fcc = BQ27541_REG_FCC;
		bq27541_di->cmd_addr.reg_ai = BQ27541_REG_AI;
		bq27541_di->cmd_addr.reg_si = BQ27541_REG_SI;
		bq27541_di->cmd_addr.reg_mli = BQ27541_REG_MLI;
		bq27541_di->cmd_addr.reg_ap = BQ27541_REG_AP;
		bq27541_di->cmd_addr.reg_soc = BQ27541_REG_SOC;
		bq27541_di->cmd_addr.reg_inttemp = BQ27541_REG_INTTEMP;
		bq27541_di->cmd_addr.reg_soh = BQ27541_REG_SOH;
		bq27541_di->cmd_addr.flag_dsc = BQ27541_FLAG_DSC;
		bq27541_di->cmd_addr.flag_fc = BQ27541_FLAG_FC;
		bq27541_di->cmd_addr.cs_dlogen = BQ27541_CS_DLOGEN;
		bq27541_di->cmd_addr.cs_ss = BQ27541_CS_SS;

		bq27541_di->cmd_addr.reg_ar = BQ27541_REG_AR;
		bq27541_di->cmd_addr.reg_artte = BQ27541_REG_ARTTE;
		bq27541_di->cmd_addr.reg_tte = BQ27541_REG_TTE;
		bq27541_di->cmd_addr.reg_ttf = BQ27541_REG_TTF;
		bq27541_di->cmd_addr.reg_stte = BQ27541_REG_STTE;
		bq27541_di->cmd_addr.reg_mltte = BQ27541_REG_MLTTE;
		bq27541_di->cmd_addr.reg_ae = BQ27541_REG_AE;
		bq27541_di->cmd_addr.reg_ttecp = BQ27541_REG_TTECP;
		bq27541_di->cmd_addr.reg_cc = BQ27541_REG_CC;
		bq27541_di->cmd_addr.reg_nic = BQ27541_REG_NIC;
		bq27541_di->cmd_addr.reg_icr = BQ27541_REG_ICR;
		bq27541_di->cmd_addr.reg_logidx = BQ27541_REG_LOGIDX;
		bq27541_di->cmd_addr.reg_logbuf = BQ27541_REG_LOGBUF;
		bq27541_di->cmd_addr.reg_dod0 = BQ27541_REG_DOD0;

		bq27541_di->cmd_addr.subcmd_cntl_status = BQ27541_SUBCMD_CTNL_STATUS;
		bq27541_di->cmd_addr.subcmd_device_type = BQ27541_SUBCMD_DEVCIE_TYPE;
		bq27541_di->cmd_addr.subcmd_fw_ver = BQ27541_SUBCMD_FW_VER;
		bq27541_di->cmd_addr.subcmd_dm_code = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_prev_macw = BQ27541_SUBCMD_PREV_MACW;
		bq27541_di->cmd_addr.subcmd_chem_id = BQ27541_SUBCMD_CHEM_ID;
		bq27541_di->cmd_addr.subcmd_set_hib = BQ27541_SUBCMD_SET_HIB;
		bq27541_di->cmd_addr.subcmd_clr_hib = BQ27541_SUBCMD_CLR_HIB;
		bq27541_di->cmd_addr.subcmd_set_cfg = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_sealed = BQ27541_SUBCMD_SEALED;
		bq27541_di->cmd_addr.subcmd_reset = BQ27541_SUBCMD_RESET;
		bq27541_di->cmd_addr.subcmd_softreset = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_exit_cfg = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_enable_dlog = BQ27541_SUBCMD_ENABLE_DLOG;
		bq27541_di->cmd_addr.subcmd_disable_dlog = BQ27541_SUBCMD_DISABLE_DLOG;
		bq27541_di->cmd_addr.subcmd_enable_it = BQ27541_SUBCMD_ENABLE_IT;
		bq27541_di->cmd_addr.subcmd_disable_it = BQ27541_SUBCMD_DISABLE_IT;

		bq27541_di->cmd_addr.subcmd_hw_ver = BQ27541_SUBCMD_HW_VER;
		bq27541_di->cmd_addr.subcmd_df_csum = BQ27541_SUBCMD_DF_CSUM;
		bq27541_di->cmd_addr.subcmd_bd_offset = BQ27541_SUBCMD_BD_OFFSET;
		bq27541_di->cmd_addr.subcmd_int_offset = BQ27541_SUBCMD_INT_OFFSET;
		bq27541_di->cmd_addr.subcmd_cc_ver = BQ27541_SUBCMD_CC_VER;
		bq27541_di->cmd_addr.subcmd_ocv = BQ27541_SUBCMD_OCV;
		bq27541_di->cmd_addr.subcmd_bat_ins = BQ27541_SUBCMD_BAT_INS;
		bq27541_di->cmd_addr.subcmd_bat_rem = BQ27541_SUBCMD_BAT_REM;
		bq27541_di->cmd_addr.subcmd_set_slp = BQ27541_SUBCMD_SET_SLP;
		bq27541_di->cmd_addr.subcmd_clr_slp = BQ27541_SUBCMD_CLR_SLP;
		bq27541_di->cmd_addr.subcmd_fct_res = BQ27541_SUBCMD_FCT_RES;
		bq27541_di->cmd_addr.subcmd_cal_mode = BQ27541_SUBCMD_CAL_MODE;
	} else {		//device_bq27411
		bq27541_di->cmd_addr.reg_cntl = BQ27411_REG_CNTL;
		bq27541_di->cmd_addr.reg_temp = BQ27411_REG_TEMP;
		bq27541_di->cmd_addr.reg_volt = BQ27411_REG_VOLT;
		bq27541_di->cmd_addr.reg_flags = BQ27411_REG_FLAGS;
		bq27541_di->cmd_addr.reg_nac = BQ27411_REG_NAC;
		bq27541_di->cmd_addr.reg_fac = BQ27411_REG_FAC;
		bq27541_di->cmd_addr.reg_rm = BQ27411_REG_RM;
		bq27541_di->cmd_addr.reg_fcc = BQ27411_REG_FCC;
		bq27541_di->cmd_addr.reg_ai = BQ27411_REG_AI;
		bq27541_di->cmd_addr.reg_si = BQ27411_REG_SI;
		bq27541_di->cmd_addr.reg_mli = BQ27411_REG_MLI;
		bq27541_di->cmd_addr.reg_ap = BQ27411_REG_AP;
		bq27541_di->cmd_addr.reg_soc = BQ27411_REG_SOC;
		bq27541_di->cmd_addr.reg_inttemp = BQ27411_REG_INTTEMP;
		bq27541_di->cmd_addr.reg_soh = BQ27411_REG_SOH;
		bq27541_di->cmd_addr.flag_dsc = BQ27411_FLAG_DSC;
		bq27541_di->cmd_addr.flag_fc = BQ27411_FLAG_FC;
		bq27541_di->cmd_addr.cs_dlogen = BQ27411_CS_DLOGEN;
		bq27541_di->cmd_addr.cs_ss = BQ27411_CS_SS;
		/*bq27541 external standard cmds*/
		bq27541_di->cmd_addr.reg_ar = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_artte = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_tte = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_ttf = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_stte = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_mltte = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_ae = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_ttecp = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_cc = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_nic = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_icr = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_logidx = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_logbuf = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.reg_dod0 = BQ27541_BQ27411_CMD_INVALID;

		bq27541_di->cmd_addr.subcmd_cntl_status = BQ27411_SUBCMD_CNTL_STATUS;
		bq27541_di->cmd_addr.subcmd_device_type = BQ27411_SUBCMD_DEVICE_TYPE;
		bq27541_di->cmd_addr.subcmd_fw_ver = BQ27411_SUBCMD_FW_VER;
		bq27541_di->cmd_addr.subcmd_dm_code = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_prev_macw = BQ27411_SUBCMD_PREV_MACW;
		bq27541_di->cmd_addr.subcmd_chem_id = BQ27411_SUBCMD_CHEM_ID;
		bq27541_di->cmd_addr.subcmd_set_hib = BQ27411_SUBCMD_SET_HIB;
		bq27541_di->cmd_addr.subcmd_clr_hib = BQ27411_SUBCMD_CLR_HIB;
		bq27541_di->cmd_addr.subcmd_set_cfg = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_sealed = BQ27411_SUBCMD_SEALED;
		bq27541_di->cmd_addr.subcmd_reset = BQ27411_SUBCMD_RESET;
		bq27541_di->cmd_addr.subcmd_softreset = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_exit_cfg = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_enable_dlog = BQ27411_SUBCMD_ENABLE_DLOG;
		bq27541_di->cmd_addr.subcmd_disable_dlog = BQ27411_SUBCMD_DISABLE_DLOG;
		bq27541_di->cmd_addr.subcmd_enable_it = BQ27411_SUBCMD_ENABLE_IT;
		bq27541_di->cmd_addr.subcmd_disable_it = BQ27411_SUBCMD_DISABLE_IT;
		/*bq27541 external sub cmds*/
		bq27541_di->cmd_addr.subcmd_hw_ver = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_df_csum = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_bd_offset = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_int_offset = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_cc_ver = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_ocv = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_bat_ins = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_bat_rem = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_set_slp = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_clr_slp = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_fct_res = BQ27541_BQ27411_CMD_INVALID;
		bq27541_di->cmd_addr.subcmd_cal_mode = BQ27541_BQ27411_CMD_INVALID;
	}
}

#endif


//static bool bq27541_authenticate(struct i2c_client *client);
//static int bq27541_batt_type_detect(struct i2c_client *client);
static void bq27541_hw_config(struct work_struct *work)
{
	int ret = 0, flags = 0, type = 0, fw_ver = 0;
	struct bms_bq27541 *di;

	di  = container_of(work, struct bms_bq27541, hw_config.work);
	ret = bq27541_hw_init();
	if (ret) {
		dev_err(di->dev, "Failed to config Bq27541\n");
		di->retry_count--;
		if(di->retry_count > 0) {
			schedule_delayed_work(&di->hw_config, HZ);
		}
		return;
	}
	
	qpnp_battery_gauge_register(&bq27541_batt_gauge);
	bq27541_cntl_cmd(BQ27541_BQ27411_SUBCMD_CTNL_STATUS);
	udelay(66);
	bq27541_read_i2c(BQ27541_BQ27411_REG_CNTL, &flags);
	bq27541_cntl_cmd(BQ27541_BQ27411_SUBCMD_DEVICE_TYPE);
	udelay(66);
	bq27541_read_i2c(BQ27541_BQ27411_REG_CNTL, &type);
	bq27541_cntl_cmd(BQ27541_BQ27411_SUBCMD_FW_VER);
	udelay(66);
	bq27541_read_i2c(BQ27541_BQ27411_REG_CNTL, &fw_ver);

#ifdef CONFIG_GAUGE_BQ27411
	if(type == DEVICE_TYPE_BQ27411)
		bq27541_di->device_type = DEVICE_BQ27411;
	else
		bq27541_di->device_type = DEVICE_BQ27541;
	gauge_set_cmd_addr(bq27541_di->device_type);
#endif

	//di->is_authenticated = bq27541_authenticate(di->client);
	//di->battery_type = bq27541_batt_type_detect(di->client);
	di->battery_type = BATTERY_3000MA;
	dev_err(di->dev, "DEVICE_TYPE is 0x%02X, FIRMWARE_VERSION is 0x%02X\n",
			type, fw_ver);
	dev_info(di->dev, "Complete bq27541 configuration 0x%02X\n", flags);
}

#ifdef OPPO_USE_FAST_CHARGER
void reset_fastchg_after_usbout(void)
{
	int rc =0;
	if(opchg_get_prop_fast_chg_started() == false) {
		pr_err("%s switch off fastchg\n", __func__);
			//opchg_set_switch_sleep(opchg_pinctrl_chip);
			rc =opchg_set_switch_mode(NORMAL_CHARGER_MODE);
	}
	rc =opchg_set_fast_switch_to_normal_false();
	rc =opchg_set_fast_normal_to_warm_false();
	//whenever charger gone, mask allo fast chg.
	rc =opchg_set_fast_chg_allow(false);
	rc =opchg_get_fast_low_temp_full();
}
#endif
  
//static irqreturn_t irq_rx_handler(int irq, void *dev_id)
static void irq_rx_handler(void)
{
	//struct bms_bq27541 *di = dev_id;
	//pr_info("%s\n", __func__);
	
	//schedule_work(&bq27541_di->fastcg_work);
	queue_work(fastchg_wq, &bq27541_di->fastcg_work);
	//return IRQ_HANDLED;
}


static void fastcg_work_func(struct work_struct *work)
{
	int data = 0;
	int i;
	int bit = 0;
	int retval = 0;
	int ret_info = 0;
	static int fw_ver_info = 0;
	int volt = 0;
	int temp = 0;
	int soc = 0;
	int current_now = 0;
	//int remain_cap = 0;
	static bool isnot_power_on = 0;
	int rc =0	;
	static bool plug_in_fastcharger = false;

	mt_eint_mask(CUST_EINT_MCU_AP_DATA);
	if(1)
	{
		usleep_range(2000,2000);
		if(mt_get_gpio_in(OPPO_VOOC_MCU_AP_DATA) !=1 )
		{
			printk("shield fastcg irq,return\r\n");
			mt_set_gpio_mode(OPPO_VOOC_MCU_AP_DATA, GPIO_MODE_00);
			mt_set_gpio_dir(OPPO_VOOC_MCU_AP_DATA,GPIO_DIR_IN);
			mt_set_gpio_pull_enable(OPPO_VOOC_MCU_AP_DATA, GPIO_PULL_ENABLE);
			mt_set_gpio_pull_select(OPPO_VOOC_MCU_AP_DATA, GPIO_PULL_UP);
		    mt_eint_registration(CUST_EINT_MCU_AP_DATA, IRQF_TRIGGER_RISING, irq_rx_handler, 0);
			mt_eint_unmask(CUST_EINT_MCU_AP_DATA);
			return;
		}		
	}
	//free_irq(bq27541_di->irq, bq27541_di);
	//mt_eint_mask(CUST_EINT_MCU_AP_DATA);
	for(i = 0; i < 7; i++) 
	{
		#if 1
			mt_set_gpio_mode(OPPO_VOOC_MCU_AP_CLK, GPIO_MODE_00);	//Set GPIO P9.3 as Output
			mt_set_gpio_mode(OPPO_VOOC_MCU_AP_DATA, GPIO_MODE_00);	//Set GPIO P9.3 as Output
			mt_set_gpio_dir(OPPO_VOOC_MCU_AP_CLK, GPIO_DIR_OUT);
			mt_set_gpio_out(OPPO_VOOC_MCU_AP_CLK, 0);
			usleep_range(1000,1000);
			mt_set_gpio_out(OPPO_VOOC_MCU_AP_CLK, 1);
			usleep_range(19000,19000);
			mt_set_gpio_dir(OPPO_VOOC_MCU_AP_DATA, GPIO_DIR_IN);
			bit = mt_get_gpio_in(OPPO_VOOC_MCU_AP_DATA); 
		#else
			gpio_set_value(0, 0);
			gpio_tlmm_config(AP_TX_EN, GPIO_CFG_ENABLE);
			usleep_range(1000,1000);
			gpio_set_value(0, 1);
			gpio_tlmm_config(AP_TX_DIS, GPIO_CFG_ENABLE);
			usleep_range(19000,19000);
			bit = gpio_get_value(1);
		#endif

		data |= bit<<(6-i);	
		if((i == 2) && (data != 0x50) && (!fw_ver_info)){	//data recvd not start from "101"
			pr_err("%s data err:%d\n",__func__,data);
			if(bq27541_di)
				fast_charger_reset_count=0;
			if(bq27541_di->fast_chg_started == true) {
				bq27541_di->alow_reading = true;
				bq27541_di->fast_chg_started = false;
				bq27541_di->fast_chg_allow = false;
				bq27541_di->fast_switch_to_normal = false;
				bq27541_di->fast_normal_to_warm = false;
				bq27541_di->fast_chg_ing = false;
				del_timer(&bq27541_di->watchdog);
			#if 1
				//rc =opchg_set_switch_sleep(opchg_pinctrl_chip);
				rc =opchg_set_switch_mode(NORMAL_CHARGER_MODE);
			#else
				gpio_set_value(96, 0);
				retval = gpio_tlmm_config(AP_SWITCH_USB, GPIO_CFG_ENABLE);
				if (retval) {
					pr_err("%s switch usb error %d\n", __func__, retval);
				}
			#endif				
				power_supply_changed(bq27541_di->batt_psy);
			}
			goto out;
		}
	}

	printk("%s recv data:0x%x,fw_version = 0x%x\n", __func__, data, vooc_firmware_data[vooc_fw_ver_count - 4]);

	if(data == VOOC_NOTIFY_FAST_PRESENT) {
		//request fast charging
		wake_lock(&bq27541_di->fastchg_wake_lock);
		vooc_need_to_up_fw = 0;
		fw_ver_info = 0;
		bq27541_di->alow_reading = false;
		bq27541_di->fast_chg_started = true;
		bq27541_di->fast_chg_allow = false;
		bq27541_di->fast_normal_to_warm = false;
		fastchg_present_flag = 1;
		mod_timer(&bq27541_di->watchdog,
		  jiffies + msecs_to_jiffies(15000));
		if(!isnot_power_on){
			isnot_power_on = 1;
			ret_info = 0x1;
		} else {
			ret_info = 0x2;
		}
//		isnot_power_on = 0;
		plug_in_fastcharger = true;
		
	} 
	else if(data == VOOC_NOTIFY_FAST_ABSENT) 
	{
		//fast charge stopped
		if(at_test_chg_on == 0){
			usleep_range(1000000,1000000);
		}
		bq27541_di->alow_reading = true;
		bq27541_di->fast_chg_started = false;
		bq27541_di->fast_chg_allow = false;
		bq27541_di->fast_switch_to_normal = false;
		bq27541_di->fast_normal_to_warm = false;
		bq27541_di->fast_chg_ing = false;
	
		//switch off fast chg
		pr_info("%s fastchg stop unexpectly,switch off fastchg\n", __func__);

	#if 1
		//rc =opchg_set_switch_sleep(opchg_pinctrl_chip);
		rc = opchg_set_switch_mode(NORMAL_CHARGER_MODE);
	#else
		
		gpio_set_value(96, 0);
		retval = gpio_tlmm_config(AP_SWITCH_USB, GPIO_CFG_ENABLE);
		if (retval) {
			pr_err("%s switch usb error %d\n", __func__, retval);
		}
	#endif		
		
		del_timer(&bq27541_di->watchdog);
		ret_info = 0x2;
		if(g_bat_init_flag == KAL_TRUE)
		{
			do_chrdet_int_task();
			//mt_battery_update_status();
		}
	} 
	else if(data == VOOC_NOTIFY_ALLOW_READING_IIC) 
	{
		//tell ap can read i2c
		bq27541_di->alow_reading = true;
		//reading
		bq27541_di->fast_chg_ing = true;
		
		volt = bq27541_get_battery_mvolts();
		temp = bq27541_get_battery_temperature();
	//	remain_cap = bq27541_get_batt_remaining_capacity();
		soc = bq27541_get_battery_soc();
		current_now = bq27541_get_average_current();
		bq24196_kick_wdt();
		pr_err("%s volt:%d,temp:%d,soc:%d,current_now:%d\n",__func__,volt,temp,soc,current_now);	
		if(plug_in_fastcharger == true){
			plug_in_fastcharger = false;
			#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
			// add for tp avoid charge disturb 
			charge_plug_tp_avoid_distrub(3,is_oppo_fast_charger);//for TP
			is_oppo_fast_charger = 1;
			#endif  //CHARGE_PLUG_IN_TP_AVOID_DISTURB
		}
		//don't read
		bq27541_di->alow_reading = false;
		mod_timer(&bq27541_di->watchdog,
			  jiffies + msecs_to_jiffies(10000));
		ret_info = 0x2;
	} 
	else if(data == VOOC_NOTIFY_NORMAL_TEMP_FULL)
	{
		//fastchg full,vbatt > 4350
#if 0	//lfc modify for it(set fast_switch_to_normal ture) is earlier than usb_plugged_out irq(set it false)
		bq27541_di->fast_switch_to_normal = true;
		bq27541_di->alow_reading = true;
		bq27541_di->fast_chg_started = false;
		bq27541_di->fast_chg_allow = false;
#endif
	#if 1	
		//rc =opchg_set_switch_sleep(opchg_pinctrl_chip);
		rc =opchg_set_switch_mode(NORMAL_CHARGER_MODE);
	#else
		gpio_set_value(96, 0);
		retval = gpio_tlmm_config(AP_SWITCH_USB, GPIO_CFG_ENABLE);
		if (retval) {
			pr_err("%s switch usb error %d\n", __func__, retval);
		}
	#endif	
		del_timer(&bq27541_di->watchdog);
		ret_info = 0x2;
	}
	else if(data == VOOC_NOTIFY_LOW_TEMP_FULL)
	{
		if (bq27541_di->battery_type == BATTERY_3000MA){	//13097 ATL battery
			//if temp:10~20 decigec,vddmax = 4250mv
			//switch off fast chg
			pr_info("%s fastchg low temp full,switch off fastchg,set GPIO96 0\n", __func__);

		#if 1
			//rc =opchg_set_switch_sleep(opchg_pinctrl_chip);
			rc =opchg_set_switch_mode(NORMAL_CHARGER_MODE);
		#else
			gpio_set_value(96, 0);
			retval = gpio_tlmm_config(AP_SWITCH_USB, GPIO_CFG_ENABLE);
			if (retval) {
				pr_err("%s switch usb error %d\n", __func__, retval);
			}
		#endif					
		}
		del_timer(&bq27541_di->watchdog);
		ret_info = 0x2;
	}
	else if(data == VOOC_NOTIFY_BAD_CONNECTED)
	{
		//usb bad connected,stop fastchg
#if 0	//lfc modify for it(set fast_switch_to_normal ture) is earlier than usb_plugged_out irq(set it false)
		bq27541_di->alow_reading = true;
		bq27541_di->fast_chg_started = false;
		bq27541_di->fast_chg_allow = false;
		bq27541_di->fast_switch_to_normal = true;
#endif
	#if 1	
		//rc =opchg_set_switch_sleep(opchg_pinctrl_chip);
		rc =opchg_set_switch_mode(NORMAL_CHARGER_MODE);
	#else
		gpio_set_value(96, 0);
		retval = gpio_tlmm_config(AP_SWITCH_USB, GPIO_CFG_ENABLE);
		if (retval) {
			pr_err("%s switch usb error %d\n", __func__, retval);
		}
	#endif		
		del_timer(&bq27541_di->watchdog);
		ret_info = 0x2;
	}
	else if(data == VOOC_NOTIFY_TEMP_OVER)
	{
		//fastchg temp over 45 or under 20
		pr_info("%s fastchg temp > 45 or < 20,switch off fastchg,set GPIO96 0\n", __func__);

	#if 1
		//rc =opchg_set_switch_sleep(opchg_pinctrl_chip);
		rc =opchg_set_switch_mode(NORMAL_CHARGER_MODE);
	#else	
		gpio_set_value(96, 0);
		retval = gpio_tlmm_config(AP_SWITCH_USB, GPIO_CFG_ENABLE);
		if (retval) {
			pr_err("%s switch usb error %d\n", __func__, retval);
		}
	#endif

		del_timer(&bq27541_di->watchdog);
		ret_info = 0x2;
	}
	else if(data == VOOC_NOTIFY_FIRMWARE_UPDATE)
	{
		//ready to get fw_ver
		fw_ver_info = 1;
		ret_info = 0x2;
	} 
	else if(fw_ver_info)
	{
		//get fw_ver
		//fw in local is large than mcu1503_fw_ver
		if((!vooc_have_updated) && (vooc_firmware_data[vooc_fw_ver_count - 4] != data)){
			ret_info = 0x2;
			vooc_need_to_up_fw = 1;	//need to update fw
		}else{
			ret_info = 0x1;
			vooc_need_to_up_fw = 0;	//fw is already new,needn't to up
		}
		pr_info("local_fw:0x%x,need_to_up_fw:%d\n",vooc_firmware_data[vooc_fw_ver_count - 4],vooc_need_to_up_fw);
		fw_ver_info = 0;
	} 
	else 
	{

	#if 1
		//rc =opchg_set_switch_sleep(opchg_pinctrl_chip);
		rc =opchg_set_switch_mode(NORMAL_CHARGER_MODE);
		/*if (rc) {
			pr_err("%s data err(101xxxx) switch usb error %d\n", __func__, retval);
			goto out;	//avoid i2c conflict
		}*/
	#else
		gpio_set_value(96, 0);
		retval = gpio_tlmm_config(AP_SWITCH_USB, GPIO_CFG_ENABLE);
		if (retval) {
			pr_err("%s data err(101xxxx) switch usb error %d\n", __func__, retval);
			goto out;	//avoid i2c conflict
		}
	#endif		
		msleep(500);	//avoid i2c conflict
		//data err
		bq27541_di->alow_reading = true;
		bq27541_di->fast_chg_started = false;
		bq27541_di->fast_chg_allow = false;
		bq27541_di->fast_switch_to_normal = false;
		bq27541_di->fast_normal_to_warm = false;
		bq27541_di->fast_chg_ing = false;
		//data err
		pr_err("%s data err,set 0x101,data=0x%x switch off fastchg\n", __func__, data);
		power_supply_changed(bq27541_di->batt_psy);
		goto out;
	}
	msleep(2);

#if 1
	//if(is_project(OPPO_14005)||is_project(OPPO_14023))
	{
		rc =opchg_set_data_sleep(opchg_pinctrl_chip);
		for(i = 0; i < 3; i++) {
			if(i == 0){	//tell mcu1503 battery_type
				//opchg_set_gpio_val(opchg_pinctrl_chip->opchg_data_gpio, ret_info >> 1);
				mt_set_gpio_out(OPPO_VOOC_MCU_AP_DATA, ret_info >> 1);
			} else if(i == 1){
				mt_set_gpio_out(OPPO_VOOC_MCU_AP_DATA, ret_info & 0x1);
			} else {
				mt_set_gpio_out(OPPO_VOOC_MCU_AP_DATA,bq27541_di->device_type);
				printk("ppp device_type = %d\n",bq27541_di->device_type);
//				mt_set_gpio_out(OPPO_VOOC_MCU_AP_DATA,0);
			}
			rc =opchg_set_clock_active();
			usleep_range(1000,1000);
			rc =opchg_set_clock_sleep();
			usleep_range(19000,19000);
		}
	}
#else
	gpio_tlmm_config(AP_RX_DIS,1);
	gpio_direction_output(1, 0);
	for(i = 0; i < 3; i++) {
		if(i == 0){	//tell mcu1503 battery_type
			gpio_set_value(1, ret_info >> 1);
		} else if(i == 1){
			gpio_set_value(1, ret_info & 0x1);
		} else {
			gpio_set_value(1,bq27541_di->battery_type);
		}
		
		gpio_set_value(0, 0);
		gpio_tlmm_config(AP_TX_EN, GPIO_CFG_ENABLE);
		usleep_range(1000,1000);
		gpio_set_value(0, 1);
		gpio_tlmm_config(AP_TX_DIS, GPIO_CFG_ENABLE);
		usleep_range(19000,19000);
	}
#endif
out:

	rc = opchg_set_data_active();

	rc = opchg_set_clock_active();
	usleep_range(10000,10000);
	rc = opchg_set_clock_sleep();
	usleep_range(25000,25000);
	
	//lfc add for it is faster than usb_plugged_out irq to send 0x5a(fast_chg full and usb bad connected) to AP
	if(data == VOOC_NOTIFY_NORMAL_TEMP_FULL || data == VOOC_NOTIFY_BAD_CONNECTED){
		if(bq27541_di->fast_chg_started == true)
		{
			usleep_range(650000,650000);
			bq27541_di->fast_switch_to_normal = true;
			bq27541_di->alow_reading = true;
			bq27541_di->fast_chg_started = false;
			bq27541_di->fast_chg_allow = false;
			bq27541_di->fast_chg_ing = false;
			#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
			//pengnan  2015/5/1 add for tp avoid charge disturb 
			charge_plug_tp_avoid_distrub(1,is_oppo_fast_charger);
			#endif 
		}
		else {
			//do nothing
		}
		
	}
	//fastchg temp over( > 45 or < 20)

	//lfc add to set fastchg vddmax = 4250mv during 10 ~ 20 decigec for ATL 3000mAH battery
	if(data == VOOC_NOTIFY_LOW_TEMP_FULL){
		if(bq27541_di->battery_type == BATTERY_3000MA){	//13097 ATL battery
			usleep_range(180000,180000);
			bq27541_di->fast_low_temp_full = true;
			bq27541_di->alow_reading = true;
			bq27541_di->fast_chg_started = false;
			bq27541_di->fast_chg_allow = false;
			bq27541_di->fast_chg_ing = false;
		}
	}
	//lfc add to set fastchg vddmax = 4250mv end
	
	if(data == VOOC_NOTIFY_TEMP_OVER){
		usleep_range(650000,650000);
		bq27541_di->fast_normal_to_warm = true;
		bq27541_di->alow_reading = true;
		bq27541_di->fast_chg_started = false;
		bq27541_di->fast_chg_allow = false;
		bq27541_di->fast_chg_ing = false;
		#ifdef CHARGE_PLUG_IN_TP_AVOID_DISTURB
		//pengnan  2015/5/1 add for tp avoid charge disturb 
		charge_plug_tp_avoid_distrub(1,is_oppo_fast_charger);
		#endif 
	}
	
	#ifdef OPPO_USE_FAST_CHARGER
	if(vooc_need_to_up_fw){
		msleep(500);
		del_timer(&bq27541_di->watchdog);

		#if 1
		rc = vooc_fw_update(opchg_fast_charger_chip,false);
		#else
		//vooc_fw_update(false);
		#endif
		vooc_need_to_up_fw = 0;
		mod_timer(&bq27541_di->watchdog,
			  jiffies + msecs_to_jiffies(10000));
	}
	#endif
	#if 0
	retval = request_irq(bq27541_di->irq, irq_rx_handler, IRQF_TRIGGER_RISING, "mcu_data", bq27541_di);	//0X01:rising edge,0x02:falling edge
	if(retval < 0) {
	pr_err("%s request ap rx irq failed.\n", __func__);
	}
	#endif
	mt_set_gpio_mode(OPPO_VOOC_MCU_AP_DATA, GPIO_MODE_00);
	mt_set_gpio_dir(OPPO_VOOC_MCU_AP_DATA,GPIO_DIR_IN);
	mt_set_gpio_pull_enable(OPPO_VOOC_MCU_AP_DATA, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(OPPO_VOOC_MCU_AP_DATA, GPIO_PULL_UP);
    mt_eint_registration(CUST_EINT_MCU_AP_DATA, IRQF_TRIGGER_RISING, irq_rx_handler, 0);
	mt_eint_unmask(CUST_EINT_MCU_AP_DATA);
	if((data == VOOC_NOTIFY_FAST_PRESENT) || (data == VOOC_NOTIFY_ALLOW_READING_IIC)){
		power_supply_changed(bq27541_di->batt_psy);
	}

	if(data == VOOC_NOTIFY_LOW_TEMP_FULL){
		if(bq27541_di->battery_type == BATTERY_3000MA){
			power_supply_changed(bq27541_di->batt_psy);
			wake_unlock(&bq27541_di->fastchg_wake_lock);
		}
	}	
	if((data == VOOC_NOTIFY_FAST_ABSENT) || (data == VOOC_NOTIFY_NORMAL_TEMP_FULL) 
					|| (data == VOOC_NOTIFY_BAD_CONNECTED) || (data == VOOC_NOTIFY_TEMP_OVER)){
		power_supply_changed(bq27541_di->batt_psy);
		wake_unlock(&bq27541_di->fastchg_wake_lock);
	}
}

void di_watchdog(unsigned long data)
{
	struct bms_bq27541 *di = (struct bms_bq27541 *)data;

	int rc = 0;
	
	pr_err("di_watchdog can't receive mcu data\n");
	di->alow_reading = true;
	di->fast_chg_started = false;
	di->fast_switch_to_normal = false;
	di->fast_low_temp_full = false;
	di->fast_chg_allow = false;
	di->fast_normal_to_warm = false;
	di->fast_chg_ing = false;
	//switch off fast chg
	pr_info("%s switch off fastchg\n", __func__);
	
#if 1
	//rc =opchg_set_switch_sleep(opchg_pinctrl_chip);
	rc =opchg_set_switch_mode(NORMAL_CHARGER_MODE);
#else
	gpio_set_value(96, 0);
	ret = gpio_tlmm_config(AP_SWITCH_USB, GPIO_CFG_ENABLE);
	if (ret) {
		pr_info("%s switch usb error %d\n", __func__, ret);
	}
#endif	
	wake_unlock(&bq27541_di->fastchg_wake_lock);
}

#ifdef OPPO_USE_FAST_CHARGER
#if 0
int opchg_bq27541_gpio_pinctrl_init(struct bms_bq27541 *di)
{

    di->pinctrl = devm_pinctrl_get(di->dev);
    if (IS_ERR_OR_NULL(di->pinctrl)) {
            pr_err("%s:%d Getting pinctrl handle failed\n",
            __func__, __LINE__);
         return -EINVAL;
	}

	// set switch1 is active and switch2 is active
	if(get_PCB_Version()== HW_VERSION__10)
	{
	    di->gpio_switch1_act_switch2_act = 
	        pinctrl_lookup_state(di->pinctrl, "switch1_act_switch3_act");
	    if (IS_ERR_OR_NULL(di->gpio_switch1_act_switch2_act)) {
	            pr_err("%s:%d Failed to get the active state pinctrl handle\n",
	            __func__, __LINE__);
	        return -EINVAL;
	    }

		// set switch1 is sleep and switch2 is sleep
	    di->gpio_switch1_sleep_switch2_sleep = 
	        pinctrl_lookup_state(di->pinctrl, "switch1_sleep_switch3_sleep");
	    if (IS_ERR_OR_NULL(di->gpio_switch1_sleep_switch2_sleep)) {
	            pr_err("%s:%d Failed to get the suspend state pinctrl handle\n",
	            __func__, __LINE__);
	        return -EINVAL;
	    }
	}
	else
	{
		di->gpio_switch1_act_switch2_act = 
	        pinctrl_lookup_state(di->pinctrl, "switch1_act_switch2_act");
	    if (IS_ERR_OR_NULL(di->gpio_switch1_act_switch2_act)) {
	            pr_err("%s:%d Failed to get the active state pinctrl handle\n",
	            __func__, __LINE__);
	        return -EINVAL;
	    }

		// set switch1 is sleep and switch2 is sleep
	    di->gpio_switch1_sleep_switch2_sleep = 
	        pinctrl_lookup_state(di->pinctrl, "switch1_sleep_switch2_sleep");
	    if (IS_ERR_OR_NULL(di->gpio_switch1_sleep_switch2_sleep)) {
	            pr_err("%s:%d Failed to get the suspend state pinctrl handle\n",
	            __func__, __LINE__);
	        return -EINVAL;
	    }
	}
	// set switch1 is active and switch2 is sleep
    di->gpio_switch1_act_switch2_sleep = 
        pinctrl_lookup_state(di->pinctrl, "switch1_act_switch2_sleep");
    if (IS_ERR_OR_NULL(di->gpio_switch1_act_switch2_sleep)) {
            pr_err("%s:%d Failed to get the state 2 pinctrl handle\n",
            __func__, __LINE__);
        return -EINVAL;
    }

	// set switch1 is sleep and switch2 is active
    di->gpio_switch1_sleep_switch2_act = 
        pinctrl_lookup_state(di->pinctrl, "switch1_sleep_switch2_act");
    if (IS_ERR_OR_NULL(di->gpio_switch1_sleep_switch2_act)) {
            pr_err("%s:%d Failed to get the state 3 pinctrl handle\n",
            __func__, __LINE__);
        return -EINVAL;
    }

	// set clock is active
	di->gpio_clock_active = 
        pinctrl_lookup_state(di->pinctrl, "clock_active");
    if (IS_ERR_OR_NULL(di->gpio_clock_active)) {
            pr_err("%s:%d Failed to get the state 3 pinctrl handle\n",
            __func__, __LINE__);
        return -EINVAL;
    }
	
	// set clock is sleep
	di->gpio_clock_sleep = 
        pinctrl_lookup_state(di->pinctrl, "clock_sleep");
    if (IS_ERR_OR_NULL(di->gpio_clock_sleep)) {
            pr_err("%s:%d Failed to get the state 3 pinctrl handle\n",
            __func__, __LINE__);
        return -EINVAL;
    }

	// set clock is active
	di->gpio_data_active = 
        pinctrl_lookup_state(di->pinctrl, "data_active");
    if (IS_ERR_OR_NULL(di->gpio_data_active)) {
            pr_err("%s:%d Failed to get the state 3 pinctrl handle\n",
            __func__, __LINE__);
        return -EINVAL;
    }
	
	// set clock is sleep
	di->gpio_data_sleep = 
        pinctrl_lookup_state(di->pinctrl, "data_sleep");
    if (IS_ERR_OR_NULL(di->gpio_data_sleep)) {
            pr_err("%s:%d Failed to get the state 3 pinctrl handle\n",
            __func__, __LINE__);
        return -EINVAL;
    }
	// set reset is atcive
	di->gpio_reset_active = 
        pinctrl_lookup_state(di->pinctrl, "reset_active");
    if (IS_ERR_OR_NULL(di->gpio_reset_active)) {
            pr_err("%s:%d Failed to get the state 3 pinctrl handle\n",
            __func__, __LINE__);
        return -EINVAL;
    }
	// set reset is sleep
	di->gpio_reset_sleep = 
        pinctrl_lookup_state(di->pinctrl, "reset_sleep");
    if (IS_ERR_OR_NULL(di->gpio_reset_sleep)) {
            pr_err("%s:%d Failed to get the state 3 pinctrl handle\n",
            __func__, __LINE__);
        return -EINVAL;
    }
    return 0;
}
#endif

int opchg_set_clock_active()
{
	int rc=0;
	mt_set_gpio_pull_select(OPPO_VOOC_MCU_AP_CLK,GPIO_PULL_DOWN);
	mt_get_gpio_pull_enable(OPPO_VOOC_MCU_AP_CLK);	
	mt_set_gpio_dir(OPPO_VOOC_MCU_AP_CLK, GPIO_DIR_OUT);
	mt_set_gpio_out(OPPO_VOOC_MCU_AP_CLK, 0);
	#if 0
	rc=opchg_set_gpio_dir_output(di->opchg_clock_gpio,0);	// out 0
	rc=pinctrl_select_state(di->pinctrl,di->gpio_clock_sleep);	// PULL_down
	#endif
	return rc;
}
int opchg_set_clock_sleep()
{
	int rc=0;
	
	mt_set_gpio_pull_select(OPPO_VOOC_MCU_AP_CLK,GPIO_PULL_UP);
	mt_get_gpio_pull_enable(OPPO_VOOC_MCU_AP_CLK);	
	mt_set_gpio_dir(OPPO_VOOC_MCU_AP_CLK, GPIO_DIR_OUT);
	mt_set_gpio_out(OPPO_VOOC_MCU_AP_CLK, 1);
	#if 0
		rc=opchg_set_gpio_dir_output(di->opchg_clock_gpio,1);	// out 1
		rc=pinctrl_select_state(di->pinctrl,di->gpio_clock_active);// PULL_up
	#endif
	return rc;
}

int opchg_set_data_active()
{
	int rc=0;
	
	mt_set_gpio_pull_select(OPPO_VOOC_MCU_AP_DATA,GPIO_PULL_DOWN);
	mt_get_gpio_pull_enable(OPPO_VOOC_MCU_AP_DATA);	
	mt_set_gpio_dir(OPPO_VOOC_MCU_AP_DATA, GPIO_DIR_IN);
	#if 0
		rc=opchg_set_gpio_dir_intput(di->opchg_data_gpio);	// in
		rc=pinctrl_select_state(di->pinctrl,di->gpio_data_active);	// no_PULL
	#endif
	//rc=opchg_set_gpio_val(di->opchg_data_gpio,1);	// in
	return rc;
}

int opchg_set_data_sleep()
{
	int rc=0;
	
	mt_set_gpio_pull_select(OPPO_VOOC_MCU_AP_DATA,GPIO_PULL_UP);
	mt_get_gpio_pull_enable(OPPO_VOOC_MCU_AP_DATA);	
	mt_set_gpio_dir(OPPO_VOOC_MCU_AP_DATA, GPIO_DIR_OUT);
	mt_set_gpio_out(OPPO_VOOC_MCU_AP_DATA, 1);
	#if 0
		rc=opchg_set_gpio_dir_output(di->opchg_data_gpio,0);	// out 1
		rc=pinctrl_select_state(di->pinctrl,di->gpio_clock_active);// no_PULL
	#endif
	return rc;
}

#define wait_us(n) udelay(n)
#define wait_ms(n) mdelay(n)
int opchg_set_reset_active()
{
	int rc=0;

	mt_set_gpio_mode(OPPO_VOOC_RESET_MCU_EN, GPIO_MODE_00);	//Set GPIO P9.3 as Output
	mt_set_gpio_dir(OPPO_VOOC_RESET_MCU_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(OPPO_VOOC_RESET_MCU_EN, 1);
	msleep(10);
	mt_set_gpio_out(OPPO_VOOC_RESET_MCU_EN, 0);	
	msleep(10);
	
	return rc;
}

int opchg_set_switch_fast_charger(void)
{
	int rc=0;
	mt_set_gpio_mode(gpio_oppo_vooc_sw_ctrl, GPIO_MODE_00);	//Set GPIO P9.3 as Output
	mt_set_gpio_dir(gpio_oppo_vooc_sw_ctrl, GPIO_DIR_OUT);
	mt_set_gpio_out(gpio_oppo_vooc_sw_ctrl, 1);
	return rc;

}

int opchg_set_switch_normal_charger(void)
{
	int rc=0;

	mt_set_gpio_mode(gpio_oppo_vooc_sw_ctrl, GPIO_MODE_00);	//Set GPIO P9.3 as Output
	mt_set_gpio_dir(gpio_oppo_vooc_sw_ctrl, GPIO_DIR_OUT);
	mt_set_gpio_out(gpio_oppo_vooc_sw_ctrl, 0);
	return rc;
}
int opchg_set_switch_earphone(void)
{
	int rc=0;

	//no this
	return rc;
}

#endif

int opchg_get_prop_authenticate()
{
	return true;
	if (qpnp_batt_gauge && qpnp_batt_gauge->is_battery_authenticated)
		return qpnp_batt_gauge->is_battery_authenticated();
	else {
		pr_err("qpnp-charger no batt gauge assuming false\n");
		return false;
	}
}

#if 0
static void opchg_low_batt_soc_handle(struct opchg_charger *chip)
{	
	static int reduce_count = 0;
	int batt_vol = 0;

	batt_vol = opchg_get_prop_battery_voltage_now(chip)/1000;
	if(batt_vol <= 3300){
        reduce_count++;
		pr_err("%s batt_vol:%d,reduce_count:%d\n",__func__, batt_vol,reduce_count);
        if(reduce_count >= 3){
            if(chip->bat_volt_check_point > 2)
				chip->bat_volt_check_point--;
			else
				chip->bat_volt_check_point = 0;
			reduce_count = 0;
        }   
    } else {
		reduce_count = 0;
    }
}
#endif

#if 0
static bool opchg_soc_reduce_slowly_when_1(struct opchg_charger *chip,int soc_init)
{
	static int reduce_count = 0;
	int batt_vol = 0;
	
	if(soc_init == 0)
		return 1;
	batt_vol = opchg_get_prop_battery_voltage_now(chip)/1000;
	if(batt_vol < 3400)
		reduce_count++;
	else 
		reduce_count = 0;
	pr_err("%s batt_vol:%d,reduce_count:%d\n",__func__,batt_vol,reduce_count);
	if(reduce_count < 5)
		return 0;
	else {
		reduce_count = 5;
		return 1;
	}
}

int opchg_get_prop_batt_capacity_from_bms(struct opchg_charger *chip)
{
	static int soc_init = 0;
	static char is_pon_on = 0;
	static char sync_count = 0;
	union power_supply_propval ret = {0, }; 
	
	if(!chip->bms_psy){
		return OPCHG_DEFAULT_BATT_CAPACITY;
	} else if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,POWER_SUPPLY_PROP_CAPACITY, &ret);		
	}
	chip->soc_bms = ret.intval;
		
	return chip->bat_volt_check_point;
}
#endif	

int opchg_get_prop_bq24196_full_status()
{

	return bq27541_get_bq24196_full_status();
}


int opchg_get_prop_battery_voltage_now()
{
	int rc = 0;
	int V_battery = 0;


	if (qpnp_batt_gauge && qpnp_batt_gauge->get_battery_mvolts)
		V_battery =qpnp_batt_gauge->get_battery_mvolts();
	else {
		pr_err("qpnp-charger no batt gauge assuming 3.5V\n");
		V_battery =3500*1000;
	}
	
	return V_battery;
}

int opchg_get_prop_current_now()
{
	int rc = 0;
	int chg_current = 0;

	
	if (qpnp_batt_gauge && qpnp_batt_gauge->get_average_current)
		chg_current = qpnp_batt_gauge->get_average_current();
	else {
		pr_err("qpnp-charger no batt gauge assuming 0mA\n");
		chg_current = 0;
	}

	return chg_current;
}


int opchg_get_prop_batt_capacity()
{
	int soc;


	if (qpnp_batt_gauge && qpnp_batt_gauge->get_battery_soc){
		soc = qpnp_batt_gauge->get_battery_soc();
	} else {
		pr_err("qpnp-charger no batt gauge assuming 50percent\n");
		soc = 50;
	}

	
	return soc;
}

bool opchg_get_prop_batt_present()
{
	return check_batt_exist();
}

int opchg_get_prop_batt_temp()
{
	int rc = 0;
	int T_battery = 0;
	
	if (qpnp_batt_gauge && qpnp_batt_gauge->get_battery_temperature) {
			T_battery =qpnp_batt_gauge->get_battery_temperature();
	} else {
			pr_err("qpnp-charger no batt gauge assuming 35 deg G\n");
			T_battery = -400;
	}
	return T_battery;
}


int opchg_set_switch_mode(u8 mode)
{
	int rc=0;

	
	// check GPIO23 and GPIO38 is  undeclared, prevent the  Invalid use for earphone
	if(opchg_pinctrl_chip == NULL)
	{
		pr_err("%s GPIO23 and GPIO38 is no probe\n",__func__);
		return -1;
	}

	// GPIO23 and GPIO38 is  declared
    switch(mode) {
        case VOOC_CHARGER_MODE:	//11
				rc=opchg_set_switch_fast_charger();
				//opchg_gpio.opchg_fastcharger= 1;
				pr_err("%s charge_mode,rc:%d,gpio_oppo_vooc_sw_ctrl:%d\n",__func__,rc,mt_get_gpio_in(gpio_oppo_vooc_sw_ctrl));
			break;
            
        case HEADPHONE_MODE:		//10
				rc=opchg_set_switch_earphone();
				//opchg_gpio.opchg_fastcharger= 2;
				pr_err("%s headphone mode,do nothing rc:%d,gpio_oppo_vooc_sw_ctrl:%d\n",__func__,rc,mt_get_gpio_in(gpio_oppo_vooc_sw_ctrl));
			break;
            
        case NORMAL_CHARGER_MODE:	//01
        default:
				rc=opchg_set_switch_normal_charger();
				//opchg_gpio.opchg_fastcharger= 0;
				pr_err("%s normal mode,rc:%d,,gpio_oppo_vooc_sw_ctrl:%d\n",__func__,rc,mt_get_gpio_in(gpio_oppo_vooc_sw_ctrl));
			break;
    }
	
	return rc;
}

void KernelVendorFastchgLog(void)
{
	if(!bq27541_di) {
		return;
	}
	else {
		printk("[VENDOR FASTCHGVAL] soc_pre = %d, temp_pre=%d, batt_vol_pre = %d, current_pre = %d, alow_reading = %d,fast_chg_ing = %d,fcc_pre = %d,soh_pre = %d,cc_pre = %d\n",
		bq27541_di->soc_pre, bq27541_di->temp_pre, bq27541_di->batt_vol_pre, bq27541_di->current_pre, bq27541_di->alow_reading, bq27541_di->fast_chg_ing, bq27541_di->fcc_pre, bq27541_di->soh_pre, bq27541_di->cc_pre);
		printk("[VENDOR FASTCHGSTATUS] fast_chg_started = %d, fast_switch_to_normal=%d, fast_normal_to_warm = %d, fast_chg_allow = %d,fast_low_temp_full = %d, suspended = %d\n",
		bq27541_di->fast_chg_started, bq27541_di->fast_switch_to_normal, bq27541_di->fast_normal_to_warm, bq27541_di->fast_chg_allow, bq27541_di->fast_low_temp_full, atomic_read(&bq27541_di->suspended));
	}
}

void bq27541_power(int on)
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

#define CONTROL_CMD				0x00
#define CONTROL_STATUS				0x00
#define SEAL_POLLING_RETRY_LIMIT	100
//#define BQ27541_UNSEAL_KEY			11151986
#define BQ27541_UNSEAL_KEY			0x11151986
#define BQ27411_UNSEAL_KEY			0x80008000

#define BQ27541_RESET_SUBCMD		0x0041
#define BQ27411_RESET_SUBCMD		0x0042
#define SEAL_SUBCMD					0x0020

extern int bat_volt_check_point;

static int sealed()
{
	//return control_cmd_read(di, CONTROL_STATUS) & (1 << 13);
	int value = 0;
	
	bq27541_cntl_cmd(CONTROL_STATUS);
	//bq27541_cntl_cmd(di,CONTROL_STATUS);
	msleep(10);
	bq27541_read_i2c(CONTROL_STATUS, &value);
	pr_err("%s REG_CNTL: 0x%x\n", __func__, value);

	if (bq27541_di->device_type == DEVICE_BQ27541)
		return value & BIT(14);
	else if (bq27541_di->device_type == DEVICE_BQ27411)
		return value & BIT(13);
	else 
		return 1;
}

static int seal(void)
{
	int i = 0;

	if(sealed()){
		printk("bq27541/27411 sealed,return\n");
		return 1;
	}
	bq27541_cntl_cmd(SEAL_SUBCMD);
	msleep(10);
	for(i = 0;i < SEAL_POLLING_RETRY_LIMIT;i++){
		if (sealed())
			return 1;
		msleep(10);
	}
	return 0;
}


static int unseal(u32 key)
{
	int i = 0;

	if (!sealed())
		goto out;
	
	if(bq27541_di->device_type == DEVICE_BQ27541){
		//bq27541_write(CONTROL_CMD, key & 0xFFFF, false, di);
		bq27541_cntl_cmd(0x1115);
		msleep(10);
		//bq27541_write(CONTROL_CMD, (key & 0xFFFF0000) >> 16, false, di);
		bq27541_cntl_cmd(0x1986);
		msleep(10);
	} 
	else if(bq27541_di->device_type == DEVICE_BQ27411){
	//bq27541_write(CONTROL_CMD, key & 0xFFFF, false, di);
		bq27541_cntl_cmd(0x8000);
		msleep(10);
		//bq27541_write(CONTROL_CMD, (key & 0xFFFF0000) >> 16, false, di);
		bq27541_cntl_cmd(0x8000);
		msleep(10);
	}
	bq27541_cntl_cmd(0xffff);
	msleep(10);
	bq27541_cntl_cmd(0xffff);
	msleep(10);

	while (i < SEAL_POLLING_RETRY_LIMIT) {
		i++;
		if (!sealed())
			break;
		msleep(10);
	}

out:
	printk(KERN_ERR "bq27541 %s: i=%d\n", __FUNCTION__, i);

	if ( i == SEAL_POLLING_RETRY_LIMIT) {
		printk(KERN_ERR "bq27541 %s failed\n", __FUNCTION__);
		return 0;
	} else {
		return 1;
	}
}


static void bq27541_reset(struct i2c_client *client)
{
	struct bq27541_device_info *di = i2c_get_clientdata(client);

	if (opchg_get_prop_battery_voltage_now() <= 3300 * 1000 
			&& opchg_get_prop_battery_voltage_now() > 2500 * 1000
			&& bat_volt_check_point == 0 
			&& opchg_get_prop_batt_temp() > 150) {
		if (!unseal(BQ27541_UNSEAL_KEY)) {
			printk(KERN_ERR "bq27541 unseal fail !\n");
			return;
		}
		printk("bq27541 unseal OK vol = %d,point = %d,temp = %d!\n",opchg_get_prop_battery_voltage_now(),bat_volt_check_point,opchg_get_prop_batt_temp());
		
		if (bq27541_di->device_type == DEVICE_BQ27541)
			bq27541_cntl_cmd(BQ27541_RESET_SUBCMD);
		else if (bq27541_di->device_type == DEVICE_BQ27411)
			bq27541_cntl_cmd(BQ27411_RESET_SUBCMD);//27411
		msleep(50);

		if (bq27541_di->device_type == DEVICE_BQ27411){
			if (!seal())
				printk("bq27411 seal fail\n");
		}
		msleep(150);
		printk("bq27541_reset,point = %d\r\n",opchg_get_prop_batt_capacity());
	}
	return;
}

static int bq27541_driver_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{         
    strcpy(info->type, "bq27541_i2c");                                                         

	printk("[bq27541_driver_detect] \n");
	
    return 0;                                                                                       
}

#define MAX_RETRY_COUNT	5
static int bq27541_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
	struct bms_bq27541 *di;
	struct class_device *class_dev = NULL;
    int err=0,retval=0; 
	
	if(chargin_hw_init_done_vooc == KAL_FALSE)
	{
		printk("MCU vooc isn't ok,please wait\r\n");
		return -EPROBE_DEFER;
	}
    printk("[bq27541_driver_probe] \n");
		
	if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }	
    memset(new_client, 0, sizeof(struct i2c_client));
	new_client = client;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto exit_kfree;
	}
	
	i2c_set_clientdata(client, di);	
	
    bq27541_power(1);
	di->dev = &client->dev;
	di->client = client;
	di->temp_pre = 0;
	di->alow_reading = true;
	di->fast_chg_ing = false;
	di->fast_low_temp_full = false;
	di->retry_count = MAX_RETRY_COUNT;
	atomic_set(&di->suspended, 0);
	opchg_pinctrl_chip =di;
	
	bq27541_di = di;
	//INIT_WORK(&di->counter, bq27541_coulomb_counter_work);
	INIT_DELAYED_WORK(&di->hw_config, bq27541_hw_config);
	schedule_delayed_work(&di->hw_config, 0);
	
	
	init_timer(&di->watchdog);
	di->watchdog.data = (unsigned long)di;
	di->watchdog.function = di_watchdog;
	wake_lock_init(&di->fastchg_wake_lock,		
		WAKE_LOCK_SUSPEND, "fastcg_wake_lock");
	INIT_WORK(&di->fastcg_work,fastcg_work_func);
	
#if 	1
	retval= opchg_set_data_active();
	//di->irq = gpio_to_irq(OPPO_VOOC_MCU_AP_DATA);
	//retval = request_irq(di->irq, irq_rx_handler, IRQF_TRIGGER_RISING, "mcu_data", di);	//0X01:rising edge,0x02:falling edge
	mt_set_gpio_mode(OPPO_VOOC_MCU_AP_DATA, GPIO_MODE_00);
	mt_set_gpio_dir(OPPO_VOOC_MCU_AP_DATA,GPIO_DIR_IN);
	mt_set_gpio_pull_enable(OPPO_VOOC_MCU_AP_DATA, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(OPPO_VOOC_MCU_AP_DATA, GPIO_PULL_UP);
    mt_eint_registration(CUST_EINT_MCU_AP_DATA, IRQF_TRIGGER_RISING, irq_rx_handler, 0);
	mt_eint_unmask(CUST_EINT_MCU_AP_DATA);
	
#else
	gpio_request(1, "mcu_clk");
	gpio_tlmm_config(AP_RX_EN,1);
	gpio_direction_input(1);
	di->irq = gpio_to_irq(1);
	retval = request_irq(di->irq, irq_rx_handler, IRQF_TRIGGER_RISING, "mcu_data", di);	//0X01:rising edge,0x02:falling edge
	if(retval < 0) {
		pr_err("%s request ap rx irq failed.\n", __func__);
	}
#endif	
	
	fastchg_wq = create_singlethread_workqueue("fastchg_wq");
    if (!fastchg_wq) {
		printk("[fastchg_wq] success\n");
        return -ENOMEM;
    } 
	chargin_hw_init_done_bq27541 = 1;
	printk("[bq27541_driver_probe] success\n");
    return 0;                                                                                       

exit_kfree:
    kfree(new_client);
    bq27541_power(0);
exit:
    return err;

}
/**********************************************************
  *
  *   [platform_driver API] 
  *
  *********************************************************/
static __init  bq27541_subsys_init(void)
{	
	int ret=0;
	
	printk("[bq27541_init] init start\n");
	i2c_register_board_info(2, &i2c_bq27541, 1);
	if(i2c_add_driver(&bq27541_i2c_driver)!=0)
	{
		printk("[bq27541_init] failed to register bq27541 i2c driver.\n");
	}
	else
	{
		printk("[bq27541_init] Success to register bq27541 i2c driver.\n");
	}

	return 0;		
}
/*----------------------------------------------------------------------------*/
static void  bq27541_exit(void)
{
	i2c_del_driver(&bq27541_i2c_driver);
}
/*----------------------------------------------------------------------------*/
/*
static int bq27541_platform_driver_probe(struct platform_device *pdev) 
{
    int ret=0;
    printk("%s\n",__func__);
	ret = bq27541_init();
	if(ret)
	{
	    printk("bq27541_init error!\n");
	}

	return 0;
	
}
static int bq27541_platform_driver_remove(struct platform_device *pdev)
{
    int ret=0;
    printk("%s\n",__func__);
	struct bms_bq27541 *di = i2c_get_clientdata(new_client);

	qpnp_battery_gauge_unregister(&bq27541_batt_gauge);
	bq27541_cntl_cmd(BQ27541_SUBCMD_DISABLE_DLOG);
	udelay(66);
	bq27541_cntl_cmd(BQ27541_SUBCMD_DISABLE_IT);
	cancel_delayed_work_sync(&di->hw_config);

	kfree(di);
	return 0;
	
	bq27541_exit();

	return 0;
}

#if 0
extern int msmrtc_alarm_read_time(struct rtc_time *tm);
#endif
static int bq27541_platform_suspend(struct platform_device *dev, pm_message_t state)	
{
	
	int ret=0;
	//struct rtc_time	rtc_suspend_rtc_time;
	bq27541_power(0);
	atomic_set(&bq27541_di->suspended, 1);
#if 0
	ret = msmrtc_alarm_read_time(&rtc_suspend_rtc_time);

	if (ret < 0) {
		pr_err("%s: Failed to read RTC time\n", __func__);
		return 0;
	}
	rtc_tm_to_time(&rtc_suspend_rtc_time, &bq27541_di->rtc_suspend_time);
#endif
    printk("%s,power down i2c\n",__func__);
    return 0;
}


static int bq27541_platform_resume(struct platform_device *dev, pm_message_t state)	
{
	int ret=0;
	//struct rtc_time	rtc_resume_rtc_time;
			
	atomic_set(&bq27541_di->suspended, 0);
#if 0
	ret = msmrtc_alarm_read_time(&rtc_resume_rtc_time);

	if (ret < 0) {
		pr_err("%s: Failed to read RTC time\n", __func__);
		return 0;
	}
	rtc_tm_to_time(&rtc_resume_rtc_time, &bq27541_di->rtc_resume_time);
	
	if((bq27541_di->rtc_resume_time - bq27541_di->rtc_suspend_time)>= RESUME_TIME){
		//update pre capacity when sleep time more than 1minutes
		bq27541_battery_soc(true); 
	}
#endif	
	bq27541_power(1);
    printk("%s,power on i2c\n",__func__);
    return 0;
}

static struct platform_driver bq27541_platform_driver = {
	.probe      = bq27541_platform_driver_probe,
	.remove     = bq27541_platform_driver_remove,  
	.suspend    = bq27541_platform_suspend,
	.resume     = bq27541_platform_resume,
	.driver     = {
		.name  = "bq27541",
		.owner = THIS_MODULE,
	}
};


static int __init bq27541_platform_driver_init(void)
{
	printk("%s\n",__func__);
	i2c_register_board_info(1, &i2c_bq27541, 1);
    
	if(platform_driver_register(&bq27541_platform_driver))
	{
		printk("%s failed to register driver",__func__);
		return -ENODEV;
	}
	return 0;    
}

static void __exit bq27541_platform_driver_deinit(void)
{
	printk("%s\n",__func__);
	platform_driver_unregister(&bq27541_platform_driver);

}
*/
/*----------------------------------------------------------------------------*/

/*
module_init(bq27541_platform_driver_init);
module_exit(bq27541_platform_driver_deinit);
*/

subsys_initcall(bq27541_subsys_init);

//module_init(bq27541_init);
//module_exit(bq27541_exit);
MODULE_DESCRIPTION("Driver for bq27541 charger chip");
MODULE_LICENSE("GPL v2");

