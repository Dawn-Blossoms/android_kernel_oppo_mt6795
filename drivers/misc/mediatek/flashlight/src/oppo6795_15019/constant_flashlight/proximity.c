/*
 *  stmvl6180.c - Linux kernel module for STM VL6180 FlightSense Time-of-Flight
 *
 *  Copyright (C) 2014 STMicroelectronics Imaging Division.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
//#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "kd_camera_hw.h"
#include "proximity.h"

 #define PROXIMITY_DEBUG 
#ifdef PROXIMITY_DEBUG
#define PROXIMITYDB(fmt, args...)	printk(fmt, ##args)
#else
#define PROXIMITYDB(x, ...)
#endif

//distance filter
#define DISTANCE_FILTER 
static struct stmvl6180_data st_vl6180_data;
struct stmvl6180_data *vl6180_data = &st_vl6180_data;
#define DEFAULT_CROSSTALK	 0//4 
static DEFINE_SPINLOCK(g_VL6180Lock);	/* for SMP */
#define PROXIMITY_I2C_BUSNUM 0 // 1
#define PRO_HW_NAME 			"kd_pro_vl6180"
static struct i2c_board_info kd_proximity_dev __initdata =
    { I2C_BOARD_INFO(PRO_HW_NAME, 0x29) };
static const unsigned short normal_i2c[] = { 0x29,I2C_CLIENT_END };
/*******************************************************************************
*
*
********************************************************************************/
#define PROXIMITY_I2C_GROUP_ID 0
/*******************************************************************************
*
********************************************************************************/
static struct i2c_client *g_pstI2Cclient;

/* 81 is used for V4L driver */
static dev_t g_PROXIMITYdevno; // = MKDEV(PROXIMITY_DEV_MAJOR_NUMBER, 0);
static struct cdev *g_pPROXIMITY_CharDrv;
/* static spinlock_t g_VL6180Lock; */
/* spin_lock(&g_VL6180Lock); */
/* spin_unlock(&g_VL6180Lock); */

static struct class *PROXIMITY_class;
//static atomic_t g_PROXIMITYatomic;
/* static DEFINE_SPINLOCK(kdeeprom_drv_lock); */
/* spin_lock(&kdeeprom_drv_lock); */
/* spin_unlock(&kdeeprom_drv_lock); */
#ifdef DISTANCE_FILTER
void VL6180_InitDistanceFilter(void);
uint16_t VL6180_DistanceFilter(struct stmvl6180_data *vl6180_data, 
	uint16_t m_trueRange_mm, uint16_t m_rawRange_mm, 
	uint32_t m_rtnSignalRate, uint32_t m_rtnAmbientRate, uint16_t errorCode);
uint32_t VL6180_StdDevDamper(uint32_t AmbientRate, uint32_t SignalRate, 
	uint32_t StdDevLimitLowLight, uint32_t StdDevLimitLowLightSNR, 
	uint32_t StdDevLimitHighLight, uint32_t StdDevLimitHighLightSNR);
#endif
/*******************************************************************************
*
********************************************************************************/
/* maximun read length is limited at "I2C_FIFO_SIZE" in I2c-mt65xx.c which is 8 bytes */
int iWritePROXIMITY(u16 a_u2Addr, u32 a_u4Bytes, u8 *puDataInBytes)
{
	int i4RetValue = 0;
	u32 u4Index = 0;
	char puSendCmd[8] = { (char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF),
		0, 0, 0, 0, 0, 0
	};
	if (a_u4Bytes + 2 > 8) {
		printk("[stmvl6180] exceed I2c-mt65xx.c 8 bytes limitation (include address 2 Byte)\n");
		return -1;
	}

	for (u4Index = 0; u4Index < a_u4Bytes; u4Index += 1) {
		puSendCmd[(u4Index + 2)] = puDataInBytes[u4Index];
	}

	i4RetValue = i2c_master_send(g_pstI2Cclient, puSendCmd, (a_u4Bytes + 2));
	if (i4RetValue != (a_u4Bytes + 2)) {
		printk("[stmvl6180] I2C write  failed!!\n");
		return -1;
	}
	mdelay(10);		/* for tWR singnal --> write data form buffer to memory. */

	/* PROXIMITYDB("[EEPROM] iWriteEEPROM done!!\n"); */
	return 0;
}


/* maximun read length is limited at "I2C_FIFO_SIZE" in I2c-mt65xx.c which is 8 bytes */
int iReadPROXIMITY(u16 a_u2Addr, u32 ui4_length, u8 *a_puBuff)
{
	int i4RetValue = 0;
	char puReadCmd[2] = { (char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF) };

	/* PROXIMITYDB("[EEPROM] iReadEEPROM!!\n"); */

	if (ui4_length > 8) {
		PROXIMITYDB("[stmvl6180] exceed I2c-mt65xx.c 8 bytes limitation\n");
		return -1;
	}
	spin_lock(&g_VL6180Lock);	/* for SMP */
	g_pstI2Cclient->addr = g_pstI2Cclient->addr & (I2C_MASK_FLAG | I2C_WR_FLAG);
	spin_unlock(&g_VL6180Lock);	/* for SMP */

	/* PROXIMITYDB("[EEPROM] i2c_master_send\n"); */
	i4RetValue = i2c_master_send(g_pstI2Cclient, puReadCmd, 2);
	if (i4RetValue != 2) {
		PROXIMITYDB("[stmvl6180] I2C send read address failed!!\n");
		return -1;
	}
	/* PROXIMITYDB("[EEPROM] i2c_master_recv\n"); */
	i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)a_puBuff, ui4_length);
	if (i4RetValue != ui4_length) {
		PROXIMITYDB("[stmvl6180] I2C read data failed!!\n");
		return -1;
	}
	spin_lock(&g_VL6180Lock);	/* for SMP */
	g_pstI2Cclient->addr = g_pstI2Cclient->addr & I2C_MASK_FLAG;
	spin_unlock(&g_VL6180Lock);	/* for SMP */

	return 0;
}


static int iWriteData(unsigned int ui4_offset, unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;
	int i4ResidueDataLength;
	u32 u4IncOffset = 0;
	u32 u4CurrentOffset;
	u8 *pBuff;

	PROXIMITYDB("[S24EEPROM] iWriteData\n");

	if (ui4_offset + ui4_length >= 0x2000) {
		PROXIMITYDB("[stmvl6180] Write Error!! S-stmvl6180 not supprt address >= 0x2000!!\n");
		return -1;
	}

	i4ResidueDataLength = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;

	PROXIMITYDB("[stmvl6180] iWriteData u4CurrentOffset is %d\n", u4CurrentOffset);

	do {
		if (i4ResidueDataLength >= 6) {
			i4RetValue = iWritePROXIMITY((u16) u4CurrentOffset, 6, pBuff);
			if (i4RetValue != 0) {
				PROXIMITYDB("[EEPROM] I2C iWriteData failed!!\n");
				return -1;
			}
			u4IncOffset += 6;
			i4ResidueDataLength -= 6;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
		} else {
			i4RetValue =
			    iWritePROXIMITY((u16) u4CurrentOffset, i4ResidueDataLength, pBuff);
			if (i4RetValue != 0) {
				PROXIMITYDB("[EEPROM] I2C iWriteData failed!!\n");
				return -1;
			}
			u4IncOffset += 6;
			i4ResidueDataLength -= 6;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
			/* break; */
		}
	} while (i4ResidueDataLength > 0);
	PROXIMITYDB("[stmvl6180] iWriteData done\n");

	return 0;
}

/* int iReadData(stPROXIMITY_INFO_STRUCT * st_pOutputBuffer) */
static int iReadData(unsigned int ui4_offset, unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;
	int i4ResidueDataLength;
	u32 u4IncOffset = 0;
	u32 u4CurrentOffset;
	u8 *pBuff;
/* PROXIMITYDB("[S24EEPORM] iReadData\n" ); */

	if (ui4_offset + ui4_length >= 0x2000) {
		PROXIMITYDB("[stmvl6180] Read Error!! S-stmvl6180 not supprt address >= 0x2000!!\n");
		return -1;
	}

	i4ResidueDataLength = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;
	do {
		if (i4ResidueDataLength >= 8) {
			i4RetValue = iReadPROXIMITY((u16) u4CurrentOffset, 8, pBuff);
			if (i4RetValue != 0) {
				PROXIMITYDB("[stmvl6180] I2C iReadData failed!!\n");
				return -1;
			}
			u4IncOffset += 8;
			i4ResidueDataLength -= 8;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
		} else {
			i4RetValue = iReadPROXIMITY((u16) u4CurrentOffset, i4ResidueDataLength, pBuff);
			if (i4RetValue != 0) {
				PROXIMITYDB("[stmvl6180] I2C iReadData failed!!\n");
				return -1;
			}
			u4IncOffset += 8;
			i4ResidueDataLength -= 8;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
			/* break; */
		}
	} while (i4ResidueDataLength > 0);
/* PROXIMITYDB("[S24EEPORM] iReadData finial address is %d length is %d buffer address is 0x%x\n",u4CurrentOffset, i4ResidueDataLength, pBuff); */
/* PROXIMITYDB("[S24EEPORM] iReadData done\n" ); */
	return 0;
}
// 8 bits cci read
int vl6180_i2c_read_8bits(unsigned int addr,  uint16_t *pdata)
{
	uint16_t tmp=0;
	int rc = 0;

	rc = iReadPROXIMITY(addr,1,&tmp);

	*pdata = (uint16_t)tmp;
	return rc;
}
// 8 bits cci write 
int vl6180_i2c_write_8bits(uint32_t addr,  uint16_t data)
{	
	int rc = 0;
	rc = iWritePROXIMITY(addr, 1,&data);

	return rc;
}
// 32 bits cci read
int vl6180_i2c_read_32bits(unsigned int addr,  unsigned int *pdata)
{	
	unsigned char read_buffer[4] = { 0, 0, 0, 0 };
	int rc = 0;
	
	rc = iReadPROXIMITY(addr,4,read_buffer);

	*pdata = (unsigned int)( (unsigned int)(read_buffer[0] <<24)
				 | (unsigned int)((read_buffer[1])<<16)
				 | (unsigned int)((read_buffer[2])<<8)
				 | (unsigned int)(read_buffer[3]) );

	return rc;
} 
int stmvl6180_power_enable(unsigned int enable)
{
	int rc = 0;

	printk("%s %d\n",__func__, enable);
	
	if(enable) {
		if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800, "stmvl6180")) {
			PROXIMITYDB("[stmvl6180] Fail to enable analog gain\n");
			return -EIO;
		}
		msleep(10);
		if(mt_set_gpio_mode(GPIO121,GPIO_MODE_00)){PROXIMITYDB("[CAMERA stmvl6180] set gpio mode failed!! \n");}
		if(mt_set_gpio_dir(GPIO121,GPIO_DIR_OUT)){PROXIMITYDB("[CAMERA stmvl6180] set gpio dir failed!! \n");}
		if(mt_set_gpio_out(GPIO121,GPIO_OUT_ONE)){PROXIMITYDB("[CAMERA stmvl6180] set gpio failed!! \n");}
		mdelay(3);
		if(mt_set_gpio_mode(GPIO124,GPIO_MODE_00)){PROXIMITYDB("[CAMERA stmvl6180] set gpio mode failed!! \n");}
		if(mt_set_gpio_dir(GPIO124,GPIO_DIR_OUT)){PROXIMITYDB("[CAMERA stmvl6180] set gpio dir failed!! \n");}
		if(mt_set_gpio_out(GPIO124,GPIO_OUT_ONE)){PROXIMITYDB("[CAMERA stmvl6180] set gpio failed!! \n");}
		mdelay(3);
	} else {
		if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_AF,"stmvl6180")) {
			PROXIMITYDB("[stmvl6180] Fail to disable analog gain\n");
			return -EIO;
		}
		mdelay(3);
		if(mt_set_gpio_mode(GPIO121,GPIO_MODE_00)){PROXIMITYDB("[CAMERA stmvl6180] set gpio mode failed!! \n");}
		if(mt_set_gpio_dir(GPIO121,GPIO_DIR_OUT)){PROXIMITYDB("[CAMERA stmvl6180] set gpio dir failed!! \n");}
		if(mt_set_gpio_out(GPIO121,GPIO_OUT_ZERO)){PROXIMITYDB("[CAMERA stmvl6180] set gpio failed!! \n");}
		msleep(10);
		if(mt_set_gpio_mode(GPIO124,GPIO_MODE_00)){PROXIMITYDB("[CAMERA stmvl6180] set gpio mode failed!! \n");}
        if(mt_set_gpio_dir(GPIO124,GPIO_DIR_OUT)){PROXIMITYDB("[CAMERA stmvl6180] set gpio dir failed!! \n");}
        if(mt_set_gpio_out(GPIO124,GPIO_OUT_ZERO)){PROXIMITYDB("[CAMERA stmvl6180] set gpio failed!! \n");}
        mdelay(3);
	}
	return 0;
}
int vl6180_init(struct stmvl6180_data *vl6180_data)
{
	int rc = 0;
	int i;
	int8_t offsetByte;
	uint16_t modelID = 0;
	uint16_t revID = 0;
	int8_t rangeTemp = 0;
	uint16_t chipidRange = 0;
	uint16_t CrosstalkHeight;
	uint16_t IgnoreThreshold;
	uint16_t IgnoreThresholdHeight;
	uint16_t dataByte;
	uint16_t ambpart2partCalib1 = 0;
	uint16_t ambpart2partCalib2 = 0;
#ifdef USE_INTERRUPTS
	uint16_t chipidgpio = 0;
#endif
	pr_err("vl6180_init ENTER!\n");

	stmvl6180_power_enable(1);

	vl6180_i2c_read_8bits(IDENTIFICATION__MODEL_ID, &modelID);
	vl6180_i2c_read_8bits(IDENTIFICATION__REVISION_ID, &revID);
	pr_err("Model ID : 0x%X, REVISION ID : 0x%X\n", modelID, revID);

	//waitForStandby
	for(i=0; i<100; i++) {
		vl6180_i2c_read_8bits(FIRMWARE__BOOTUP, &modelID);
		if( (modelID & 0x01) == 1 ) {
			i=100;
		}
	}

	//range device ready
	for(i=0; i<100; i++) {
		vl6180_i2c_read_8bits(RESULT__RANGE_STATUS, &modelID);
		if( (modelID & 0x01) == 1) {
			i = 100;
		}
	}
	vl6180_i2c_write_8bits(0x0207, 0x01);
	vl6180_i2c_write_8bits(0x0208, 0x01);
	vl6180_i2c_write_8bits(0x0133, 0x01);
	vl6180_i2c_write_8bits(0x0096, 0x00);
	vl6180_i2c_write_8bits(0x0097, 0x54);
	vl6180_i2c_write_8bits(0x00e3, 0x00);
	vl6180_i2c_write_8bits(0x00e4, 0x04);
	vl6180_i2c_write_8bits(0x00e5, 0x02);
	vl6180_i2c_write_8bits(0x00e6, 0x01);
	vl6180_i2c_write_8bits(0x00e7, 0x03);
	vl6180_i2c_write_8bits(0x00f5, 0x02);
	vl6180_i2c_write_8bits(0x00D9, 0x05);
	// AMB P2P calibration
	vl6180_i2c_read_8bits(SYSTEM__FRESH_OUT_OF_RESET, &dataByte);
	if(dataByte==0x01) {
		vl6180_i2c_read_8bits(0x26, &dataByte);
		ambpart2partCalib1 = dataByte<<8;
		vl6180_i2c_read_8bits(0x27, &dataByte);
		ambpart2partCalib1 = ambpart2partCalib1 + dataByte;
		vl6180_i2c_read_8bits(0x28, &dataByte);
		ambpart2partCalib2 = dataByte<<8;
		vl6180_i2c_read_8bits(0x29, &dataByte);
		ambpart2partCalib2 = ambpart2partCalib2 + dataByte;
		if(ambpart2partCalib1!=0) {
			// p2p calibrated
			vl6180_i2c_write_8bits(0xDA, (ambpart2partCalib1>>8)&0xFF);
			vl6180_i2c_write_8bits(0xDB, ambpart2partCalib1&0xFF);
			vl6180_i2c_write_8bits(0xDC, (ambpart2partCalib2>>8)&0xFF);
			vl6180_i2c_write_8bits(0xDD, ambpart2partCalib2&0xFF);
		} else {
			// No p2p Calibration, use default settings
			vl6180_i2c_write_8bits(0xDB, 0xCE);
			vl6180_i2c_write_8bits(0xDC, 0x03);
			vl6180_i2c_write_8bits(0xDD, 0xF8);
		}
	}
	vl6180_i2c_write_8bits(0x009f, 0x00);
	vl6180_i2c_write_8bits(0x00a3, 0x28);
	vl6180_i2c_write_8bits(0x00b7, 0x00);
	vl6180_i2c_write_8bits(0x00bb, 0x28);
	vl6180_i2c_write_8bits(0x00b2, 0x09);
	vl6180_i2c_write_8bits(0x00ca, 0x09);
	vl6180_i2c_write_8bits(0x0198, 0x01);
	vl6180_i2c_write_8bits(0x01b0, 0x17);
	vl6180_i2c_write_8bits(0x01ad, 0x00);
	vl6180_i2c_write_8bits(0x00FF, 0x05);
	vl6180_i2c_write_8bits(0x0100, 0x05);
	vl6180_i2c_write_8bits(0x0199, 0x05);
	vl6180_i2c_write_8bits(0x0109, 0x07);
	vl6180_i2c_write_8bits(0x010a, 0x30);
	vl6180_i2c_write_8bits(0x003f, 0x46);
	vl6180_i2c_write_8bits(0x01a6, 0x1b);
	vl6180_i2c_write_8bits(0x01ac, 0x3e);
	vl6180_i2c_write_8bits(0x01a7, 0x1f);
	vl6180_i2c_write_8bits(0x0103, 0x01);
	vl6180_i2c_write_8bits(0x0030, 0x00);
	vl6180_i2c_write_8bits(0x001b, 0x0A);
	vl6180_i2c_write_8bits(0x003e, 0x0A);
	vl6180_i2c_write_8bits(0x0131, 0x04);
	vl6180_i2c_write_8bits(0x0011, 0x10);
	vl6180_i2c_write_8bits(0x0014, 0x24);
	vl6180_i2c_write_8bits(0x0031, 0xFF);
	vl6180_i2c_write_8bits(0x00d2, 0x01);
	vl6180_i2c_write_8bits(0x00f2, 0x01);

	// RangeSetMaxConvergenceTime
	vl6180_i2c_write_8bits(SYSRANGE__MAX_CONVERGENCE_TIME, 0x3F);
	vl6180_i2c_write_8bits(SYSRANGE__MAX_AMBIENT_LEVEL_MULT, 0xFF);//SNR
	
	vl6180_i2c_read_8bits(SYSTEM__FRESH_OUT_OF_RESET, &dataByte);
	if(dataByte==0x01) {
		//readRangeOffset
		vl6180_i2c_read_8bits(SYSRANGE__PART_TO_PART_RANGE_OFFSET, &dataByte);
		rangeTemp = (int8_t)dataByte;
		if(dataByte > 0x7F) {
			rangeTemp -= 0xFF;
		}
		rangeTemp /= 3;
		rangeTemp = rangeTemp +1; //roundg
		//Range_Set_Offset
		offsetByte = *((u8*)(&rangeTemp)); // round
		vl6180_i2c_write_8bits(SYSRANGE__PART_TO_PART_RANGE_OFFSET,(u8)offsetByte);
	}
	
	// ClearSystemFreshOutofReset
	vl6180_i2c_write_8bits(SYSTEM__FRESH_OUT_OF_RESET, 0x0);
	// VL6180 CrossTalk
	vl6180_i2c_write_8bits(SYSRANGE__CROSSTALK_COMPENSATION_RATE,
										(DEFAULT_CROSSTALK>>8)&0xFF);
	vl6180_i2c_write_8bits(SYSRANGE__CROSSTALK_COMPENSATION_RATE+1,
										DEFAULT_CROSSTALK&0xFF);
	
	CrosstalkHeight = 40;
	vl6180_i2c_write_8bits(SYSRANGE__CROSSTALK_VALID_HEIGHT,CrosstalkHeight&0xFF);
	
	
	// Will ignore all low distances (<100mm) with a low return rate
	IgnoreThreshold = 64; // 64 = 0.5Mcps
	IgnoreThresholdHeight = 33; // 33 * scaler3 = 99mm
	vl6180_i2c_write_8bits(SYSRANGE__RANGE_IGNORE_THRESHOLD, (IgnoreThreshold>>8)&0xFF);
	vl6180_i2c_write_8bits(SYSRANGE__RANGE_IGNORE_THRESHOLD+1,IgnoreThreshold&0xFF);
	vl6180_i2c_write_8bits(SYSRANGE__RANGE_IGNORE_VALID_HEIGHT,IgnoreThresholdHeight&0xFF);
	
	vl6180_i2c_read_8bits(SYSRANGE__RANGE_CHECK_ENABLES, &dataByte);
	dataByte = dataByte & 0xFE; // off ECE
	dataByte = dataByte | 0x02; // on ignore thr
	vl6180_i2c_write_8bits(SYSRANGE__RANGE_CHECK_ENABLES, dataByte);
	// Init of Averaging samples
	for(i=0; i<8;i++) {
		vl6180_data->LastMeasurements[i]=65535; // 65535 means no valid data
	}
	vl6180_data->CurrentIndex = 0;

#ifdef USE_INTERRUPTS
	// SetSystemInterruptConfigGPIORanging
	vl6180_i2c_read_8bits(SYSTEM__INTERRUPT_CONFIG_GPIO, &chipidgpio);
	vl6180_i2c_write_8bits(SYSTEM__INTERRUPT_CONFIG_GPIO, (chipidgpio | 0x04));
#endif

	//RangeSetSystemMode
	chipidRange = 0x01;
	vl6180_i2c_write_8bits(SYSRANGE__START, chipidRange);
#ifdef DISTANCE_FILTER
	VL6180_InitDistanceFilter();
#endif

	return rc;
}

int vl6180_release(void) {

	stmvl6180_power_enable(0);

	return 0;
}


uint16_t vl6180_getDistance()
{
	uint16_t dist = 0;//8
	uint16_t chipidcount = 0;
	uint32_t m_rawRange_mm=0;//8
	uint32_t m_rtnConvTime=0;//32
	uint32_t m_rtnSignalRate=0;//16
	uint32_t m_rtnAmbientRate=0;
	uint32_t m_rtnSignalCount = 0;//32
	uint32_t m_refSignalCount = 0;//32
	uint32_t m_rtnAmbientCount =0;//32
	uint32_t m_refAmbientCount =0;//32
	uint32_t m_refConvTime =0;//32
	uint32_t m_refSignalRate =0;
	uint32_t m_refAmbientRate =0;
	uint32_t cRtnSignalCountMax = 0x7FFFFFFF;
	uint32_t  cDllPeriods = 6;
	uint32_t rtnSignalCountUInt = 0;//32
	uint32_t  calcConvTime = 0;
	uint16_t chipidRangeStart = 0;
	uint16_t statusCode = 0;//8
	uint16_t errorCode = 0;//8

//	uint32_t m_trueRange_mm;//True Range
//	uint32_t m_rtnConvTime;//rtn Conv Time 32
//	uint32_t m_refConvTime;//Ref Conv Time	32
//	uint32_t m_strayLightFactor;
	uint16_t m_rangeOffset;// 8
	unsigned int m_crossTalk;// 16

	
	vl6180_i2c_read_8bits(SYSRANGE__START, &chipidRangeStart);
	//Read Error Code
	vl6180_i2c_read_8bits(RESULT__RANGE_STATUS, &statusCode);
	errorCode = statusCode>>4;

	printk("status code 0x%x, chipidRangeStart %x\n",statusCode,chipidRangeStart);

	if(((statusCode&0x01)==0x01)&&(chipidRangeStart==0x00)){
				
		vl6180_i2c_read_8bits(RESULT__RANGE_VAL, &dist);

		dist *= 3;

		vl6180_i2c_read_8bits(RESULT__RANGE_RAW, &chipidcount);
		m_rawRange_mm = (uint32_t)chipidcount;

		vl6180_i2c_read_32bits(RESULT__RANGE_RETURN_SIGNAL_COUNT, &rtnSignalCountUInt);

		if(rtnSignalCountUInt > cRtnSignalCountMax){
			rtnSignalCountUInt = 0;
		}

		m_rtnSignalCount  = rtnSignalCountUInt;

		vl6180_i2c_read_32bits(RESULT__RANGE_REFERENCE_SIGNAL_COUNT, &m_refSignalCount);
		vl6180_i2c_read_32bits(RESULT__RANGE_RETURN_AMB_COUNT, &m_rtnAmbientCount);
		vl6180_i2c_read_32bits(RESULT__RANGE_REFERENCE_AMB_COUNT, &m_refAmbientCount);
		vl6180_i2c_read_32bits(RESULT__RANGE_RETURN_CONV_TIME, &m_rtnConvTime);
		vl6180_i2c_read_32bits(RESULT__RANGE_REFERENCE_CONV_TIME, &m_refConvTime);

//vl6180_i2c_read_32bits(RESULT__RANGE_RETURN_CONV_TIME, &m_rtnConvTime);
//vl6180_i2c_read_32bits(RESULT__RANGE_REFERENCE_CONV_TIME, &m_refConvTime);
		vl6180_i2c_read_8bits(SYSRANGE__PART_TO_PART_RANGE_OFFSET, &m_rangeOffset);
		iReadData(SYSRANGE__CROSSTALK_COMPENSATION_RATE, 2, &m_crossTalk);

		vl6180_i2c_write_8bits(SYSTEM__INTERRUPT_CLEAR, 0x07);

		calcConvTime = m_refConvTime;
		if (m_rtnConvTime > m_refConvTime){
			calcConvTime = m_rtnConvTime;
		}
		if(calcConvTime==0)
			calcConvTime=63000;

		m_rtnSignalRate  = (m_rtnSignalCount*1000)/calcConvTime;
		m_refSignalRate  = (m_refSignalCount*1000)/calcConvTime;
		m_rtnAmbientRate = (m_rtnAmbientCount * cDllPeriods*1000)/calcConvTime;
		m_refAmbientRate = (m_rtnAmbientCount * cDllPeriods*1000)/calcConvTime;
		//printk("m_rtnSignalRate is %d, m_rtnAmbientRate is %d\n",m_rtnSignalRate,m_rtnAmbientRate);
#ifdef DISTANCE_FILTER
		dist = VL6180_DistanceFilter(vl6180_data, dist, m_rawRange_mm*3, 
						m_rtnSignalRate, m_rtnAmbientRate, errorCode);
#endif

		// Start new measurement
		//vl6180_i2c_write_8bits( SYSRANGE__START, 0x03);
		vl6180_i2c_write_8bits(SYSRANGE__START, 0x01);
		vl6180_data->m_chipid = dist;
		vl6180_data->rangeData.m_range = dist;
		vl6180_data->rangeData.m_rtnRate = m_rtnSignalRate;
		vl6180_data->rangeData.m_refRate = m_refSignalRate;
		vl6180_data->rangeData.m_rtnAmbRate = m_rtnAmbientRate;
		vl6180_data->rangeData.m_refAmbRate = m_refAmbientRate;
		vl6180_data->rangeData.m_rawRange_mm = m_rawRange_mm*3;
		vl6180_data->rangeData.m_ConvTime = calcConvTime;

		vl6180_data->rangeData.m_rtnSignalCount = m_rtnSignalCount;
		vl6180_data->rangeData.m_refSignalCount = m_refSignalCount;
		vl6180_data->rangeData.m_rtnAmbientCount = m_rtnAmbientCount;
		vl6180_data->rangeData.m_refAmbientCount = m_refAmbientCount;
		vl6180_data->rangeData.m_errorCode = statusCode;
		vl6180_data->rangeData.m_rtnConvTime = m_rtnConvTime;
		vl6180_data->rangeData.m_refConvTime = m_refConvTime;
		vl6180_data->rangeData.m_rangeOffset = m_rangeOffset;
		vl6180_data->rangeData.m_crossTalk = m_crossTalk;		
		
		printk("dist %d, rate %d, ambi rate%d\n",dist, m_rtnSignalRate, m_rtnAmbientRate);

	}
	else{
		// Return immediately with previous value
		dist = vl6180_data->m_chipid;
	}
	return dist;

}


#ifdef DISTANCE_FILTER
void VL6180_InitDistanceFilter(void)
{
	int i;

	vl6180_data->MeasurementIndex = 0;

	vl6180_data->Default_ZeroVal = 0;
	vl6180_data->Default_VAVGVal = 0;
	vl6180_data->NoDelay_ZeroVal = 0;
	vl6180_data->NoDelay_VAVGVal = 0;
	vl6180_data->Previous_VAVGDiff = 0;

	vl6180_data->StdFilteredReads = 0;
	vl6180_data->PreviousRangeStdDev = 0;
	vl6180_data->PreviousReturnRateStdDev = 0;

	for (i = 0; i < FILTERNBOFSAMPLES; i++){
		vl6180_data->LastTrueRange[i] = FILTERINVALIDDISTANCE;
		vl6180_data->LastReturnRates[i] = 0;
	}
}

uint32_t VL6180_StdDevDamper(uint32_t AmbientRate, uint32_t SignalRate, 
			uint32_t StdDevLimitLowLight, uint32_t StdDevLimitLowLightSNR, 
			uint32_t StdDevLimitHighLight, uint32_t StdDevLimitHighLightSNR)
{
	uint32_t newStdDev;
	uint16_t SNR;

	if (AmbientRate > 0)
		SNR = (uint16_t)((100 * SignalRate) / AmbientRate);
	else
		SNR = 9999;

	if (SNR >= StdDevLimitLowLightSNR){
		newStdDev = StdDevLimitLowLight;
	}
	else{
		if (SNR <= StdDevLimitHighLightSNR)
			newStdDev = StdDevLimitHighLight;
		else{
			newStdDev = (uint32_t)(StdDevLimitHighLight + 
					(SNR - StdDevLimitHighLightSNR) * (int)(StdDevLimitLowLight - StdDevLimitHighLight) /
					(StdDevLimitLowLightSNR - StdDevLimitHighLightSNR));
		}
	}

	return newStdDev;
}
uint16_t VL6180_DistanceFilter(struct stmvl6180_data *vl6180_data, uint16_t m_trueRange_mm,
	uint16_t m_rawRange_mm, uint32_t m_rtnSignalRate, uint32_t m_rtnAmbientRate, uint16_t errorCode)
{
	uint16_t m_newTrueRange_mm = 0;

	uint16_t i;
	uint16_t bypassFilter = 0;

	uint16_t registerValue;
   // uint16_t dataByte;
	uint32_t register32BitsValue1;
	uint32_t register32BitsValue2;

	uint16_t ValidDistance = 0;
	uint16_t MaxOrInvalidDistance = 0;

	uint16_t WrapAroundFlag = 0;
	uint16_t NoWrapAroundFlag = 0;
	uint16_t NoWrapAroundHighConfidenceFlag = 0;

	uint16_t FlushFilter = 0;
	uint32_t RateChange = 0;

	uint16_t StdDevSamples = 0;
	uint32_t StdDevDistanceSum = 0;
	uint32_t StdDevDistanceMean = 0;
	uint32_t StdDevDistance = 0;
	uint32_t StdDevRateSum = 0;
	uint32_t StdDevRateMean = 0;
	uint32_t StdDevRate = 0;
	uint32_t StdDevLimitWithTargetMove = 0;

	uint32_t VAVGDiff;
	uint32_t IdealVAVGDiff;
	uint32_t MinVAVGDiff;
	uint32_t MaxVAVGDiff;

	// Filter Parameters
	uint16_t WrapAroundLowRawRangeLimit = 20;
	uint32_t WrapAroundLowReturnRateLimit = 800;
	uint16_t WrapAroundLowRawRangeLimit2 = 55;
	uint32_t WrapAroundLowReturnRateLimit2 = 300;

	uint32_t WrapAroundLowReturnRateFilterLimit = 600;
	uint16_t WrapAroundHighRawRangeFilterLimit = 350;
	uint32_t WrapAroundHighReturnRateFilterLimit = 900;

	uint32_t WrapAroundMaximumAmbientRateFilterLimit = 7500;

	// Temporal filter data and flush values
	uint32_t MinReturnRateFilterFlush = 75;
	uint32_t MaxReturnRateChangeFilterFlush = 50;

	// STDDEV values and damper values
	uint32_t StdDevLimit = 300;
	uint32_t StdDevLimitLowLight = 300;
	uint32_t StdDevLimitLowLightSNR = 30; // 0.3
	uint32_t StdDevLimitHighLight = 2500;
	uint32_t StdDevLimitHighLightSNR = 5; //0.05

	uint32_t StdDevHighConfidenceSNRLimit = 8;

	uint32_t StdDevMovingTargetStdDevLimit = 90000;
	uint32_t StdDevMovingTargetReturnRateLimit = 3500;
	uint32_t StdDevMovingTargetStdDevForReturnRateLimit = 5000;

	uint32_t MAX_VAVGDiff = 1800;

	// WrapAroundDetection variables
	uint16_t WrapAroundNoDelayCheckPeriod = 2;

	// Reads Filtering values
	uint16_t StdFilteredReadsIncrement = 2;
	uint16_t StdMaxFilteredReads = 4;

	// End Filter Parameters

	MaxOrInvalidDistance = (uint16_t)(255 * 3);

	// Check if distance is Valid or not
	switch (errorCode){
	case 0x0C:
		m_trueRange_mm = MaxOrInvalidDistance;
		ValidDistance = 0;
		break;
	case 0x0D:
		m_trueRange_mm = MaxOrInvalidDistance;
		ValidDistance = 1;
		break;
	default:
		if (m_rawRange_mm >= MaxOrInvalidDistance){
			ValidDistance = 0;
		}
		else{
			ValidDistance = 1;
		}
		break;
	}
	m_newTrueRange_mm = m_trueRange_mm;

	// Checks on low range data
	if ((m_rawRange_mm < WrapAroundLowRawRangeLimit) && 
		(m_rtnSignalRate < WrapAroundLowReturnRateLimit)){
		//Not Valid distance
		m_newTrueRange_mm = MaxOrInvalidDistance;
		bypassFilter = 1;
	}
	if ((m_rawRange_mm < WrapAroundLowRawRangeLimit2) && 
		(m_rtnSignalRate < WrapAroundLowReturnRateLimit2)){
		//Not Valid distance
		m_newTrueRange_mm = MaxOrInvalidDistance;
		bypassFilter = 1;
	}

	// Checks on Ambient rate level
	if (m_rtnAmbientRate > WrapAroundMaximumAmbientRateFilterLimit){
		// Too high ambient rate
		FlushFilter = 1;
		bypassFilter = 1;
	}
	// Checks on Filter flush
	if (m_rtnSignalRate < MinReturnRateFilterFlush){
		// Completely lost target, so flush the filter
		FlushFilter = 1;
		bypassFilter = 1;
	}
	if (vl6180_data->LastReturnRates[0] != 0){
		if (m_rtnSignalRate > vl6180_data->LastReturnRates[0])
			RateChange = (100 * (m_rtnSignalRate - vl6180_data->LastReturnRates[0])) / 
						vl6180_data->LastReturnRates[0];
		else
			RateChange = (100 * (vl6180_data->LastReturnRates[0] - m_rtnSignalRate)) / 
						vl6180_data->LastReturnRates[0];
	}
	else
		RateChange = 0;
	if (RateChange > MaxReturnRateChangeFilterFlush){
		FlushFilter = 1;
	}

	if (FlushFilter == 1){
		vl6180_data->MeasurementIndex = 0;
		for (i = 0; i < FILTERNBOFSAMPLES; i++){
			vl6180_data->LastTrueRange[i] = FILTERINVALIDDISTANCE;
			vl6180_data->LastReturnRates[i] = 0;
		}
	}
	else{
		for (i = (uint16_t)(FILTERNBOFSAMPLES - 1); i > 0; i--){
			vl6180_data->LastTrueRange[i] = vl6180_data->LastTrueRange[i - 1];
			vl6180_data->LastReturnRates[i] = vl6180_data->LastReturnRates[i - 1];
		}
	}
	if (ValidDistance == 1)
		vl6180_data->LastTrueRange[0] = m_trueRange_mm;
	else
		vl6180_data->LastTrueRange[0] = FILTERINVALIDDISTANCE;
	vl6180_data->LastReturnRates[0] = m_rtnSignalRate;

	// Check if we need to go through the filter or not
	if (!(((m_rawRange_mm < WrapAroundHighRawRangeFilterLimit) && 
		(m_rtnSignalRate < WrapAroundLowReturnRateFilterLimit)) ||
		((m_rawRange_mm >= WrapAroundHighRawRangeFilterLimit) && 
		(m_rtnSignalRate < WrapAroundHighReturnRateFilterLimit))))
		bypassFilter = 1;

	// Check which kind of measurement has been made
	vl6180_i2c_read_8bits(0x01AC, &registerValue);

	// Read data for filtering
	vl6180_i2c_read_32bits(0x010C, &register32BitsValue1);
	vl6180_i2c_read_32bits(0x0110, &register32BitsValue2);
	if (registerValue == 0x3E){
		vl6180_data->Default_ZeroVal = register32BitsValue1;
		vl6180_data->Default_VAVGVal = register32BitsValue2;
	}
	else{
		vl6180_data->NoDelay_ZeroVal = register32BitsValue1;
		vl6180_data->NoDelay_VAVGVal = register32BitsValue2;
	}

	if (bypassFilter == 1) {
		// Do not go through the filter
		if (registerValue != 0x3E)
		{
			vl6180_i2c_write_8bits(0x01AC, 0x3E);
		}
		// Set both Defaut and NoDelay To same value
		vl6180_data->Default_ZeroVal = register32BitsValue1;
		vl6180_data->Default_VAVGVal = register32BitsValue2;
		vl6180_data->NoDelay_ZeroVal = register32BitsValue1;
		vl6180_data->NoDelay_VAVGVal = register32BitsValue2;
		vl6180_data->MeasurementIndex = 0;

		// Return immediately
		return m_newTrueRange_mm;
	}

	if (vl6180_data->MeasurementIndex % WrapAroundNoDelayCheckPeriod == 0){
		vl6180_i2c_write_8bits(0x01AC, 0x3F);
	}
	else{
		vl6180_i2c_write_8bits(0x01AC, 0x3E);
	}

	vl6180_data->MeasurementIndex = (uint16_t)(vl6180_data->MeasurementIndex + 1);

	// Computes current VAVGDiff
	if (vl6180_data->Default_VAVGVal > vl6180_data->NoDelay_VAVGVal)
		VAVGDiff = vl6180_data->Default_VAVGVal - vl6180_data->NoDelay_VAVGVal;
	else
		VAVGDiff = 0;
	vl6180_data->Previous_VAVGDiff = VAVGDiff;

	// Check the VAVGDiff
	if(vl6180_data->Default_ZeroVal > vl6180_data->NoDelay_ZeroVal)
		IdealVAVGDiff = vl6180_data->Default_ZeroVal - vl6180_data->NoDelay_ZeroVal;
	else
		IdealVAVGDiff = vl6180_data->NoDelay_ZeroVal - vl6180_data->Default_ZeroVal;
	if (IdealVAVGDiff > MAX_VAVGDiff)
		MinVAVGDiff = IdealVAVGDiff - MAX_VAVGDiff;
	else
		MinVAVGDiff = 0;
	MaxVAVGDiff = IdealVAVGDiff + MAX_VAVGDiff;
	if (VAVGDiff < MinVAVGDiff || VAVGDiff > MaxVAVGDiff){
		WrapAroundFlag = 1;
	}
	else{
		// Go through filtering check

		// StdDevLimit Damper on SNR
		StdDevLimit = VL6180_StdDevDamper(m_rtnAmbientRate, m_rtnSignalRate, 
								StdDevLimitLowLight, StdDevLimitLowLightSNR,
								StdDevLimitHighLight, StdDevLimitHighLightSNR);

		// Standard deviations computations
		StdDevSamples = 0;
		StdDevDistanceSum = 0;
		StdDevDistanceMean = 0;
		StdDevDistance = 0;
		StdDevRateSum = 0;
		StdDevRateMean = 0;
		StdDevRate = 0;
		for (i = 0; (i < FILTERNBOFSAMPLES) && (StdDevSamples < FILTERSTDDEVSAMPLES); i++){
			if (vl6180_data->LastTrueRange[i] != FILTERINVALIDDISTANCE){
				StdDevSamples = (uint16_t)(StdDevSamples + 1);
				StdDevDistanceSum = (uint32_t)(StdDevDistanceSum + vl6180_data->LastTrueRange[i]);
				StdDevRateSum = (uint32_t)(StdDevRateSum + vl6180_data->LastReturnRates[i]);
			}
		}
		if (StdDevSamples > 0){
			StdDevDistanceMean = (uint32_t)(StdDevDistanceSum / StdDevSamples);
			StdDevRateMean = (uint32_t)(StdDevRateSum / StdDevSamples);
		}
		StdDevSamples = 0;
		StdDevDistanceSum = 0;
		StdDevRateSum = 0;
		for (i = 0; (i < FILTERNBOFSAMPLES) && (StdDevSamples < FILTERSTDDEVSAMPLES); i++){
			if (vl6180_data->LastTrueRange[i] != FILTERINVALIDDISTANCE){
				StdDevSamples = (uint16_t)(StdDevSamples + 1);
				StdDevDistanceSum = (uint32_t)(StdDevDistanceSum + 
					(int)(vl6180_data->LastTrueRange[i] - StdDevDistanceMean) * 
					(int)(vl6180_data->LastTrueRange[i] - StdDevDistanceMean));
				StdDevRateSum = (uint32_t)(StdDevRateSum + (int)(vl6180_data->LastReturnRates[i] - 
					StdDevRateMean) * (int)(vl6180_data->LastReturnRates[i] - StdDevRateMean));
			}
		}
		if (StdDevSamples >= MINFILTERSTDDEVSAMPLES){
			StdDevDistance = (uint16_t)(StdDevDistanceSum / StdDevSamples);
			StdDevRate = (uint16_t)(StdDevRateSum / StdDevSamples);
		}
		else{
			StdDevDistance = 0;
			StdDevRate = 0;
		}

		// Check Return rate standard deviation
		if (StdDevRate < StdDevMovingTargetStdDevLimit){
			if (StdDevSamples < MINFILTERVALIDSTDDEVSAMPLES){
				m_newTrueRange_mm = MaxOrInvalidDistance;
			}
			else{
				// Check distance standard deviation
				if (StdDevRate < StdDevMovingTargetReturnRateLimit)
					StdDevLimitWithTargetMove = StdDevLimit + 
						(((StdDevMovingTargetStdDevForReturnRateLimit - StdDevLimit) * StdDevRate) / 
						StdDevMovingTargetReturnRateLimit);
				else
					StdDevLimitWithTargetMove = StdDevMovingTargetStdDevForReturnRateLimit;

				if ((StdDevDistance * StdDevHighConfidenceSNRLimit) < StdDevLimitWithTargetMove){
					NoWrapAroundHighConfidenceFlag = 1;
				}
				else{
					if (StdDevDistance < StdDevLimitWithTargetMove){
						if (StdDevSamples >= MINFILTERVALIDSTDDEVSAMPLES){
							NoWrapAroundFlag = 1;
						}
						else{
							m_newTrueRange_mm = MaxOrInvalidDistance;
						}
					}
					else{
						WrapAroundFlag = 1;
					}
				}
			}
		}
		else{
			WrapAroundFlag = 1;
		}
	}

	if (m_newTrueRange_mm == MaxOrInvalidDistance){
		if (vl6180_data->StdFilteredReads > 0)
			vl6180_data->StdFilteredReads = (uint16_t)(vl6180_data->StdFilteredReads - 1);
	}
	else{
		if (WrapAroundFlag == 1){
			m_newTrueRange_mm = MaxOrInvalidDistance;
			vl6180_data->StdFilteredReads = (uint16_t)(vl6180_data->StdFilteredReads + 
												StdFilteredReadsIncrement);
			if (vl6180_data->StdFilteredReads > StdMaxFilteredReads)
				vl6180_data->StdFilteredReads = StdMaxFilteredReads;
		}
		else{
			if (NoWrapAroundFlag == 1){
				if (vl6180_data->StdFilteredReads > 0){
					m_newTrueRange_mm = MaxOrInvalidDistance;
					if (vl6180_data->StdFilteredReads > StdFilteredReadsIncrement)
						vl6180_data->StdFilteredReads = (uint16_t)(vl6180_data->StdFilteredReads - 
															StdFilteredReadsIncrement);
					else
						vl6180_data->StdFilteredReads = 0;
				}
			}
			else{
				if (NoWrapAroundHighConfidenceFlag == 1){
					vl6180_data->StdFilteredReads = 0;
				}
			}
		}
	}
	vl6180_data->PreviousRangeStdDev = StdDevDistance;
	vl6180_data->PreviousReturnRateStdDev = StdDevRate;
	vl6180_data->PreviousStdDevLimit = StdDevLimitWithTargetMove;

	return m_newTrueRange_mm;
}

#endif

/*******************************************************************************
*
********************************************************************************/
static long stmvl6180_Ioctl(struct file *file, unsigned int a_u4Command, unsigned long a_u4Param)
{
	int rc=0;
	struct stmvl6180_data *vl6180_data_t = file->private_data;
       void __user *argp = (void __user *)a_u4Param;
	unsigned long data=0;
	RegisterInfo register_data;
	switch (a_u4Command) {
	case VL6180_IOCTL_INIT:	   /* init.  */
	{
        	unsigned long *distance = (unsigned long *)argp;
		data = vl6180_getDistance(vl6180_data_t);
        	*distance = data;
        	printk("vl6180_getDistance init return %ld\n",*distance);
		return 0;
	}
	case VL6180_IOCTL_GETDATA:	  /* Get proximity value only */
	{
		data = vl6180_getDistance(vl6180_data_t);
		printk("vl6180_getDistance return %ld\n",data);
		//return put_user(data, (unsigned long *)p);
		return 0;
	}
	case VL6180_IOCTL_GETDATAS:	 /* Get all range data */
	{
		data = vl6180_getDistance(vl6180_data_t);

		if (copy_to_user((RangeData *)a_u4Param, &(vl6180_data_t->rangeData), sizeof(RangeData))) {
			printk("stmvl6180 error copy to user\n");
			rc = -EFAULT;
		}	 
		return rc;   
	}
	case VL6180_IOCTL_CONFIG:        /* Get all range data */
       {
               if (copy_from_user(&register_data, argp, sizeof(RegisterInfo))) {
                       printk("stmvl6180 error copy from user\n");
                       rc = -EFAULT;
               }
               printk("%s:%d stmvl6180 wirte register 0x%x:0x%x\n", __func__, __LINE__, register_data.addr, register_data.data);

               rc = vl6180_i2c_write_8bits(register_data.addr, register_data.data);
               if(rc)
                       printk("%s:%d stmvl6180 failed to wirte register 0x%x:0x%x\n", __func__, __LINE__, register_data.addr, register_data.data);
               return rc;
       }
	
	default:
		return -EINVAL;
	}

	//kfree(pBuff);
	//kfree(pWorkingBuff);
	return rc;
}


static u32 g_u4Opened;
/* #define */
/* Main jobs: */
/* 1.check for device-specified errors, device not ready. */
/* 2.Initialize the device if it is opened for the first time. */
static int stmvl6180_Open(struct inode *a_pstInode, struct file *a_pstFile)
{
	PROXIMITYDB("[stmvl6180] Open\n");
//	spin_lock(&g_stmvl6180Lock);
	if (g_u4Opened) {
//		spin_unlock(&g_stmvl6180Lock);
		return -EBUSY;
	} else {
		g_u4Opened = 1;
	//	atomic_set(&g_PROXIMITYatomic, 0);
	}
//	spin_unlock(&g_stmvl6180Lock);



	a_pstFile->private_data = vl6180_data;
	//vl6180_init(i2c_get_clientdata(file->private_data));
      if(vl6180_data!=NULL){
	    vl6180_init(vl6180_data);
      }
	return 0;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
static int stmvl6180_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
//	spin_lock(&g_VL6180Lock);

	g_u4Opened = 0;

//	atomic_set(&g_PROXIMITYatomic, 0);

//	spin_unlock(&g_stmvl6180Lock);
      if(vl6180_data!=NULL){
	    vl6180_release();
      }
	return 0;
}

static const struct file_operations stmvl6180_ranging_fops = {
	.owner = THIS_MODULE,
	.open = stmvl6180_Open,
	.release = stmvl6180_Release,
	.unlocked_ioctl = stmvl6180_Ioctl
};
static struct miscdevice stmvl6180_ranging_dev = {
	.minor =	MISC_DYNAMIC_MINOR,
	.name =		"stmvl6180_ranging",
	.fops =		&stmvl6180_ranging_fops
};
#if 0
#define stmvl6180_DYNAMIC_ALLOCATE_DEVNO 1
#define PROXIMITY_DYNAMIC_ALLOCATE_DEVNO
inline static int RegisterPROXIMITYCharDrv(void)
{
	struct device *stmvl6180_device = NULL;

#ifdef PROXIMITY_DYNAMIC_ALLOCATE_DEVNO
	if (alloc_chrdev_region(&g_PROXIMITYdevno, 0, 1, "stmvl6180_ranging")) {
		PROXIMITYDB("[stmvl6180] Allocate device no failed\n");
		return -EAGAIN;
	}
#else
	if (register_chrdev_region(g_PROXIMITYdevno, 1, "stmvl6180_ranging")) {
		PROXIMITYDB("[stmvl6180] Register device no failed\n");
		return -EAGAIN;
	}
#endif

	/* Allocate driver */
	g_pPROXIMITY_CharDrv = cdev_alloc();

	if (NULL == g_pPROXIMITY_CharDrv) {
		unregister_chrdev_region(g_pPROXIMITY_CharDrv, 1);

		PROXIMITYDB("[stmvl6180] Allocate mem for kobject failed\n");

		return -ENOMEM;
	}
	/* Attatch file operation. */
	cdev_init(g_pPROXIMITY_CharDrv, &stmvl6180_ranging_fops);

	g_pPROXIMITY_CharDrv->owner = THIS_MODULE;

	/* Add to system */
	if (cdev_add(g_pPROXIMITY_CharDrv, g_PROXIMITYdevno, 1)) {
		PROXIMITYDB("[stmvl6180] Attatch file operation failed\n");
		unregister_chrdev_region(g_PROXIMITYdevno, 1);

		return -EAGAIN;
	}

	PROXIMITY_class = class_create(THIS_MODULE, "PROXIMITYdrv");
	if (IS_ERR(PROXIMITY_class)) {
		int ret = PTR_ERR(PROXIMITY_class);
		PROXIMITYDB("Unable to create class, err = %d\n", ret);
		return ret;
	}
	stmvl6180_device = device_create(PROXIMITY_class, NULL, g_PROXIMITYdevno, NULL, "stmvl6180_ranging");

	return 0;
}

inline static void Unregisterstmvl6180CharDrv(void)
{
	/* Release char driver */
	cdev_del(g_pPROXIMITY_CharDrv);

	unregister_chrdev_region(g_PROXIMITYdevno, 1);

	device_destroy(PROXIMITY_class, g_PROXIMITYdevno);
	class_destroy(PROXIMITY_class);
}
#endif
static int stmvl6180_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int stmvl6180_i2c_remove(struct i2c_client *);
static int stmvl6180_detect(struct i2c_client *client,  struct i2c_board_info *info) {         
    strcpy(info->type, PRO_HW_NAME);       
     printk("[stmvl6180_detect] Attach I2C \n");	
    return 0;                                                                                       
} 

static const struct of_device_id stmvl6180_HW_i2c_of_ids[] = {
{ .compatible = "mediatek,CAMERA_HW_i2C", },
{}
};


static const struct i2c_device_id stmvl6180_i2c_id[] = { {PRO_HW_NAME, 0}, {} };
static struct i2c_driver stmvl6180_i2c_driver = {
	.probe = stmvl6180_i2c_probe,
	.remove = stmvl6180_i2c_remove,
	.detect  = stmvl6180_detect,
	.driver = {
	.name = PRO_HW_NAME,
	.owner = THIS_MODULE,
	.of_match_table = stmvl6180_HW_i2c_of_ids,
	},
	.id_table = stmvl6180_i2c_id,
	.address_list = normal_i2c,
};
/*
 * Initialization function
 */

static int stmvl6180_init_client(void)
{
	int err;
	int id=0,module_major=0,module_minor=0;
	int model_major=0,model_minor=0;
	int i=0,val;

	vl6180_data->is_6180 = 1;
	err = iReadPROXIMITY(VL6180_MODEL_ID_REG,
						 1,
						 &id); 
	if (id == 0xb4) {
		printk("STM stmvl6180 Found\n");
	}
	else if (id==0) {
		printk("Not found stmvl6180\n");
		return -EIO;
	}

	// Read Model Version
	err = iReadData(VL6180_MODEL_REV_MAJOR_REG,
						 1,
						 &model_major); 
	err = iReadData(VL6180_MODEL_REV_MINOR_REG,
						 1,
						 &model_minor); 
	printk("stmvl6180 Model Version : %d.%d\n", model_major,model_minor);

	// Read Module Version
	err = iReadData(VL6180_MODULE_REV_MAJOR_REG,
						 1,
						 &module_major);
	err = iReadData(VL6180_MODULE_REV_MINOR_REG,
						 1,
						 &module_minor); 
	printk("stmvl6180 Module Version : %d.%d\n",module_major,module_minor);
	
	// Read Identification 
	printk("stmvl6180 Serial Number: ");
	for (i=0; i<=(VL6180_FIRMWARE_REVISION_ID_REG-VL6180_REVISION_ID_REG);i++) {
		err = iReadData((VL6180_REVISION_ID_REG+i),
						 		 1,
						 		 &val);
		printk("0x%x-",val);
	}
	printk("\n");
	return 0;
}
static int stmvl6180_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int i4RetValue = 0;
	int rc = 0;
	printk("[stmvl6180] Attach I2C\n");
/* spin_lock_init(&g_VL6180Lock); */
#if 1
	unsigned long data=0;
#endif
	/* get sensor i2c client */
	spin_lock(&g_VL6180Lock);	/* for SMP */
	g_pstI2Cclient = client;
	g_pstI2Cclient->addr = STMVL6180_DEVICE_ID;
	spin_unlock(&g_VL6180Lock);	/* for SMP */

	printk("[stmvl6180] g_pstI2Cclient->addr = 0x%8x\n", g_pstI2Cclient->addr);

	rc = stmvl6180_power_enable(1);
	if(rc) {
		pr_err("failed rc %d\n", rc);
		return rc;
	}

	/* Initialize the STM VL6180 chip */
	rc = stmvl6180_init_client();
	if (rc) {
		stmvl6180_power_enable(0);
		pr_err("failed rc %d\n", rc);
		return rc;
	}

	stmvl6180_power_enable(0);
#if 0
	/* Register char driver */
	i4RetValue = RegisterPROXIMITYCharDrv();

	if (i4RetValue) {
		printk("[stmvl6180] register char device failed!\n");
		return i4RetValue;
	}
#else
	//to register as a misc device
	if (misc_register(&stmvl6180_ranging_dev) != 0)
		printk(KERN_INFO "Could not register misc. dev for stmvl6180 ranging\n");
#if 1
 vl6180_init(vl6180_data);//just for test
 msleep(10);
		data = vl6180_getDistance(vl6180_data);
        printk("vl6180_getDistance init return %ld\n",data);
	stmvl6180_power_enable(0);
#endif
#endif
	printk("[stmvl6180] Attached!!\n");
	return 0;
}

static int stmvl6180_i2c_remove(struct i2c_client *client)
{
	misc_deregister(&stmvl6180_ranging_dev);
	return 0;
}
static int stmvl6180_platform_probe(struct platform_device *pdev)
{
       printk("strmvl6180 stmvl6180_platform_probe\n");
	return i2c_add_driver(&stmvl6180_i2c_driver);
}

static int stmvl6180_remove(struct platform_device *pdev)
{
	i2c_del_driver(&stmvl6180_i2c_driver);
	return 0;
}
/* platform structure */
static struct platform_driver stmvl6180_platform_driver = {
	.probe = stmvl6180_platform_probe,
	.remove = stmvl6180_remove,
	.driver = {
		   .name = STMVL6180_DRV_NAME,
		   .owner = THIS_MODULE,
		   }
};


static struct platform_device stmvl6180_platform_device = {
	.name = STMVL6180_DRV_NAME,
	.id = 0,
	.dev = {
		}
};

static int __init PROXIMITY_i2C_init(void)
{
      printk("stmvl6180 proximity_i2c_init E\n");
	i2c_register_board_info(PROXIMITY_I2C_BUSNUM, &kd_proximity_dev, 1);

	if (platform_driver_register(&stmvl6180_platform_driver)) {
		PROXIMITYDB("failed to register stmvl6180 driver\n");
		return -ENODEV;
	}
	if (platform_device_register(&stmvl6180_platform_device)) {
		PROXIMITYDB("failed to register stmvl6180 device\n");
		return -ENODEV;
	}
	return 0;
}

static void __exit PROXIMITY_i2C_exit(void)
{
	platform_driver_unregister(&stmvl6180_platform_driver);
}

module_init(PROXIMITY_i2C_init);
module_exit(PROXIMITY_i2C_exit);


MODULE_AUTHOR("STM");
MODULE_DESCRIPTION("ST FlightSense Time-of-Flight sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
