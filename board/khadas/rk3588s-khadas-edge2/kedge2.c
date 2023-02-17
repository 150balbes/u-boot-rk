/*
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * (C) Copyright 2022 Wesion Technology Co., Ltd
 */

#include <common.h>
#include <dwc3-uboot.h>
#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_USB_DWC3
static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = 0xfc000000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
	.usb2_phyif_utmi_width = 16,
};

int usb_gadget_handle_interrupts(void)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	return dwc3_uboot_init(&dwc3_device_data);
}
#endif

int rk_board_late_init(void)
{
	// Set FAN test
	run_command("i2c dev 2; i2c mw 18 96 1", 0);

	// Set Green LED on
	run_command("gpio set 138; gpio clear 139; gpio clear 140", 0);

	return 0;
}
