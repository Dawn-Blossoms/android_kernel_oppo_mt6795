
include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
ARCH_MTK_PROJECT := $(shell echo $(CONFIG_ARCH_MTK_PROJECT))
obj-y   += core/
obj-y   += dct/dct/
endif

ifeq ($(CONFIG_MTK_SENSOR_SUPPORT),y)

ifeq ($(CONFIG_MTK_STK3310_SA),y)
obj-y	+=  alsps/stk3310-sa/
endif

ifeq ($(CONFIG_MTK_LIS3DH),y)
obj-y   +=  accelerometer/lis3dh-new/
endif

ifeq ($(CONFIG_MTK_MPU6050GY_NEW),y)
obj-y   +=  gyroscope/mpu6050gy-new/
endif

ifeq ($(CONFIG_MTK_MMC3416X),y)
obj-y	+=  magnetometer/mmc3416x-new/
endif

ifeq ($(CONFIG_MTK_BMP180_NEW),y)
obj-y	+=  barometer/BMP180-new/
endif
endif
obj-y   +=  camera/camera/
obj-y   +=  vibrator/
obj-y   +=  leds/
obj-y   +=  keypad/
obj-y   +=  sound/
obj-y   +=  accdet/

ifeq ($(CONFIG_MTK_HDMI_SUPPORT),y)
obj-y	+=  hdmi/
endif

