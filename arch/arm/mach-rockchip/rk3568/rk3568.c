// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2021 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <fdt_support.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <asm/arch-rockchip/bootrom.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/grf_rk3568.h>
#include <asm/arch-rockchip/hardware.h>
#include <dt-bindings/clock/rk3568-cru.h>

#define PMUGRF_BASE			0xfdc20000
#define GRF_BASE			0xfdc60000
#define GRF_GPIO1B_IOMUX_H		0x0c
#define GRF_GPIO1C_IOMUX_L		0x10
#define GRF_GPIO1C_IOMUX_H		0x14
#define GRF_GPIO1D_IOMUX_L		0x18
#define GRF_GPIO1D_IOMUX_H		0x1c
#define GRF_GPIO2A_IOMUX_L		0x20
#define GRF_GPIO1B_DS_2			0x218
#define GRF_GPIO1B_DS_3			0x21c
#define GRF_GPIO1C_DS_0			0x220
#define GRF_GPIO1C_DS_1			0x224
#define GRF_GPIO1C_DS_2			0x228
#define GRF_GPIO1C_DS_3			0x22c
#define GRF_GPIO1D_DS_0			0x230
#define GRF_GPIO1D_DS_1			0x234
#define GRF_GPIO1D_DS_2			0x238
#define SGRF_BASE			0xfdd18000
#define SGRF_SOC_CON3			0x0c
#define SGRF_SOC_CON4			0x10
#define EMMC_HPROT_SECURE_CTRL		0x03
#define SDMMC0_HPROT_SECURE_CTRL	0x01

#define PMU_BASE_ADDR		0xfdd90000
#define PMU_NOC_AUTO_CON0	(0x70)
#define PMU_NOC_AUTO_CON1	(0x74)
#define EDP_PHY_GRF_BASE	0xfdcb0000
#define EDP_PHY_GRF_CON0	(EDP_PHY_GRF_BASE + 0x00)
#define EDP_PHY_GRF_CON10	(EDP_PHY_GRF_BASE + 0x28)
#define CPU_GRF_BASE		0xfdc30000
#define GRF_CORE_PVTPLL_CON0	(0x10)

/* PMU_GRF_GPIO0D_IOMUX_L */
enum {
	GPIO0D1_SHIFT		= 4,
	GPIO0D1_MASK		= GENMASK(6, 4),
	GPIO0D1_GPIO		= 0,
	GPIO0D1_UART2_TXM0,

	GPIO0D0_SHIFT		= 0,
	GPIO0D0_MASK		= GENMASK(2, 0),
	GPIO0D0_GPIO		= 0,
	GPIO0D0_UART2_RXM0,
};

/* GRF_IOFUNC_SEL3 */
enum {
	UART2_IO_SEL_SHIFT	= 10,
	UART2_IO_SEL_MASK	= GENMASK(11, 10),
	UART2_IO_SEL_M0		= 0,
};

const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
	[BROM_BOOTSOURCE_EMMC] = "/mmc@fe310000",
	[BROM_BOOTSOURCE_SPINOR] = "/spi@fe300000/flash@0",
	[BROM_BOOTSOURCE_SD] = "/mmc@fe2b0000",
};

static struct mm_region rk3568_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0xf0000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0xf0000000UL,
		.phys = 0xf0000000UL,
		.size = 0x10000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x300000000,
		.phys = 0x300000000,
		.size = 0x0c0c00000,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = rk3568_mem_map;

void board_debug_uart_init(void)
{
	static struct rk3568_pmugrf * const pmugrf = (void *)PMUGRF_BASE;
	static struct rk3568_grf * const grf = (void *)GRF_BASE;

	/* UART2 M0 */
	rk_clrsetreg(&grf->iofunc_sel3, UART2_IO_SEL_MASK,
		     UART2_IO_SEL_M0 << UART2_IO_SEL_SHIFT);

	/* Switch iomux */
	rk_clrsetreg(&pmugrf->pmu_gpio0d_iomux_l,
		     GPIO0D1_MASK | GPIO0D0_MASK,
		     GPIO0D1_UART2_TXM0 << GPIO0D1_SHIFT |
		     GPIO0D0_UART2_RXM0 << GPIO0D0_SHIFT);
}

int arch_cpu_init(void)
{
#ifdef CONFIG_SPL_BUILD
	/*
	 * When perform idle operation, corresponding clock can
	 * be opened or gated automatically.
	 */
	writel(0xffffffff, PMU_BASE_ADDR + PMU_NOC_AUTO_CON0);
	writel(0x000f000f, PMU_BASE_ADDR + PMU_NOC_AUTO_CON1);

	/* Disable eDP phy by default */
	writel(0x00070007, EDP_PHY_GRF_CON10);
	writel(0x0ff10ff1, EDP_PHY_GRF_CON0);

	/* Set core pvtpll ring length */
	writel(0x00ff002b, CPU_GRF_BASE + GRF_CORE_PVTPLL_CON0);

	/* Set the emmc sdmmc0 to secure */
	rk_clrreg(SGRF_BASE + SGRF_SOC_CON4, (EMMC_HPROT_SECURE_CTRL << 11
		| SDMMC0_HPROT_SECURE_CTRL << 4));
	/* set the emmc driver strength to level 2 */
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1B_DS_2);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1B_DS_3);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1C_DS_0);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1C_DS_1);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1C_DS_2);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1C_DS_3);

	/* emmc, sfc, and sdmmc iomux */
	writel((0x7777UL << 16) | (0x1111), GRF_BASE + GRF_GPIO1B_IOMUX_H);
	writel((0x7777UL << 16) | (0x1111), GRF_BASE + GRF_GPIO1C_IOMUX_L);
	writel((0x7777UL << 16) | (0x2111), GRF_BASE + GRF_GPIO1C_IOMUX_H);
	writel((0x7777UL << 16) | (0x1111), GRF_BASE + GRF_GPIO1D_IOMUX_L);
	writel((0x7777UL << 16) | (0x1111), GRF_BASE + GRF_GPIO1D_IOMUX_H);
	writel((0x7777UL << 16) | (0x1111), GRF_BASE + GRF_GPIO2A_IOMUX_L);

	/* set the fspi d0~3 cs0 to level 2 */
	writel(0x3f000700, GRF_BASE + GRF_GPIO1C_DS_3);
	writel(0x3f000700, GRF_BASE + GRF_GPIO1D_DS_0);
	writel(0x3f3f0707, GRF_BASE + GRF_GPIO1D_DS_1);
	writel(0x003f0007, GRF_BASE + GRF_GPIO1D_DS_2);

	/* Set the fspi to secure */
	writel(((0x1 << 14) << 16) | (0x0 << 14), SGRF_BASE + SGRF_SOC_CON3);

#endif
	return 0;
}

#ifdef CONFIG_OF_SYSTEM_SETUP
int ft_system_setup(void *blob, struct bd_info *bd)
{
	int ret;
	int areas = 1;
	u64 start[2], size[2];

	/* Reserve the io address space. */
	if (gd->ram_top > SDRAM_UPPER_ADDR_MIN) {
		start[0] = gd->bd->bi_dram[0].start;
		size[0] = SDRAM_LOWER_ADDR_MAX - gd->bd->bi_dram[0].start;

		/* Add the upper 4GB address space */
		start[1] = SDRAM_UPPER_ADDR_MIN;
		size[1] = gd->ram_top - SDRAM_UPPER_ADDR_MIN;
		areas = 2;

		ret = fdt_set_usable_memory(blob, start, size, areas);
		if (ret) {
			printf("Cannot set usable memory\n");
			return ret;
		}
	}

	return 0;
};
#endif

#ifdef CONFIG_SPL_BUILD

void __weak led_setup(void)
{
}

void spl_board_init(void)
{
	led_setup();

#if defined(SPL_DM_REGULATOR)
	/*
	 * Turning the eMMC and SPI back on (if disabled via the Qseven
	 * BIOS_ENABLE) signal is done through a always-on regulator).
	 */
	if (regulators_enable_boot_on(false))
		debug("%s: Cannot enable boot on regulator\n", __func__);
#endif

	setup_boot_mode();
}
#endif

#if defined(CONFIG_USB_GADGET)
#include <usb.h>

#if defined(CONFIG_USB_DWC3_GADGET) && !defined(CONFIG_DM_USB_GADGET)
#include <dwc3-uboot.h>

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = 0xfcc00000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
	.hsphy_mode = USBPHY_INTERFACE_MODE_UTMIW,
};

int usb_gadget_handle_interrupts(int index)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	return dwc3_uboot_init(&dwc3_device_data);
}
#endif /* CONFIG_USB_DWC3_GADGET */

#endif /* CONFIG_USB_GADGET */
