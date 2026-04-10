/****************************************************************************
* Copyright (c) 2004-2014 OPPO Mobile multimedia Corp.ltd.,
* VENDOR_EDIT
* Description: Source file for LCD driver.
* Version   : 1.0
* Date      : 2015-02-08
* Author    : yangxinqin@PhoneSW.Multimedia
*----------------------Revision History--------------------------
* None.
*****************************************************************************/

#ifndef BUILD_LK
#include <linux/string.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <linux/oppo_devices_list.h>
#include <mach/mt_boot_common.h>
#endif
#include "lcm_drv.h"
#include <cust_gpio_usage.h>
#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
    #include <platform/boot_mode.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif

#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)
#define PHYSICAL_WIDTH      (74)
#define PHYSICAL_HEIGHT     (132)

#define GPIO_LCD_RST_PIN 	(GPIO106| 0x80000000)//(GPIO112 | 0x80000000)
#define GPIO_LCD_VCI_EN_PIN 	(GPIO144| 0x80000000)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xdd   // END OF REGISTERS MARKER


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//static unsigned int lcm_esd_test = FALSE;      ///only for ESD test

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmd_by_cmdq_dual(handle,cmd,count,ppara,force_update)    lcm_util.dsi_set_cmdq_V23(handle,cmd,count,ppara,force_update);
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE							1
extern long int cal_te[4];
extern int te_cnt;
bool te_cal_enable =1;

volatile int OPPO_LED_ON = 0;  // Add for compile error

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
    {0x29, 1 ,{0x00}},
    {REGFLAG_DELAY, 10, {}},
    {0x11,  1 ,{0x00}},
    {REGFLAG_DELAY, 20, {}},

    // Common Setting
    // TE ON
    {0x35, 1 ,{0x00}},
    {REGFLAG_DELAY, 1, {}},
    //Power Setting
    {0xFC, 2 ,{0x5A, 0x5A}},
    {0xB0, 1 ,{0x1E}},
    {0xFD, 1 ,{0xA8}},
    {0xB0, 1 ,{0x10}},
    {0xB6, 1 ,{0x54}},
    {0xFC, 2 ,{0xA5, 0xA5}},
    {REGFLAG_DELAY, 1, {}},

    //Brightness Control
    //Dimming Mode
    {0x53, 1 ,{0x20}},
    {0x51,   1, {0x00}},
    //{REGFLAG_DELAY, 1, {}},

    {REGFLAG_DELAY, 150, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_in_setting[] = {
	// Display off sequence
	{0x28,   1, {0x00}},
	{REGFLAG_DELAY, 50, {}},
	// Sleep Mode On
	{ 0x10,   1, {0x00}},
	{REGFLAG_DELAY, 150, {}},
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0x51,   1, {0xFF}},
};


void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}

//#ifndef BUILD_LK
extern LCD_DEV lcd_dev;
//#endif

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    int boot_mode = 0;
    
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    params->physical_width = PHYSICAL_WIDTH;
    params->physical_height = PHYSICAL_HEIGHT;

    params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY; //LCM_DBI_TE_MODE_VSYNC_OR_HSYNC;                                                
    params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode   =  CMD_MODE;//ESYNC_PULSE_VDO_MODE;//; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
#else
    params->dsi.mode   =  BURST_VDO_MODE;//CMD_MODE
#endif
	
    // DSI
    /* Command mode setting */
    //1 Three lane or Four lane
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Video mode setting		
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	
    params->dsi.vertical_sync_active				= 2;// 0x05
    params->dsi.vertical_backporch					= 4;//  0x0d
    params->dsi.vertical_frontporch					= 20; // 0x08
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active				= 12;//10;// 0x12
    params->dsi.horizontal_backporch				= 70;//14; //0x5f
    params->dsi.horizontal_frontporch				= 140;//30;//0x5f
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

    //params->dsi.LPX=8; 

    // Bit rate calculation
    params->dsi.PLL_CLOCK = 450;//475;
    params->dsi.ssc_disable=1;//MUST Disable SSC

    if (g_boot_mode == META_BOOT) {
        boot_mode++;
        LCD_DEBUG("META_BOOT\n");
    }
    if (g_boot_mode == ADVMETA_BOOT) {
        boot_mode++;
        LCD_DEBUG("ADVMETA_BOOT\n");
    }
    if (g_boot_mode == ATE_FACTORY_BOOT) {
        boot_mode++;
        LCD_DEBUG("ATE_FACTORY_BOOT\n");
    }
    if (g_boot_mode == FACTORY_BOOT) {
        boot_mode++;
        LCD_DEBUG("FACTORY_BOOT\n");
    }
    if (boot_mode == 0) {
        LCD_DEBUG("neither META_BOOT or FACTORY_BOOT\n");
        params->dsi.esd_check_enable = 1;
        params->dsi.customization_esd_check_enable = 0;
    }
	
//#ifndef BUILD_LK
    lcd_dev = LCD_SAMSUNG_S6E3FA3_CMD;
//#endif	
}

static void lcm_init(void)
{
    long int tmp_te_val=0;
	int i,j;
    
    mt_set_gpio_mode(GPIO_LCD_RST_PIN, 0);
    mt_set_gpio_dir(GPIO_LCD_RST_PIN, GPIO_DIR_OUT);        
    mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
    MDELAY(10); // 1ms
    
    // Power on
    LCD_DEBUG("lcm_init() Enable LCD_VCI\n");
    mt_set_gpio_mode(GPIO_LCD_VCI_EN_PIN, 0);  //LCD_VCI_3P3, 2.8V
    mt_set_gpio_dir(GPIO_LCD_VCI_EN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_VCI_EN_PIN, GPIO_OUT_ONE);
	
	// for 15019 vgp3
#ifdef BUILD_LK
    //mt6331_upmu_set_rg_vgp3_vosel(7);//3.3v
    //mt6331_upmu_set_rg_vgp3_en(1);
#else
    //printk("[soso] vgp3 on\n");
    //hwPowerOn(MT6331_POWER_LDO_VGP3, VOL_3300, "LCD");
#endif
    MDELAY(10);

    LCD_DEBUG("lcm_init() RESX 1-->0-->1\n");
    mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
    MDELAY(10); // 1ms
    mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ONE);
    MDELAY(10); // 1ms

    LCD_DEBUG("lcm_init() Send init command\n");
    push_table(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
}


static void lcm_suspend(void)
{
    // Sleep in
    push_table(lcm_sleep_in_setting,sizeof(lcm_sleep_in_setting)/sizeof(lcm_sleep_in_setting[0]),1);
    // RESX LOW
    LCD_DEBUG("lcm_suspend() RESET 1 --> 0\n");
    mt_set_gpio_mode(GPIO_LCD_RST_PIN, 0);
    mt_set_gpio_dir(GPIO_LCD_RST_PIN, GPIO_DIR_OUT);        
    mt_set_gpio_out(GPIO_LCD_RST_PIN, GPIO_OUT_ZERO);
    MDELAY(2);

    // Power off
    LCD_DEBUG("lcm_suspend() LCD VCI OFF\n");
    mt_set_gpio_mode(GPIO_LCD_VCI_EN_PIN, 0);  //LCD_VCI_3P3, 2.8V
    mt_set_gpio_dir(GPIO_LCD_VCI_EN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_VCI_EN_PIN, GPIO_OUT_ZERO);

    OPPO_LED_ON = 0;
}


static void lcm_resume(void)
{
    LCD_DEBUG("lcm_resume\n");
    lcm_init();

    OPPO_LED_ON = 1;
}
         
#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

//extern int IMM_GetOneChannelValue(int dwChannel, int deCount);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
#define VOLTAGE_FULL_RANGE_LCD 	1500 // VA voltage
#define ADC_PRECISE 		4096 // 12 bits

static unsigned int lcm_compare_id(void)
{
    int data[4] = {0,0,0,0};
    int res =0;
    int rawdata=0;
    int val=0;
   
    res =IMM_GetOneChannelValue(0,data,&rawdata);

    if(res<0) {
        LCD_DEBUG("lcm_compare_id(), res < 0\n");  
		return 0; 
    } else {
        val=rawdata*VOLTAGE_FULL_RANGE_LCD/ADC_PRECISE;
        LCD_DEBUG("lcm_compare_id(), res >= 0");
    }
    if(val < 100)
        return 1;
    else
        return 0;
}

//static void lcm_set_backlight(unsigned int level) 
int lcm_set_backlight(unsigned int level)
{
	unsigned int mapped_level = 0;
	
	printk("%s YXQ level is %d\n", __func__, level);

	if(level > 255) {
		mapped_level = 255;
	} else if (level < 0) {
		mapped_level = 0;
	} else {
        mapped_level = level;
    }

    // Refresh value of backlight level.
    lcm_backlight_level_setting[0].para_list[0] = mapped_level;
    push_table(lcm_backlight_level_setting,sizeof(lcm_backlight_level_setting)/sizeof(lcm_backlight_level_setting[0]),1);

    return 0;
}

static void lcm_setbacklight_cmdq(void* handle,unsigned int level)
{
	unsigned int mapped_level = 0;
	
	printk("%s level is %d\n", __func__, level);

	if(level > 255) {
		mapped_level = 255;
	} else if (level < 0) {
		mapped_level = 0;
	} else {
        mapped_level = level;
    }

    // Refresh value of backlight level.
    //lcm_backlight_level_setting[0].para_list[0] = mapped_level;
    //push_table(lcm_backlight_level_setting,sizeof(lcm_backlight_level_setting)/sizeof(lcm_backlight_level_setting[0]),1);
    unsigned int cmd = 0x51;
	unsigned int count =1;
	unsigned int value = mapped_level;
	dsi_set_cmd_by_cmdq_dual(handle, cmd, count, &value, 1);

    return 0;
}

int lcm_set_backlight_lk(unsigned int level){
    lcm_set_backlight(level);
    return 0;
}

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char  buffer[2];
	int   array[4];

	/// please notice: the max return packet size is 1
	/// if you want to change it, you can refer to the following marked code
	/// but read_reg currently only support read no more than 4 bytes....
	/// if you need to read more, please let BinHan knows.
	/*
	   unsigned int data_array[16];
	   unsigned int max_return_size = 1;

	   data_array[0]= 0x00003700 | (max_return_size << 16);    

	   dsi_set_cmdq(&data_array, 1, 1);
	 */
	array[0] = 0x00013700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 1);
	if(buffer[0]==0x1C)
	{
		return FALSE;
	}
	else
	{            
		return TRUE;
	}                                                                                                                        
#endif

}

#if 0
static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	lcm_resume();

	return TRUE;
}
#endif

LCM_DRIVER s6e3fa3_fhd_dsi_cmd_lcm_drv = 
{
	.name			= "s6e3fa3_fhd_dsi_cmd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	//	.esd_check = lcm_esd_check,
	//	.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
	//.set_backlight	= lcm_set_backlight,
	.set_backlight_cmdq  = lcm_setbacklight_cmdq,
	.update         = lcm_update,
#endif
};
