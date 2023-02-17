/*
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * Copyright (c) 2022 Wesion Technology Co., Ltd
 */

#ifndef __CONFIGS_KEDGE2_H
#define __CONFIGS_KEDGE2_H

#include <configs/rk3588_common.h>

#ifndef CONFIG_SPL_BUILD

#undef ROCKCHIP_DEVICE_SETTINGS
#define ROCKCHIP_DEVICE_SETTINGS \
		"logo_addr_r=0x07000000\0" \
		"logo_file_dir=/boot\0" \
		"logocmd_mmc=load $devtype $devnum:1 $logo_addr_r $logo_file_dir/$logo_file || load $devtype $devnum:2 $logo_addr_r $logo_file_dir/$logo_file\0" \
		"logocmd_usb=run logocmd_mmc\0" \
		"logocmd=echo Load logo: $logo_file_dir/$logo_file; run logocmd_${devtype}\0" \
		"stdout=serial,vidconsole\0" \
		"stderr=serial,vidconsole\0" \
		"update=gpio clear 138; gpio clear 139; gpio set 140; rockusb 0 mmc 0\0"

#define CONFIG_SYS_MMC_ENV_DEV		0

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND  "run distro_bootcmd"

#undef BOOTENV_BOOT_TARGETS
#define BOOTENV_BOOT_TARGETS \
    "boot_targets=usb0 mmc1 mmc0 pxe dhcp\0"

#endif
#endif
