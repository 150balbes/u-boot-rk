// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2015 Google, Inc
 *
 * (C) Copyright 2008-2020 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 * Jianqun Xu, Software Engineering, <jay.xu@rock-chips.com>.
 */

#include <common.h>
#include <dm.h>
#include <dm/of_access.h>
#include <dm/device_compat.h>
#include <syscon.h>
#include <linux/errno.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/gpio.h>
#include <dm/pinctrl.h>
#include <dt-bindings/clock/rk3288-cru.h>

#include "../pinctrl/rockchip/pinctrl-rockchip.h"

#define OFFSET_TO_BIT(bit)	(1UL << (bit))

#ifdef CONFIG_ROCKCHIP_GPIO_V2
#define REG_L(R)	(R##_l)
#define REG_H(R)	(R##_h)
#define READ_REG(REG)	((readl(REG_L(REG)) & 0xFFFF) | \
			((readl(REG_H(REG)) & 0xFFFF) << 16))
#define WRITE_REG(REG, VAL)	\
{\
	writel(((VAL) & 0xFFFF) | 0xFFFF0000, REG_L(REG)); \
	writel((((VAL) & 0xFFFF0000) >> 16) | 0xFFFF0000, REG_H(REG));\
}
#define CLRBITS_LE32(REG, MASK)	WRITE_REG(REG, READ_REG(REG) & ~(MASK))
#define SETBITS_LE32(REG, MASK)	WRITE_REG(REG, READ_REG(REG) | (MASK))
#define CLRSETBITS_LE32(REG, MASK, VAL)	WRITE_REG(REG, \
				(READ_REG(REG) & ~(MASK)) | (VAL))

#else
#define READ_REG(REG)			readl(REG)
#define WRITE_REG(REG, VAL)		writel(VAL, REG)
#define CLRBITS_LE32(REG, MASK)		clrbits_le32(REG, MASK)
#define SETBITS_LE32(REG, MASK)		setbits_le32(REG, MASK)
#define CLRSETBITS_LE32(REG, MASK, VAL)	clrsetbits_le32(REG, MASK, VAL)
#endif


struct rockchip_gpio_priv {
	struct rockchip_gpio_regs *regs;
	struct udevice *pinctrl;
	int bank;
	char name[2];
};

static int rockchip_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	struct rockchip_gpio_priv *priv = dev_get_priv(dev);
	struct rockchip_gpio_regs *regs = priv->regs;

	CLRBITS_LE32(&regs->swport_ddr, OFFSET_TO_BIT(offset));

	return 0;
}

static int rockchip_gpio_direction_output(struct udevice *dev, unsigned offset,
					  int value)
{
	struct rockchip_gpio_priv *priv = dev_get_priv(dev);
	struct rockchip_gpio_regs *regs = priv->regs;
	int mask = OFFSET_TO_BIT(offset);

	CLRSETBITS_LE32(&regs->swport_dr, mask, value ? mask : 0);
	SETBITS_LE32(&regs->swport_ddr, mask);

	return 0;
}

static int rockchip_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct rockchip_gpio_priv *priv = dev_get_priv(dev);
	struct rockchip_gpio_regs *regs = priv->regs;

	return readl(&regs->ext_port) & OFFSET_TO_BIT(offset) ? 1 : 0;
}

static int rockchip_gpio_set_value(struct udevice *dev, unsigned offset,
				   int value)
{
	struct rockchip_gpio_priv *priv = dev_get_priv(dev);
	struct rockchip_gpio_regs *regs = priv->regs;
	int mask = OFFSET_TO_BIT(offset);

	CLRSETBITS_LE32(&regs->swport_dr, mask, value ? mask : 0);

	return 0;
}

static int rockchip_gpio_get_function(struct udevice *dev, unsigned offset)
{
#ifdef CONFIG_SPL_BUILD
	return -ENODATA;
#else
	struct rockchip_gpio_priv *priv = dev_get_priv(dev);
	struct rockchip_gpio_regs *regs = priv->regs;
	bool is_output;
	int ret;

	ret = pinctrl_get_gpio_mux(priv->pinctrl, priv->bank, offset);
	if (ret)
		return ret;
	is_output = READ_REG(&regs->swport_ddr) & OFFSET_TO_BIT(offset);
	
	return is_output ? GPIOF_OUTPUT : GPIOF_INPUT;
#endif
}

/* Simple SPL interface to GPIOs */
#if defined(CONFIG_SPL_BUILD) && !defined(CONFIG_ROCKCHIP_GPIO_V2)

enum {
	PULL_NONE_1V8 = 0,
	PULL_DOWN_1V8 = 1,
	PULL_UP_1V8 = 3,
};

int spl_gpio_set_pull(void *vregs, uint gpio, int pull)
{
	u32 *regs = vregs;
	uint val;

	regs += gpio >> GPIO_BANK_SHIFT;
	gpio &= GPIO_OFFSET_MASK;
	switch (pull) {
	case GPIO_PULL_UP:
		val = PULL_UP_1V8;
		break;
	case GPIO_PULL_DOWN:
		val = PULL_DOWN_1V8;
		break;
	case GPIO_PULL_NORMAL:
	default:
		val = PULL_NONE_1V8;
		break;
	}
	clrsetbits_le32(regs, 3 << (gpio * 2), val << (gpio * 2));

	return 0;
}

int spl_gpio_output(void *vregs, uint gpio, int value)
{
	struct rockchip_gpio_regs * const regs = vregs;

	clrsetbits_le32(&regs->swport_dr, 1 << gpio, value << gpio);

	/* Set direction */
	clrsetbits_le32(&regs->swport_ddr, 1 << gpio, 1 << gpio);

	return 0;
}
#endif /* CONFIG_SPL_BUILD */

static int rockchip_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct rockchip_gpio_priv *priv = dev_get_priv(dev);
	struct rockchip_pinctrl_priv *pctrl_priv;
	struct rockchip_pin_bank *bank;
	char *end = NULL;
	static int gpio;
	int id = -1, ret;

	priv->regs = dev_read_addr_ptr(dev);
	ret = uclass_first_device_err(UCLASS_PINCTRL, &priv->pinctrl);
	if (ret) {
		dev_err(dev, "failed to get pinctrl device %d\n", ret);
		return ret;
	}

	pctrl_priv = dev_get_priv(priv->pinctrl);
	if (!pctrl_priv) {
		dev_err(dev, "failed to get pinctrl priv\n");
		return -EINVAL;
	}

	end = strrchr(dev->name, '@');
	if (end)
		id = trailing_strtoln(dev->name, end);
	else
		dev_read_alias_seq(dev, &id);

	if (id < 0)
		id = gpio++;

	if (id >= pctrl_priv->ctrl->nr_banks) {
		dev_err(dev, "bank id invalid\n");
		return -EINVAL;
	}

	bank = &pctrl_priv->ctrl->pin_banks[id];
	if (bank->bank_num != id) {
		dev_err(dev, "bank id mismatch with pinctrl\n");
		return -EINVAL;
	}

	priv->bank = bank->bank_num;
	uc_priv->gpio_count = bank->nr_pins;
	uc_priv->gpio_base = bank->pin_base;
	uc_priv->bank_name = bank->name;

	return 0;
}

static const struct dm_gpio_ops gpio_rockchip_ops = {
	.direction_input	= rockchip_gpio_direction_input,
	.direction_output	= rockchip_gpio_direction_output,
	.get_value		= rockchip_gpio_get_value,
	.set_value		= rockchip_gpio_set_value,
	.get_function		= rockchip_gpio_get_function,
};

static const struct udevice_id rockchip_gpio_ids[] = {
	{ .compatible = "rockchip,gpio-bank" },
	{ }
};

U_BOOT_DRIVER(rockchip_gpio_bank) = {
	.name	= "rockchip_gpio_bank",
	.id	= UCLASS_GPIO,
	.of_match = rockchip_gpio_ids,
	.ops	= &gpio_rockchip_ops,
	.priv_auto	= sizeof(struct rockchip_gpio_priv),
	.probe	= rockchip_gpio_probe,
};
