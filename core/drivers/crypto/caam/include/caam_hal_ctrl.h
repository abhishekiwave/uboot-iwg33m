/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2018-2019 NXP
 *
 * Brief   CAAM Controller Hardware Abstration Layer header.
 */
#ifndef __CAAM_HAL_CTRL_H__
#define __CAAM_HAL_CTRL_H__

#include <caam_common.h>
#include <types_ext.h>

/*
 * Initializes the CAAM HW Controller
 *
 * @baseaddr  Controller base address
 */
void caam_hal_ctrl_init(vaddr_t baseaddr);

/*
 * Returns the number of Job Ring supported
 *
 * @baseaddr  Controller base address
 */
uint8_t caam_hal_ctrl_jrnum(vaddr_t baseaddr);

/*
 * If Hash operation is supported, returns the Maximum Hash Algorithm
 * supported by the HW else UINT8_MAX
 *
 * @baseaddr  Controller base address
 */
uint8_t caam_hal_ctrl_hash_limit(vaddr_t baseaddr);

/*
 * Returns the number of Public Key module supported
 *
 * @baseaddr  Controller base address
 */
uint8_t caam_hal_ctrl_pknum(vaddr_t baseaddr);

/*
 * Returns if the HW support the split key operation.
 * Split key is supported if CAAM Version is >= 3
 *
 * @baseaddr  Controller base address
 */
bool caam_hal_ctrl_splitkey(vaddr_t baseaddr);

/*
 * Returns the CAAM Era
 *
 * @baseaddr  Controller base address
 */
uint8_t caam_hal_ctrl_era(vaddr_t baseaddr);

/*
 * Get the SCFGR content and check the MPCURVE fields.
 * The function returns either:
 *   - UINT8_MAX if the Manafacturing Protection is not supported
 *   - The MP Curve Value is programmed (4 bits value)
 *   - 0 if the MP Curve is not programmed
 *
 * @ctrl_addr  Controller base address
 */
uint8_t caam_hal_ctrl_get_mpcurve(vaddr_t ctrl_addr);

/*
 * Read the MPMR content
 *
 * @ctrl_addr  Controller base address
 * @mpmr       [out] MPMR buffer read
 */
TEE_Result caam_hal_ctrl_read_mpmr(vaddr_t ctrl_addr, struct caambuf *mpmr);

/*
 * Fill the MPMR content then lock the register
 *
 * @ctrl_addr  Controller base address
 * @msg_mpmr   Buffer with the message and length to fill the MPMR content
 */
void caam_hal_ctrl_fill_mpmr(vaddr_t ctrl_addr, struct caambuf *msg_mpmr);

/*
 * Get the Secure Memory Virtual base address setup in the given job ring
 *
 * @ctrl_addr  Controller base address
 * @jr_offset  Job ring offset
 */
vaddr_t caam_hal_ctrl_get_smvaddr(vaddr_t ctrl_addr, paddr_t jr_offset);
#endif /* __CAAM_HAL_CTRL_H__ */
