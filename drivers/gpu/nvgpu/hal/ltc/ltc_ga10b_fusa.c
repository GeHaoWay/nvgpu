/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/types.h>
#include <nvgpu/gk20a.h>
#include <nvgpu/io.h>
#include <nvgpu/channel.h>
#include <nvgpu/regops.h>
#include <nvgpu/errata.h>

#include "hal/gr/gr/gr_gk20a.h"
#include "ltc_ga10b.h"

#include <nvgpu/hw/ga10b/hw_ltc_ga10b.h>

/* Minimum value of cacheline_size */
#define CACHELINE_SIZE_BASE		512U

void ga10b_ltc_init_fs_state(struct gk20a *g)
{
	u32 reg;

	g->ltc->max_ltc_count = g->ops.top.get_num_ltcs(g);
	g->ltc->ltc_count = g->ops.priv_ring.enum_ltc(g);
	nvgpu_log_info(g, "%u ltcs present out of %u total supported ltcs",
		g->ltc->ltc_count, g->ltc->max_ltc_count);

	reg = nvgpu_readl(g, ltc_ltcs_ltss_cbc_param2_r());
	g->ltc->slices_per_ltc =
		ltc_ltcs_ltss_cbc_param2_slices_per_ltc_v(reg);
	g->ltc->cacheline_size = CACHELINE_SIZE_BASE <<
		ltc_ltcs_ltss_cbc_param2_cache_line_size_v(reg);

	nvgpu_log_info(g, "slices_per_ltc %u", g->ltc->slices_per_ltc);
	nvgpu_log_info(g, "cacheline_size %u", g->ltc->cacheline_size);

	/* PLC compression */
	reg = nvgpu_readl(g, ltc_ltcs_ltss_tstg_set_mgmt_1_r());
	if (nvgpu_is_enabled(g, NVGPU_SUPPORT_POST_L2_COMPRESSION)) {
		reg = set_field(reg,
			ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_plc_m(),
			ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_plc_enabled_f());
		reg = set_field(reg,
			ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_rmw_m(),
			ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_rmw_enabled_f());
	} else {
		reg = set_field(reg,
			ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_plc_m(),
			ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_plc_disabled_f());
		reg = set_field(reg,
			ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_rmw_m(),
			ltc_ltcs_ltss_tstg_set_mgmt_1_plc_recompress_rmw_disabled_f());
	}
	nvgpu_writel(g, ltc_ltcs_ltss_tstg_set_mgmt_1_r(), reg);
}

void ga10b_ltc_lts_set_mgmt_setup(struct gk20a *g)
{
	u32 reg;

	if (nvgpu_is_errata_present(g, NVGPU_ERRATA_200601972)) {
		reg = nvgpu_readl(g, ltc_ltcs_ltss_tstg_set_mgmt_3_r());
		reg = set_field(reg,
			ltc_ltcs_ltss_tstg_set_mgmt_3_disallow_clean_ce_imm_m(),
			ltc_ltcs_ltss_tstg_set_mgmt_3_disallow_clean_ce_imm_enabled_f());
		reg = set_field(reg,
			ltc_ltcs_ltss_tstg_set_mgmt_3_disallow_clean_fclr_imm_m(),
			ltc_ltcs_ltss_tstg_set_mgmt_3_disallow_clean_fclr_imm_enabled_f());
		nvgpu_writel(g, ltc_ltcs_ltss_tstg_set_mgmt_3_r(), reg);
	}
}

int ga10b_set_l2_max_ways_evict_last(struct gk20a *g, struct nvgpu_tsg *tsg,
		u32 num_ways)
{
	struct nvgpu_dbg_reg_op ops = {
		.op = REGOP(READ_32),
		.type = REGOP(TYPE_GR_CTX),
		.offset = ltc_ltcs_ltss_tstg_set_mgmt0_r(),
		.and_n_mask_lo = 0xffffffff
	};
	int err = -EINVAL;
	u32 flags = NVGPU_REG_OP_FLAG_MODE_ALL_OR_NONE;
	const u32 num_ops = 1U;

	/*
	 * MAX_WAYS_EVICT_LAST ways should not exceed the number of ways in a
	 * L2 set.
	 */
	if (num_ways > g->ops.get_litter_value(g, GPU_LIT_NUM_LTC_LTS_WAYS)) {
		nvgpu_err(g, "error: num_ways(%d) > max_ways(%d)", num_ways,
				g->ops.get_litter_value(g, GPU_LIT_NUM_LTC_LTS_WAYS));
		return err;
	}

	/*
	 * Readback the current TSTG setting.
	 */
	err = gr_gk20a_exec_ctx_ops(tsg, &ops, num_ops, 0, num_ops, &flags);
	if (err != 0) {
		nvgpu_err(g, "regops_rd failed for LTCS_LTSS_TSTG_MGMT_0");
		return err;
	}
	nvgpu_log_info(g, "current max_ways_l2_evict_last value=0x%x",
		ltc_ltcs_ltss_tstg_set_mgmt0_max_evict_last_v(ops.value_lo));

	ops.value_lo = set_field(ops.value_lo,
			ltc_ltcs_ltss_tstg_set_mgmt0_max_evict_last_m(),
			ltc_ltcs_ltss_tstg_set_mgmt0_max_evict_last_f(num_ways));
	nvgpu_log_info(g, "writing 0x%x to change l2 max_ways_evict_last to 0x%x",
			ops.value_lo, num_ways);

	/*
	 * Write out the new value for L2_MAX_EVICT_LAST.
	 */
	ops.op = REGOP(WRITE_32);
	err = gr_gk20a_exec_ctx_ops(tsg, &ops, num_ops, num_ops, 0, &flags);
	if (err != 0) {
		nvgpu_err(g, "regops_wr failed for LTCS_LTSS_TSTG_MGMT_0");
		return err;
	}

	/*
	 * Readback and verify L2_MAX_EVICT_LAST.
	 */
	ops.op = REGOP(READ_32);
	ops.value_lo = 0U;
	err = gr_gk20a_exec_ctx_ops(tsg, &ops, num_ops, 0, num_ops, &flags);
	if (err != 0) {
		nvgpu_err(g, "regops_rd failed for LTCS_LTSS_TSTG_MGMT_0");
	}
	if (ltc_ltcs_ltss_tstg_set_mgmt0_max_evict_last_v(ops.value_lo) !=
			num_ways) {
		nvgpu_err(g, "mismatch, expected(%d) != readback(%d)", num_ways,
			ltc_ltcs_ltss_tstg_set_mgmt0_max_evict_last_v(ops.value_lo));
		return -EINVAL;
	}

	return err;
}

int ga10b_get_l2_max_ways_evict_last(struct gk20a *g, struct nvgpu_tsg *tsg,
		u32 *num_ways)
{
	struct nvgpu_dbg_reg_op ops = {
		.op = REGOP(READ_32),
		.type = REGOP(TYPE_GR_CTX),
		.offset = ltc_ltcs_ltss_tstg_set_mgmt0_r(),
		.and_n_mask_lo = 0xffffffff
	};
	int err;
	u32 flags = NVGPU_REG_OP_FLAG_MODE_ALL_OR_NONE;
	u32 num_ops = 1U;

	if (num_ways == NULL) {
		return -EINVAL;
	}

	/*
	 * Readback the current TSTG setting.
	 */
	err = gr_gk20a_exec_ctx_ops(tsg, &ops, num_ops, 0, num_ops, &flags);
	if (err != 0) {
		nvgpu_err(g, "regops_rd failed for LTCS_LTSS_TSTG_MGMT_0");
		return err;
	}
	*num_ways = ltc_ltcs_ltss_tstg_set_mgmt0_max_evict_last_v(ops.value_lo);
	nvgpu_log_info(g, "current max_ways_l2_evict_last value=0x%x", *num_ways);

	return err;
}
