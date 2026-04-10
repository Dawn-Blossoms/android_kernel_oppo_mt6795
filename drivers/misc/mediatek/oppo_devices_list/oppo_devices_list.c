/************************************************************************************
** File: - mediatek\source\kernel\drivers\oppo_devices_list\oppo_devices_list.c
** VENDOR_EDIT
** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
** 
** Description: 
**      driver for get devices list
** 
** Version: 1.0
** Date created: 10:38:47,18/06/2012
** Author: Yixue.Ge@ProDrv.BL
** 
** --------------------------- Revision History: --------------------------------
** 	<author>	<data>			<desc>
** 
************************************************************************************/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
//#include <mach/mt6577_gpio.h>
#include <linux/platform_device.h>
#include <linux/oppo_devices_list.h>
#include <mach/mt_gpio.h> 

volatile LCD_DEV lcd_dev = LCD_NONE;
volatile TP_DEV oppo_tp_dev = TP_NONE;
volatile int TP_FW = 0;
volatile int TP_FW_CONFIG_ID= 0;
// lingjianing@CameraDrv, 2013/10/28, add for 13065 firmware version	
volatile int MS2R_FW = 0;
volatile OPPO_BKL_DEV oppo_bkl_dev = BKL_NONE;
volatile CAMERA_BACK_DEV camera_back_dev = CAMERA_BACK_NONE;
volatile CAMERA_FRONT_DEV camera_front_dev = CAMERA_FRONT_NONE;
volatile MCP_DEV mcp_dev= MCP_NONE;
volatile ALSPS_DEV alsps_dev = ALSPS_NONE;
volatile GYRO_DEV gyro_dev = GYRO_NONE;

unsigned char Mcp_Id[][9] = {
{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},//NONE
{0x15,0x01,0x00,0x53,0x4A,0x53,0x30,0x30,0x4D},//MCP_KMSJS000KM_B308,		//0x150100534A5330304D
{0x90,0x01,0x4A,0x20,0x58,0x49,0x4E,0x59,0x48},//MCP_H9DP32A4JJACGR_KEM,	//0x90014A2058494E5948
{0x15,0x01,0x00,0x4B,0x4A,0x53,0x30,0x30,0x4D},//MCP_KMKJS000VM_B309,		//0x1501004B4A5330304D
{0x15,0x01,0x00,0x4E,0x4A,0x53,0x30,0x30,0x4D},//MCP_KMNJS000ZM_B205,		//0x1501004E4A5330304D
{0x90,0x01,0x4A,0x20,0x58,0x49,0x4E,0x59,0x48},//MCP_H9TP32A4GDMCPR_KDM,	//0x90014A2058494E5948  
{0x15,0x01,0x00,0x4B,0x54,0x53,0x30,0x30,0x4D},//MCP_KMKJS000VM_B604,		//0x1501004B545330304D
{0x15,0x01,0x00,0x4B,0x33,0x55,0x30,0x30,0x4D},//MCP_KMK3U000VM_B410,		//0x1501004B335530304D
{0x90,0x01,0x4A,0x48,0x41,0x47,0x32,0x65,0x04},//MCP_H9TP17A8JDACNR_KGM, 	//0x90014A484147326504
{0x11,0x01,0x00,0x30,0x31,0x36,0x47,0x39,0x34},//MCP_TYD0HH251623RC,		//0x110100303136473934
{0x45,0x01,0x00,0x53,0x45,0x4D,0x31,0x36,0x47},//MCP_SD5C28B_16G,			//0x45010053454D313647
{0x90,0x01,0x4A,0x20,0x58,0x49,0x4E,0x59,0x48},//MCP_H9TP32A8JDMCPR_KGM, 	//0x90014A2058494E5948
{0x90,0x01,0x4A,0x48,0x34,0x47,0x31,0x64,0x04},//MCP_H9TP32A4GDBCPR_KGM, 	//0x90014A483447316404
{0x11,0x01,0x00,0x30,0x30,0x34,0x47,0x39,0x30},//MCP_TYC0FH121597RA,		//0x110100303034473930
};
static int getMcpId(void)
{
    struct file* pfile = NULL;
	unsigned long magic; 
	loff_t pos = 0;
	mm_segment_t fs;
	int num = -1;
    unsigned char pCustPartBuf[1024];
    TOppoCustConfigInf *pConfigInf;
    int i;
    
	if(NULL == pfile){
		pfile = filp_open("/dev/oppo_custom", O_RDONLY, 0);
	}
	if(IS_ERR(pfile)){
		printk("error occured while opening file /dev/oppo_custom.\n");
		return 0;
	}
	//char * buf;
	fs = get_fs();
	set_fs(KERNEL_DS);
	num = vfs_read(pfile, pCustPartBuf, 1024, &pos);
	if(num != 1024)
	{
	    filp_close(pfile, NULL);
        return 0;
	}
	filp_close(pfile, NULL);
	set_fs(fs);
	pConfigInf = (TOppoCustConfigInf *)pCustPartBuf;
	if(D_OPPO_CUST_PART_MAGIC_NUM != pConfigInf->nMagicNum1)
    {  
	    printk("getMcpId OPPO_CUSTOM partition is illegal!\n");
	    return 0;
	}

	if(D_OPPO_CUST_PART_CONFIG_MAGIC_NUM != pConfigInf->nMagicNum2)
	{
	    printk("getMcpId OPPO_CUSTOM partition with error config magic number!\n");
	    return 0;
	}   
    printk("getMcpId:%x %x %x %x %x %x %x %x %x\n",pConfigInf->sMcpId[0],pConfigInf->sMcpId[1],
        pConfigInf->sMcpId[2],pConfigInf->sMcpId[3],pConfigInf->sMcpId[4],pConfigInf->sMcpId[5],
        pConfigInf->sMcpId[6],pConfigInf->sMcpId[7],pConfigInf->sMcpId[8]);
    for(i = 0; i < sizeof(Mcp_Id)/9;i++)
    {
        if(strncmp(Mcp_Id[i],pConfigInf->sMcpId,9) == 0)
        return i;
    }
	return 0;
}
void McpIdInit()
{
    if(!mcp_dev)
        mcp_dev = getMcpId();
}
static ssize_t oppo_devices_list_write(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	/*not support write now*/
	return count;
}

static ssize_t oppo_devices_list_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	char page[512]; 
	char *p = page;
	int len = 0; 
	unsigned char id[12]={0};
	printk("oppo_devices_list_read is called\n");

	/*************************add here for customer devices list***************/
    switch(lcd_dev)
    {
        case LCD_NONE:
            p += sprintf(p, "LCD :None\n");
            break;
        case LCD_HITACHI:
            p += sprintf(p, "LCD :Hitachi\n");
            break;
        case LCD_TRULY:
            p += sprintf(p, "LCD :Truly\n");
            break;
        case LCD_BYD:
            p += sprintf(p, "LCD :BYD\n");
            break;		
	     case LCD_JDI:
            p += sprintf(p, "LCD :JDI\n");
            break;	
        case LCD_HITACHI_VIDEO:
            p += sprintf(p, "LCD :Hitachi_video\n");
            break;
		 case LCD_TRULY_OLD:
            p += sprintf(p, "LCD :TRULY_OLD\n");
            break;	
	    case LCD_TRULY_NEW:
            p += sprintf(p, "LCD :TRULY_NEW\n");
			break;
		case LCD_NOVTEK:
            p += sprintf(p, "LCD :LCD_NOVTEK\n");		
            break;
		case LCD_TIANMA:
            p += sprintf(p, "LCD :LCD_TIANMA\n");
			break;			
        case LCD_TRULY_COMMAND:
            p += sprintf(p, "LCD :TRULY_COMMAND\n");
			break;
		case LCD_TRULY_VIDEO:
            p += sprintf(p, "LCD :TRULY_VIDEO\n");
			break;
        case LCD_BYD_COMMAND:
            p += sprintf(p, "LCD :BYD_COMMAND\n");
			break;
		case LCD_BYD_VIDEO:
            p += sprintf(p, "LCD :BYD_VIDEO\n");
            break;
		case LCD_BYD_JDI_NT35521:
            p += sprintf(p, "LCD :BYD_JDI_NT35521\n");		
            break;	
		case LCD_TRULY_LG_NT35521:
            p += sprintf(p, "LCD :TRULY_LG_NT35521\n");		
            break;	
		case LCD_TRULY_LG_HX8394:
            p += sprintf(p, "LCD :TRULY_LG_HX8394\n");		
            break;	
		case LCD_TRULY_JDI_NT35521:
            p += sprintf(p, "LCD :TRULY_JDI_NT35521\n");
        /* Xinqin.Yang@PhoneSW.Multimedia, 2014/09/20  Add for 14053 JDI panel begin*/
            break;
        case LCD_JDI_R63417_VIDEO:
            p += sprintf(p, "LCD :JDI_R63417_VIDEO\n");
            break;
        /* Xinqin.Yang@PhoneSW.Multimedia, 2014/09/20  Add for 14053 JDI panel end*/
        /* Xinqin.Yang@PhoneSW.Multimedia, 2015/02/08  Add for 15011 SAMSUNG panel begin*/
        case LCD_SAMSUNG_S6E3FA3_CMD:
            p += sprintf(p, "LCD :LCD_SAMSUNG_S6E3FA3_CMD\n");
            break;
        /* Xinqin.Yang@PhoneSW.Multimedia, 2015/02/08  Add for 15011 SAMSUNG panel end*/
#ifdef VENDOR_EDIT
/* liuyan@Onlinerd.driver, 2014/11/28  Add for 14007 oled panel */
        case LCD_OLED_S6E3FA2_CMD:
            p += sprintf(p, "LCD :OLED_S6E3FA2_CMD\n");
#endif /*CONFIG_VENDOR_EDIT*/
            break;
		default:
            p += sprintf(p, "LCD :unknown\n");
            break;
    }
    
    switch(oppo_tp_dev)
    {
        case TP_NONE:
            p += sprintf(p, "TP  :None\n");
            break;
        case TP_ALPS:
            p += sprintf(p, "TP  :ALPS FW:0x%x\n",TP_FW);
            break;
        case TP_TRULY:
            p += sprintf(p, "TP  :TRULY 0x%x\n",TP_FW);
            break;
		case TP_YOUNGFAST:		
            p += sprintf(p, "TP  :YOUNGFAST 0x%x\n",TP_FW);
			break;
		case TP_OFILM:		
            p += sprintf(p, "TP  :OFILM 0x%x\n",TP_FW);		
			break;
		case TP_TPK:		
            p += sprintf(p, "TP  :TPK 0x%x\n",TP_FW);	
            break;
		case TP_NITTO:
            p += sprintf(p, "TP  :NITTO 0x%x\n",TP_FW);
            break;
        case TP_OIKE:
            p += sprintf(p, "TP  :OIKE 0x%x\n",TP_FW);
            break;	
		case TP_JDI:
            p += sprintf(p, "TP  :JDI 0x%x\n",TP_FW);
            break;		
		case TP_OFILM_WHITE:
            p += sprintf(p, "TP  :OFILM_WHITE 0x%x\n",TP_FW);
			break;
		case TP_OFILM_BLACK:
            p += sprintf(p, "TP  :OFILM_BLACK 0x%x\n",TP_FW);
			break;
		case TP_TPK_WHITE:
            p += sprintf(p, "TP  :TPK_WHITE 0x%x\n",TP_FW);
			break;
		case TP_TPK_BLACK:
            p += sprintf(p, "TP  :TPK_BLACK 0x%x\n",TP_FW);
			break;	
		case TP_TPK_GOODIX:
            p += sprintf(p, "TP  :TP_TPK_GOODIX Base_ID=0x%x Config_ID=0x%x \n",TP_FW,TP_FW_CONFIG_ID);
			break;	
		case TP_TRULY_GOODIX:
            p += sprintf(p, "TP  :TP_TRULY_GOODIX Base_ID=0x%x Config_ID=0x%x \n",TP_FW,TP_FW_CONFIG_ID);
			break;
		case TP_TPK_SYNAPTICS:
            p += sprintf(p, "TP  :TP_TPK_SYNAPTICS 0x%x\n",TP_FW);
			break;
		case TP_TRULY_SYNAPTICS:
            p += sprintf(p, "TP  :TP_TRULY_SYNAPTICS 0x%x\n",TP_FW);
			break;		
		case TP_SAMSUNG_SYNAPTICS:
            p += sprintf(p, "TP  :TP_SAMSUNG_SYNAPTICS 0x%x\n",TP_FW);
			break;		
        default:
            p += sprintf(p, "TP  :unknown\n");
            break;
    }
    
    switch(camera_back_dev)
    {
        case CAMERA_BACK_NONE:
            p += sprintf(p, "Cam_b:None\n");
            break;
        case CAMERA_BACK_OV5650MIPI:
            p += sprintf(p, "Cam_b:OV5650MIPI\n");
            break;
        case CAMERA_BACK_OV5647:
            p += sprintf(p, "Cam_b:OV5647\n");
            break;
	 	case CAMERA_BACK_OV5647AC:
            p += sprintf(p, "Cam_b:OV5647AC\n");
            break;
        case CAMERA_BACK_S5K4E5YA:
            p += sprintf(p, "Cam_b:S5K4E5YA\n");
            break;
        case CAMERA_BACK_IMX105MIPI:
            p += sprintf(p, "Cam_b:IMX105MIPI\n");
            break;
	// lingjianing@CameraDrv, 2013/10/28, add for 13065 test mode and get firmware version	
	  case CAMERA_BACK_MS2R: 
            p += sprintf(p, "Cam_b  :MS2RMIPI 0x%x\n",MS2R_FW);
            break;
        case CAMERA_BACK_VD6803A:
		    p += sprintf(p,"Cam_b:VD6803A\n");
		    break;
		//zhengrong.zhang@CameraDrv, 2013/09/24, add for 13059 test mode
    	case CAMERA_BACK_IMX179:
		    p += sprintf(p,"Cam_b:IMX179\n");
		    break;
		//bin.liu@CameraDrv, 2014/11/18, add for 14053 test mode
		case CAMERA_BACK_IMX214:
		    p += sprintf(p,"Cam_b:IMX214\n");
		    break;
		/*oppo hufeng 20150515 add for device list*/
		case CAMERA_BACK_IMX278:
		    p += sprintf(p,"Cam_b:IMX278\n");
		    break;
		//xianglie.liu@CameraDrv, 2013/12/24, add for 13085 factory mode
    	case CAMERA_BACK_OV5648:
		    p += sprintf(p,"Cam_b:OV5648\n");
		    break;
    	//zhangkw add for 15011 factory mode			
    	case CAMERA_BACK_S5K3M2:
		    p += sprintf(p,"Cam_b:S5K3M2\n");
		    break;		
        default:
            p += sprintf(p, "Cam_b:unknown\n");
            break;
    }
    
    switch(camera_front_dev)
    {
        case CAMERA_FRONT_NONE:
            p += sprintf(p, "Cam_f:None\n");
            break;
        case CAMERA_FRONT_mt9d115:
            p += sprintf(p, "Cam_f:mt9d115\n");
            break;
        case CAMERA_FRONT_s5k5bbgx:
            p += sprintf(p, "Cam_f:s5k5bbgx\n");
            break;
		//lvxj@MutimediaDrv.camsensor, 2012/08/31, add for 12021 test mode	
        case CAMERA_FRONT_ov7675:
            p += sprintf(p, "Cam_f:ov7675\n");
            break;
        case CAMERA_FRONT_hi704:
            p += sprintf(p, "Cam_f:hi704\n");
            break;
        case CAMERA_FRONT_HI253:
            p += sprintf(p, "Cam_f:HI253\n");
            break;
		//zhengrong.zhang@CameraDrv, 2013/09/24, add for 13059 test mode
		case CAMERA_FRONT_HI256:
            p += sprintf(p, "Cam_f:HI256\n");
            break;
        case CAMERA_FRONT_OV5647:
            p += sprintf(p, "Cam_f:OV5647\n");
            break;
		//zhangkw@CameraDrv, 2013/06/08, add for test mode 13003		
	 	case CAMERA_FRONT_OV5693:
            p += sprintf(p, "Cam_f:OV5693\n");
            break;
	// lingjianing@CameraDrv, 2013/10/28, add for 13065 test mode	
	case CAMERA_FRONT_OV5648:  //=====
            p += sprintf(p, "Cam_f:OV5648\n");
            break;
	// zhangkw add factory mode for 15011
	case CAMERA_FRONT_OV8858: 
            p += sprintf(p, "Cam_f:OV8858\n");
            break;			
        default:
            p += sprintf(p, "Cam_f:unknown\n");
            break;
    }
    //McpIdInit();
    /*switch(mcp_dev)
    {
        case MCP_NONE:
            p += sprintf(p, "mcp :None\n");
            break;
        case MCP_KMSJS000KM_B308:
            p += sprintf(p, "mcp :EMMC_KMSJS000KM_B308\n");
            break;
        case MCP_H9DP32A4JJACGR_KEM:
            p += sprintf(p, "mcp :EMMC_H9DP32A4JJACGR_KEM\n");
            break;
        case MCP_KMNJS000ZM_B205:
            p += sprintf(p, "mcp :EMMC_KMNJS000ZM_B205\n");
            break;
        case MCP_H9TP32A4GDMCPR_KDM:
            p += sprintf(p, "mcp :EMMC_H9TP32A4GDMCPR_KDM\n");
            break;
        default:
            p += sprintf(p, "mcp :unknown\n");
            break;
    }*/
/*for als ps dev*/
	switch(alsps_dev)
	{
		case ALSPS_NONE:
			p += sprintf(p, "ALS_PS:none\n");
			break;
		case ALSPS_STK31XX:
			p += sprintf(p, "ALS_PS:stk31xx\n");
			break;
		case ALSPS_STK3X1X:
			p += sprintf(p, "ALS_PS:stk3x1x\n");
			break;
		case ALSPS_TAOS_277X:
			p += sprintf(p, "ALS_PS:tmd277x\n");
			break;
		case ALSPS_LITE_558:
			p += sprintf(p, "ALS_PS:ltr558\n");
			break;
		case ALSPS_GP2AAP052A:
			p += sprintf(p, "ALS_PS:gp2ap052a\n");
			break;	
		case ALSPS_TAOS_TMG399X:
			p += sprintf(p, "ALS_PS:tmg399x\n");
			break;	
		case ALSPS_TMD_27723:
			p += sprintf(p, "ALS_PS:tmd27723\n");
			break;
		case ALSPS_CM_36686:
			p += sprintf(p, "ALS_PS:cm36686\n");
			break;
		case ALSPS_APDS_9921:
			p += sprintf(p, "ALS_PS:apds9921\n");
			break;
		default :
			p += sprintf(p, "ALS_PS:unknown\n");

	}

	switch(gyro_dev)
	{
		case GYRO_NONE:
			p += sprintf(p, "GYRO:None\n");
			break;
		case GYRO_MISS:
			p += sprintf(p, "GYRO:Miss\n");
			break;			
		case GYRO_MPU6050C:
			p += sprintf(p, "GYRO:MPU6050C\n");
			break;
		case GYRO_L3GD20:
			p += sprintf(p, "GYRO:L3GD20\n");
			break;
		case GYRO_MMC_PG:
			p += sprintf(p, "GYRO:MMC_PG\n");
			break;			
		default :
			p += sprintf(p, "GYRO:Unknown\n");
	}

	switch(oppo_bkl_dev)
	{
		case BKL_NONE:
			p += sprintf(p, "back light:None\n");
			break;
		case BKL_LM3580:
			p += sprintf(p, "back light:LM3580\n");
			break;			
		case BKL_LM3630:
			p += sprintf(p, "back light:LM3630\n");
			break;
		default :
			p += sprintf(p, "back light:Unknown\n");
	}


	/*****************************add end**************************************/
	
	len = p - page;
	if (len > *pos)
		len -= *pos;
	else
		len = 0;
	if (copy_to_user(buf,page,len < count ? len  : count))
		return -EFAULT;
	*pos = *pos + (len < count ? len  : count);
	return len < count ? len  : count;

}

static const struct file_operations oppo_devices_list = {
	.write		= oppo_devices_list_write,
	.read		= oppo_devices_list_read,
};

static int oppo_smallboard = 0;


static ssize_t oppo_smallboard_id_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{

    char temp_buffer[2];
	int num_read_chars = 0;
	
    num_read_chars += sprintf(temp_buffer, "%d\n",oppo_smallboard);
	num_read_chars = simple_read_from_buffer(buf, count, pos, temp_buffer, strlen(temp_buffer));
	return num_read_chars; 
}


static ssize_t oppo_smallboard_id_write(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	/*not support write now*/
	return count;
}
static const struct file_operations oppo_smallboard_id = {
	.write		= oppo_smallboard_id_write,
	.read		= oppo_smallboard_id_read,
};

static int oppo_dev_platform_probe(struct platform_device *pdev)
{
  
    int id0 = 0, id1 = 0;
  
   
	
	id0 = mt_get_gpio_in(GPIO_SUB_HW_ID0_PIN);
	id1 = mt_get_gpio_in(GPIO_SUB_HW_ID1_PIN);
	
	printk("id0 = %d, id1 = %d\n",id0, id1);
	#if defined(OPPO_CMCC_TEST) || defined(OPPO_CMCC_MP) 
	if(id0 == 0 && id1 == 1)
		oppo_smallboard = 1;
	#else
	if(id0 == 1 && id1 == 1)
		oppo_smallboard = 1;
	#endif
	
	
	proc_create("oppo_devices_list", 0666, NULL, &oppo_devices_list);
	proc_create("oppo_smallboard_id", 0666, NULL, &oppo_smallboard_id);
   
    return 0;
}

static struct platform_driver oppo_dev_platform_driver = {
    //.remove     = NULL,
    //.shutdown   = NULL,
    .probe      = oppo_dev_platform_probe,
    //#ifndef CONFIG_HAS_EARLYSUSPEND
    //.suspend    = NULL,
    .resume     = NULL,
   // #endif
    .driver     = {
    	.owner = THIS_MODULE,
        .name = "oppo_dev_platform_driver",
    },
};
static struct platform_device oppo_dev_platform_device = {
	.name = "oppo_dev_platform_driver",
	.id = -1
};

static int __init oppo_devices_list_init(void)
{
    int ret;
	ret = platform_device_register(&oppo_dev_platform_device);
	if (ret)
		printk("oppo_dev_platform_device:dev:E%d\n", ret);

	ret = platform_driver_register(&oppo_dev_platform_driver);

	if (ret)
	{
		printk("oppo_dev_platform_driver:drv:E%d\n", ret);
		platform_device_unregister(&oppo_dev_platform_device);
		return ret;
	}
	return 0;
}
static void __exit oppo_devices_list_exit(void)
{
	platform_driver_unregister(&oppo_dev_platform_driver);
	platform_device_unregister(&oppo_dev_platform_device);
}
module_init(oppo_devices_list_init);

