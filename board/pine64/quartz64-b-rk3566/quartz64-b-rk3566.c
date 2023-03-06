// SPDX-License-Identifier: GPL-2.0+

#include <asm/io.h>

#define PIPE_GRF_BASE_ADDR	0xfdc50000
#define PIPE_GRF_USB3OTG0_CON1	(0x104)

int rk_board_late_init(void)
{
	/* setup usb_otg clock */
	writel(0xffff0181, PIPE_GRF_BASE_ADDR + PIPE_GRF_USB3OTG0_CON1);

	return 0;
}
