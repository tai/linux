/*
 * mach-celestialsemi/include/mach/nand.h
 *
 *   Xiaodong Fan <xiaodong.fan@celestialsemi.com>
 *
 * --------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __ARCH_ARM_CNC_NAND_H
#define __ARCH_ARM_CNC_NAND_H

#include <linux/mtd/nand.h>

#define CE0_CFG_0_OFFSET 0
#define CE0_CFG_1_OFFSET 0x04
#define CE0_CFG_2_OFFSET 0x08

#define CE1_CFG_0_OFFSET 0x0C
#define CE1_CFG_1_OFFSET 0x10
#define CE1_CFG_2_OFFSET 0x14


#define CE2_CFG_0_OFFSET 0x18
#define CE2_CFG_1_OFFSET 0x1C
#define CE2_CFG_2_OFFSET 0x20


#define CE3_CFG_0_OFFSET 0x24
#define CE3_CFG_1_OFFSET 0x28
#define CE3_CFG_2_OFFSET 0x2c

#define STATUS_OFFSET 0x30
#define MODE_OFFSET 0x38

#define ECC_CODE_OFFSET 0x40
#define ECC_CNT_OFFSET 0x44
#define ECC_A_OFFSET  0x48

/* NOTE:  boards don't need to use these address bits
 * for ALE/CLE unless they support booting from NAND.
 * They're used unless platform data overrides them.
 */
#ifdef CONFIG_MACH_CELESTIAL_CNC1800L
  #define	MASK_ALE		0x8
  #define	MASK_CLE		0x4
#else
  #define   MASK_ALE        0x4
  #define   MASK_CLE        0x2
#endif


#define MASK_CE_FORCE_LOW 0x200

struct cnc_nand_pdata {		/* platform_data */
	uint32_t		mask_ale;
	uint32_t		mask_cle;

	/* for packages using two chipselects */
	uint32_t		maxchip;

	/* board's default static partition info */
	struct mtd_partition	*parts;
	unsigned		nr_parts;

	/* none  == NAND_ECC_NONE (strongly *not* advised!!)
	 * soft  == NAND_ECC_SOFT
	 * else  == NAND_ECC_HW, according to ecc_bits
	 *
	 * cnc1800h chip support 1-bit hardware ECC.
	 */
	nand_ecc_modes_t	ecc_mode;
	u8			ecc_bits;

	/* e.g. NAND_BUSWIDTH_16 or NAND_USE_FLASH_BBT */
	unsigned		options;
};

#endif	/* __ARCH_ARM_CNC_NAND_H */
