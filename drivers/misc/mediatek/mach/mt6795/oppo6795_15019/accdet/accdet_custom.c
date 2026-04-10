#include "accdet_custom_def.h"
#include <accdet_custom.h>

//key press customization: long press time
/*xiang.fei@Multimedia, 2015/04/14, Modify for headset*/
#if defined(OPPO_MTK_15011) ||  defined(OPPO_MTK_15019)
struct headset_key_custom headset_key_custom_setting = {
	500
};
#else /* OPPO_MTK_15019 */
struct headset_key_custom headset_key_custom_setting = {
	2000
};
#endif /* OPPO_MTK_15019 */
struct headset_key_custom* get_headset_key_custom_setting(void)
{
	return &headset_key_custom_setting;
}

#if defined  ACCDET_EINT || defined ACCDET_EINT_IRQ
/*xiang.fei@Multimedia, 2015/04/20, Modify for headset*/
#if defined(OPPO_MTK_15011) ||  defined(OPPO_MTK_15019)
static struct headset_mode_settings cust_headset_settings = {
	0x900, 0x900, 1, 0x1f0, 0x800, 0x800, 0x20
};
#else /* OPPO_MTK_15019 */
static struct headset_mode_settings cust_headset_settings = {
	0x900, 0x200, 1, 0x1f0, 0x800, 0x800, 0x20
};
#endif /* OPPO_MTK_15019 */
#else
//ACCDET only mode register settings
static struct headset_mode_settings cust_headset_settings = {
	0x900, 0x600, 1, 0x5f0, 0x3000, 0x3000, 0x400
};
#endif

struct headset_mode_settings* get_cust_headset_settings(void)
{
	return &cust_headset_settings;
}