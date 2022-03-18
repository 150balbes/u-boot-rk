// SPDX-License-Identifier: GPL-2.0+

#include <asm/io.h>

#define PMU_BASE_ADDR	0xfdd90000
#define PMU_PWR_CON	(0x04)

int rk_board_late_init(void)
{
	/* set pmu_sleep_pol to low */
	writel(0x80008000, PMU_BASE_ADDR + PMU_PWR_CON);

	return 0;
}
