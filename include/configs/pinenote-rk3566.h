/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __PINENOTE_RK3566_H
#define __PINENOTE_RK3566_H

#include <configs/rk3568_common.h>

#define CONFIG_SUPPORT_EMMC_RPMB

#define ROCKCHIP_DEVICE_SETTINGS \
			"stdout=serial,vidconsole\0" \
			"stderr=serial,vidconsole\0"

#define CONFIG_USB_OHCI_NEW
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS     2

#endif
