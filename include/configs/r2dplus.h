#ifndef __CONFIG_H
#define __CONFIG_H

#define __LITTLE_ENDIAN__	1

/* SCIF */
#define CONFIG_CONS_SCIF1	1

/* SDRAM */
#define CONFIG_SYS_SDRAM_BASE		0x8C000000
#define CONFIG_SYS_SDRAM_SIZE		0x04000000

#define CONFIG_SYS_PBSIZE		256

/* Address of u-boot image in Flash */
#define CONFIG_SYS_MONITOR_BASE	(CONFIG_SYS_FLASH_BASE)
#define CONFIG_SYS_MONITOR_LEN		(256 * 1024)
#define CONFIG_SYS_BOOTMAPSZ		(8 * 1024 * 1024)

/*
 * NOR Flash ( Spantion S29GL256P )
 */
#define CONFIG_SYS_FLASH_BASE		(0xA0000000)
#define CONFIG_SYS_MAX_FLASH_SECT  256
#define CONFIG_SYS_FLASH_BANKS_LIST	{ CONFIG_SYS_FLASH_BASE }

/*
 * SuperH Clock setting
 */
#define	CONFIG_SYS_PLL_SETTLING_TIME	100/* in us */

/*
 * IDE support
 */
#define CONFIG_IDE_RESET	1
#define CONFIG_SYS_PIO_MODE		1
#define CONFIG_SYS_IDE_MAXBUS		1 /* IDE bus */
#define CONFIG_SYS_IDE_MAXDEVICE	1
#define CONFIG_SYS_ATA_BASE_ADDR	0xb4000000
#define CONFIG_SYS_ATA_STRIDE		2 /* 1bit shift */
#define CONFIG_SYS_ATA_DATA_OFFSET	0x1000	/* data reg offset */
#define CONFIG_SYS_ATA_REG_OFFSET	0x1000	/* reg offset */
#define CONFIG_SYS_ATA_ALT_OFFSET	0x800	/* alternate register offset */

/*
 * SuperH PCI Bridge Configration
 */
#define CONFIG_SH7751_PCI

#endif /* __CONFIG_H */
