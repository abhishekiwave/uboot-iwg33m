// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright 2018-2019 NXP
 *
 * Brief   CAAM Controller Hardware Abstration Layer.
 *         Implementation of primitives to access HW.
 */
#include <caam_hal_ctrl.h>
#include <caam_io.h>
#include <platform_config.h>
#include <registers/ctrl_regs.h>
#include <registers/version_regs.h>
#include <registers/jr_regs.h>

uint8_t caam_hal_ctrl_jrnum(vaddr_t baseaddr)
{
	uint32_t val = 0;

	val = io_caam_read32(baseaddr + CHANUM_MS);

	return GET_CHANUM_MS_JRNUM(val);
}

uint8_t caam_hal_ctrl_hash_limit(vaddr_t baseaddr)
{
	uint32_t val = 0;

	/* Read the number of instance */
	val = io_caam_read32(baseaddr + CHANUM_LS);

	if (GET_CHANUM_LS_MDNUM(val)) {
		/* Hashing is supported */
		val = io_caam_read32(baseaddr + CHAVID_LS);
		val &= BM_CHAVID_LS_MDVID;
		if (val == CHAVID_LS_MDVID_LP256)
			return TEE_MAIN_ALGO_SHA256;

		return TEE_MAIN_ALGO_SHA512;
	}

	return UINT8_MAX;
}

bool caam_hal_ctrl_splitkey(vaddr_t baseaddr)
{
	uint32_t val = 0;

	val = io_caam_read32(baseaddr + CAAMVID_MS);

	if (GET_CAAMVID_MS_MAJ_REV(val) < 3)
		return false;

	return true;
}

uint8_t caam_hal_ctrl_pknum(vaddr_t baseaddr)
{
	uint32_t val = 0;

	val = io_caam_read32(baseaddr + CHANUM_LS);

	return GET_CHANUM_LS_PKNUM(val);
}

uint8_t caam_hal_ctrl_era(vaddr_t baseaddr)
{
	uint32_t val = 0;

	/* Read the number of instance */
	val = io_caam_read32(baseaddr + CCBVID);

	return GET_CCBVID_CAAM_ERA(val);
}

uint8_t caam_hal_ctrl_get_mpcurve(vaddr_t ctrl_addr)
{
	uint32_t val_scfgr = 0;

	/*
	 * On i.MX8MQ B0, the MP is not usable, hence
	 * return UINT8_MAX
	 */
	if (soc_is_imx8mq_b0_layer())
		return UINT8_MAX;

	/*
	 * Verify if the device is closed or not
	 * If device is closed, check get the MPCurve
	 */
	if (imx_is_device_closed()) {
		/* Get the SCFGR content */
		val_scfgr = io_caam_read32(ctrl_addr + SCFGR);

		/* Get the MPCurve field value - 4 bits */
		val_scfgr = (val_scfgr & BM_SCFGR_MPCURVE) >> BS_SCFGR_MPCURVE;

		/*
		 * If the device is closed and the MPCurve field is 0
		 * return UINT8_MAX indicating that there is a problem and the
		 * MP can not be supported.
		 */
		if (!val_scfgr)
			return UINT8_MAX;
	}

	return val_scfgr;
}

TEE_Result caam_hal_ctrl_read_mpmr(vaddr_t ctrl_addr, struct caambuf *mpmr)
{
	int i = 0;
	uint32_t val = 0;

	if (mpmr->length < MPMR_NB_REG) {
		mpmr->length = MPMR_NB_REG;
		return TEE_ERROR_SHORT_BUFFER;
	}

	/* MPMR endianness is reverted between write and read */
	for (; i < MPMR_NB_REG; i += 4) {
		val = io_caam_read32(ctrl_addr + MPMR + i);
		mpmr->data[i] = (uint8_t)((val >> 24) & UINT8_MAX);
		mpmr->data[i + 1] = (uint8_t)((val >> 16) & UINT8_MAX);
		mpmr->data[i + 2] = (uint8_t)((val >> 8) & UINT8_MAX);
		mpmr->data[i + 3] = (uint8_t)(val & UINT8_MAX);
	}

	mpmr->length = MPMR_NB_REG;
	return TEE_SUCCESS;
}

void caam_hal_ctrl_fill_mpmr(vaddr_t ctrl_addr, struct caambuf *msg_mpmr)
{
	size_t i = 0;
	vaddr_t reg = ctrl_addr + MPMR;
	bool is_filled = false;
	uint32_t val = 0;
	size_t min_size = 0;
	size_t remain_size = 0;

	/* check if the MPMR is filled */
	if (io_caam_read32(ctrl_addr + SCFGR) & BM_SCFGR_MPMRL)
		is_filled = true;

	DMSG("is_filled = %s", is_filled ? "true" : "false");

	if (!is_filled) {
		/*
		 * Fill the MPMR with the most significant input value and
		 * complete with 0's if value too short.
		 */
		min_size = MIN(msg_mpmr->length, (size_t)MPMR_NB_REG);
		remain_size = min_size % 4;

		for (i = 0; i < min_size - remain_size; i += 4, reg += 4) {
			val = msg_mpmr->data[i] | msg_mpmr->data[i + 1] << 8 |
			      msg_mpmr->data[i + 2] << 16 |
			      msg_mpmr->data[i + 3] << 24;
			io_caam_write32(reg, val);
		}

		/* Last input bytes value */
		if (remain_size) {
			val = 0;

			/*
			 * Fill the MPMR with the 8 bits values
			 * until the end of the message length
			 */
			for (i = 0; i < remain_size; i++)
				val |= msg_mpmr->data[i] << (i * 8);
			io_caam_write32(reg, val);
			reg += 4;
		}

		/* Complete with 0's */
		remain_size = (MPMR_NB_REG - ROUNDUP(msg_mpmr->length, 4)) / 4;
		for (i = 0; i < remain_size; i++, reg += 4)
			io_caam_write32(reg, 0x0);

		/*
		 * Locks the MPMR for writing and remains locked until
		 * the next power-on session.
		 */
		io_caam_write32(ctrl_addr + SCFGR,
				io_caam_read32(ctrl_addr + SCFGR) |
				 BM_SCFGR_MPMRL);

		DMSG("val_scfgr = 0x%" PRIx32,
		     io_caam_read32(ctrl_addr + SCFGR));
	}
}

vaddr_t caam_hal_ctrl_get_smvaddr(vaddr_t ctrl_addr, paddr_t jr_offset)
{
	/*
	 * The Secure Memory Virtual Base Address contains only the upper
	 * bits of the base address of Secure Memory in this Job Ring's virtual
	 * address space. Since the base address of Secure Memory must be on a
	 * 64 kbyte boundary, the least significant 16 bits are omitted.
	 */
	return io_caam_read32(ctrl_addr + JRX_SMVBAR(JRX_IDX(jr_offset))) << 16;
}
