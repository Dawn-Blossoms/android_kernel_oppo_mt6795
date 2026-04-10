#ifndef __CUST_ALSPS_H__
#define __CUST_ALSPS_H__

#include <linux/types.h>

#define C_CUST_ALS_LEVEL    16
#define C_CUST_I2C_ADDR_NUM 4

#define MAX_THRESHOLD_HIGH 0xffff
#define MIN_THRESHOLD_LOW 0x0

#define PS_ADJUST_TREND_STATE       0
#define PS_ADJUST_NOTREND_STATE     1
#define PS_ADJUST_HIGHLIGHT_STATE   2
#define PS_ADJUST_AVOID_DIRTY_STATE 3

struct set_ps_thd_para {
    int low_threshold;
    int high_threshold;
    int ps_average;
    int algo_state; //PS_ADJUST_XXX_STATE
};

struct ps_adjust_para {
    int ps_up;
    int ps_thd_low_notrend;
    int ps_thd_high_notrend; //TODO when ps_up != ps_thd_high
    int ps_thd_low_trend;
    int ps_thd_high_trend;
    int ps_thd_low_highlight;
    int ps_thd_high_highlight;
    int ps_adjust_min;
    int ps_adjust_max;
    int highlight_limit;
    int sampling_time; //Unit:ms
    int sampling_count;
    int dirty_adjust_limit;
    int dirty_adjust_low_thd;
    int dirty_adjust_high_thd;
};

#if 0 
struct alsps_hw {
    int i2c_num;                                    /*!< the i2c bus used by ALS/PS */
    int power_id;                                   /*!< the VDD power id of the als chip */
    int power_vol;                                  /*!< the VDD power voltage of the als chip */
	int polling_mode;                               /*!< 1: polling mode ; 0:interrupt mode*/
	int polling_mode_ps;                               /*!< 1: polling mode ; 0:interrupt mode*/
	int polling_mode_als;                               /*!< 1: polling mode ; 0:interrupt mode*/
    unsigned char   i2c_addr[C_CUST_I2C_ADDR_NUM];  /*!< i2c address list, some chip will have multiple address */
    unsigned int    als_level[C_CUST_ALS_LEVEL-1];  /*!< (C_CUST_ALS_LEVEL-1) levels divides all range into C_CUST_ALS_LEVEL levels*/
    unsigned int    als_value[C_CUST_ALS_LEVEL];    /*!< the value reported in each level */
    unsigned int    ps_threshold;                   /*!< the threshold of proximity sensor */
	unsigned int    als_window_loss;                /*!< the window loss  */
	unsigned int    ps_threshold_high;
	unsigned int    ps_threshold_low;
	unsigned int    als_threshold_high;
	unsigned int    als_threshold_low;
    int als_power_vio_id;                                   /*!< the VIO power id of the als chip */
    int als_power_vio_vol;                                  /*!< the VIO power voltage of the als chip */
    int ps_power_vdd_id;                                   /*!< the VDD power id of the ps chip */
    int ps_power_vdd_vol;                                  /*!< the VDD power voltage of the ps chip */
    int ps_power_vio_id;                                   /*!< the VIO power id of the ps chip */
    int ps_power_vio_vol;                                  /*!< the VIO power voltage of the ps chip */
    int power_lp_mode_ctrl;                                 /*!< 1: disable ldo low power mode when p sensor enabled ; 0: no action*/
    bool is_batch_supported_ps;
    bool is_batch_supported_als;
    int (*power)(struct alsps_hw *hw, unsigned int on, char *devname);
   struct ps_adjust_para *p_ps_adjust_para;
};
#endif

struct alsps_hw {
    int i2c_num;                                    /*!< the i2c bus used by ALS/PS */
    int power_id;                                   /*!< the VDD power id of the als chip */
    int power_vol;                                  /*!< the VDD power voltage of the als chip */
    int polling_mode;                               /*!< 1: polling mode ; 0:interrupt mode*/
    int polling_mode_ps;                               /*!< 1: polling mode ; 0:interrupt mode*/
    int polling_mode_als;                               /*!< 1: polling mode ; 0:interrupt mode*/ 
    int polling_mode_gesture;                               /*!< 1: polling mode ; 0:interrupt mode*/
    unsigned char   i2c_addr[C_CUST_I2C_ADDR_NUM];  /*!< i2c address list, some chip will have multiple address */
    unsigned int    als_level[C_CUST_ALS_LEVEL-1];  /*!< (C_CUST_ALS_LEVEL-1) levels divides all range into C_CUST_ALS_LEVEL levels*/
    unsigned int    als_value[C_CUST_ALS_LEVEL];    /*!< the value reported in each level */
    unsigned int    ps_threshold;                   /*!< the threshold of proximity sensor */
    unsigned int    als_window_loss;                /*!< the window loss  */
    unsigned int    ps_threshold_high;
    unsigned int    ps_threshold_low;
    unsigned int    als_threshold_high;
    unsigned int    als_threshold_low;
    unsigned int	state_val;
    unsigned int 	psctrl_val;
    unsigned int 	alsctrl_val;
    unsigned int 	ledctrl_val;
    unsigned int 	wait_val;	
    unsigned int    ps_high_thd_val;
    unsigned int    ps_low_thd_val;
	
    int als_power_vio_id;                                   /*!< the VIO power id of the als chip */
    int als_power_vio_vol;                                  /*!< the VIO power voltage of the als chip */
    int ps_power_vdd_id;                                   /*!< the VDD power id of the ps chip */
    int ps_power_vdd_vol;                                  /*!< the VDD power voltage of the ps chip */
    int ps_power_vio_id;                                   /*!< the VIO power id of the ps chip */
    int ps_power_vio_vol;                                  /*!< the VIO power voltage of the ps chip */
    int power_lp_mode_ctrl;                                 /*!< 1: disable ldo low power mode when p sensor enabled ; 0: no action*/
    bool is_batch_supported_ps;
    bool is_batch_supported_als;
    int (*power)(struct alsps_hw *hw, unsigned int on, char *devname);
    struct ps_adjust_para *p_ps_adjust_para;
    struct ps_adjust_para *p_ps_adjust_para_2;
};

extern struct alsps_hw* get_cust_alsps_hw(void);
//extern struct alsps_hw* apds9921_get_cust_alsps_hw(void);

__weak int pmic_ldo_suspend_enable(int enable);
#endif
