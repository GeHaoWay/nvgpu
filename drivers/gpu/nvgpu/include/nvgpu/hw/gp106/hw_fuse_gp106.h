/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * Function/Macro naming determines intended use:
 *
 *     <x>_r(void) : Returns the offset for register <x>.
 *
 *     <x>_o(void) : Returns the offset for element <x>.
 *
 *     <x>_w(void) : Returns the word offset for word (4 byte) element <x>.
 *
 *     <x>_<y>_s(void) : Returns size of field <y> of register <x> in bits.
 *
 *     <x>_<y>_f(u32 v) : Returns a value based on 'v' which has been shifted
 *         and masked to place it at field <y> of register <x>.  This value
 *         can be |'d with others to produce a full register value for
 *         register <x>.
 *
 *     <x>_<y>_m(void) : Returns a mask for field <y> of register <x>.  This
 *         value can be ~'d and then &'d to clear the value of field <y> for
 *         register <x>.
 *
 *     <x>_<y>_<z>_f(void) : Returns the constant value <z> after being shifted
 *         to place it at field <y> of register <x>.  This value can be |'d
 *         with others to produce a full register value for <x>.
 *
 *     <x>_<y>_v(u32 r) : Returns the value of field <y> from a full register
 *         <x> value 'r' after being shifted to place its LSB at bit 0.
 *         This value is suitable for direct comparison with other unshifted
 *         values appropriate for use in field <y> of register <x>.
 *
 *     <x>_<y>_<z>_v(void) : Returns the constant value for <z> defined for
 *         field <y> of register <x>.  This value is suitable for direct
 *         comparison with unshifted values appropriate for use in field <y>
 *         of register <x>.
 */
#ifndef NVGPU_HW_FUSE_GP106_H
#define NVGPU_HW_FUSE_GP106_H

#include <nvgpu/types.h>
#include <nvgpu/safe_ops.h>

#define fuse_status_opt_gpc_r()                                    (0x00021c1cU)
#define fuse_status_opt_tpc_gpc_r(i)\
		(nvgpu_safe_add_u32(0x00021c38U, nvgpu_safe_mult_u32((i), 4U)))
#define fuse_ctrl_opt_tpc_gpc_r(i)\
		(nvgpu_safe_add_u32(0x00021838U, nvgpu_safe_mult_u32((i), 4U)))
#define fuse_ctrl_opt_ram_svop_pdp_r()                             (0x00021944U)
#define fuse_ctrl_opt_ram_svop_pdp_data_f(v)             ((U32(v) & 0x3U) << 0U)
#define fuse_ctrl_opt_ram_svop_pdp_data_m()                    (U32(0x3U) << 0U)
#define fuse_ctrl_opt_ram_svop_pdp_data_v(r)                (((r) >> 0U) & 0x3U)
#define fuse_ctrl_opt_ram_svop_pdp_override_r()                    (0x00021948U)
#define fuse_ctrl_opt_ram_svop_pdp_override_data_f(v)    ((U32(v) & 0x1U) << 0U)
#define fuse_ctrl_opt_ram_svop_pdp_override_data_m()           (U32(0x1U) << 0U)
#define fuse_ctrl_opt_ram_svop_pdp_override_data_v(r)       (((r) >> 0U) & 0x1U)
#define fuse_ctrl_opt_ram_svop_pdp_override_data_yes_f()                  (0x1U)
#define fuse_ctrl_opt_ram_svop_pdp_override_data_no_f()                   (0x0U)
#define fuse_status_opt_fbio_r()                                   (0x00021c14U)
#define fuse_status_opt_fbio_data_f(v)                ((U32(v) & 0xffffU) << 0U)
#define fuse_status_opt_fbio_data_m()                       (U32(0xffffU) << 0U)
#define fuse_status_opt_fbio_data_v(r)                   (((r) >> 0U) & 0xffffU)
#define fuse_status_opt_rop_l2_fbp_r(i)\
		(nvgpu_safe_add_u32(0x00021d70U, nvgpu_safe_mult_u32((i), 4U)))
#define fuse_status_opt_fbp_r()                                    (0x00021d38U)
#define fuse_status_opt_fbp_idx_v(r, i)\
		(((r) >> (0U + i*1U)) & 0x1U)
#define fuse_vin_cal_fuse_rev_r()                                  (0x0002164cU)
#define fuse_vin_cal_fuse_rev_data_v(r)                     (((r) >> 0U) & 0x3U)
#define fuse_vin_cal_gpc0_r()                                      (0x00021650U)
#define fuse_vin_cal_gpc0_icpt_int_data_s()                                (12U)
#define fuse_vin_cal_gpc0_icpt_int_data_v(r)             (((r) >> 16U) & 0xfffU)
#define fuse_vin_cal_gpc0_icpt_frac_data_s()                                (2U)
#define fuse_vin_cal_gpc0_icpt_frac_data_v(r)              (((r) >> 14U) & 0x3U)
#define fuse_vin_cal_gpc0_slope_int_data_s()                                (4U)
#define fuse_vin_cal_gpc0_slope_int_data_v(r)              (((r) >> 10U) & 0xfU)
#define fuse_vin_cal_gpc0_slope_frac_data_s()                              (10U)
#define fuse_vin_cal_gpc0_slope_frac_data_v(r)            (((r) >> 0U) & 0x3ffU)
#define fuse_vin_cal_gpc1_delta_r()                                (0x00021654U)
#define fuse_vin_cal_gpc1_delta_icpt_int_data_s()                           (8U)
#define fuse_vin_cal_gpc1_delta_icpt_int_data_v(r)        (((r) >> 14U) & 0xffU)
#define fuse_vin_cal_gpc1_delta_icpt_frac_data_s()                          (2U)
#define fuse_vin_cal_gpc1_delta_icpt_frac_data_v(r)        (((r) >> 12U) & 0x3U)
#define fuse_vin_cal_gpc1_delta_icpt_sign_data_s()                          (1U)
#define fuse_vin_cal_gpc1_delta_icpt_sign_data_v(r)        (((r) >> 22U) & 0x1U)
#define fuse_vin_cal_gpc1_delta_slope_int_data_s()                          (1U)
#define fuse_vin_cal_gpc1_delta_slope_int_data_v(r)        (((r) >> 10U) & 0x1U)
#define fuse_vin_cal_gpc1_delta_slope_frac_data_s()                        (10U)
#define fuse_vin_cal_gpc1_delta_slope_frac_data_v(r)      (((r) >> 0U) & 0x3ffU)
#define fuse_vin_cal_gpc1_delta_slope_sign_data_s()                         (1U)
#define fuse_vin_cal_gpc1_delta_slope_sign_data_v(r)       (((r) >> 11U) & 0x1U)
#define fuse_vin_cal_gpc2_delta_r()                                (0x00021658U)
#define fuse_vin_cal_gpc3_delta_r()                                (0x0002165cU)
#define fuse_vin_cal_gpc4_delta_r()                                (0x00021660U)
#define fuse_vin_cal_gpc5_delta_r()                                (0x00021664U)
#define fuse_vin_cal_shared_delta_r()                              (0x00021668U)
#define fuse_vin_cal_sram_delta_r()                                (0x0002166cU)
#define fuse_vin_cal_sram_delta_icpt_int_data_s()                           (9U)
#define fuse_vin_cal_sram_delta_icpt_int_data_v(r)       (((r) >> 13U) & 0x1ffU)
#define fuse_vin_cal_sram_delta_icpt_frac_data_s()                          (1U)
#define fuse_vin_cal_sram_delta_icpt_frac_data_v(r)        (((r) >> 12U) & 0x1U)
#endif
