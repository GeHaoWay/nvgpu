/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
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

#include <unit/unit.h>
#include <unit/io.h>
#include <nvgpu/posix/io.h>

#include <nvgpu/gk20a.h>
#include <nvgpu/nvgpu_init.h>
#include <nvgpu/hal_init.h>
#include <nvgpu/enabled.h>
#include <nvgpu/hw/gm20b/hw_mc_gm20b.h>
#include <nvgpu/posix/posix-fault-injection.h>
#include <os/posix/os_posix.h>
#include <nvgpu/dma.h>
#include <nvgpu/gops_mc.h>

/* for get_litter testing */
#include "hal/init/hal_gv11b_litter.h"
#include <nvgpu/hw/gv11b/hw_proj_gv11b.h>
#include <nvgpu/class.h>

#include "nvgpu-init.h"

/* value for GV11B */
#define MC_BOOT_0_GV11B (NVGPU_GPUID_GV11B << 20)
/* to set the security fuses */
#define GP10B_FUSE_REG_BASE		0x00021000U
#define GP10B_FUSE_OPT_PRIV_SEC_EN	(GP10B_FUSE_REG_BASE+0x434U)

#define assert(cond) unit_assert(cond, goto fail)
/*
 * Mock I/O
 */

/*
 * Write callback. Forward the write access to the mock IO framework.
 */
static void writel_access_reg_fn(struct gk20a *g,
				 struct nvgpu_reg_access *access)
{
	nvgpu_posix_io_writel_reg_space(g, access->addr, access->value);
}

/*
 * Read callback. Get the register value from the mock IO framework.
 */
static void readl_access_reg_fn(struct gk20a *g,
				struct nvgpu_reg_access *access)
{
	access->value = nvgpu_posix_io_readl_reg_space(g, access->addr);
}

static struct nvgpu_posix_io_callbacks test_reg_callbacks = {
	/* Write APIs all can use the same accessor. */
	.writel          = writel_access_reg_fn,
	.writel_check    = writel_access_reg_fn,
	.bar1_writel     = writel_access_reg_fn,
	.usermode_writel = writel_access_reg_fn,

	/* Likewise for the read APIs. */
	.__readl         = readl_access_reg_fn,
	.readl           = readl_access_reg_fn,
	.bar1_readl      = readl_access_reg_fn,
};

/*
 * Replacement functions that can be assigned to function pointers
 */
static void no_return(struct gk20a *g)
{
	/* noop */
}

static int return_success(struct gk20a *g)
{
	return 0;
}

static int return_fail(struct gk20a *g)
{
	return -1;
}

/*
 * Falcon is tricky because it is called multiple times with different IDs.
 * So, we use this variable to determine which one will return an error.
 */
static u32 falcon_fail_on_id = U32_MAX;
static int falcon_sw_init(struct gk20a *g, u32 falcon_id)
{
	if (falcon_id == falcon_fail_on_id) {
		return -1;
	}

	return 0;
}

/* generic for passing in a u32 */
static int return_success_u32_param(struct gk20a *g, u32 dummy)
{
	return 0;
}

/* generic for passing in a u32 and returning int */
static int return_failure_u32_param(struct gk20a *g, u32 dummy)
{
	return -1;
}

/* generic for passing in a u32 and returning u32 */
static u32 return_u32_u32_param(struct gk20a *g, u32 dummy)
{
	return 0;
}

/* generic for passing in a u32 but nothin to return */
static void no_return_u32_param(struct gk20a *g, u32 dummy)
{
	/* no op  */
}

int test_setup_env(struct unit_module *m,
			  struct gk20a *g, void *args)
{
	/* Create mc register space */
	nvgpu_posix_io_init_reg_space(g);
	if (nvgpu_posix_io_add_reg_space(g, mc_boot_0_r(), 0xfff) != 0) {
		unit_err(m, "%s: failed to create register space\n",
			 __func__);
		return UNIT_FAIL;
	}
	/* Create fuse register space */
	if (nvgpu_posix_io_add_reg_space(g, GP10B_FUSE_REG_BASE, 0xfff) != 0) {
		unit_err(m, "%s: failed to create register space\n",
			 __func__);
		return UNIT_FAIL;
	}
	(void)nvgpu_posix_register_io(g, &test_reg_callbacks);

	return UNIT_SUCCESS;
}

int test_free_env(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	/* Free mc register space */
	nvgpu_posix_io_delete_reg_space(g, mc_boot_0_r());
	nvgpu_posix_io_delete_reg_space(g, GP10B_FUSE_REG_BASE);

	/* Clean up quiesce thread */
	nvgpu_sw_quiesce_remove_support(g);

	return UNIT_SUCCESS;
}

int test_get_litter_value(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	assert(gv11b_get_litter_value(g, GPU_LIT_NUM_GPCS) ==
					proj_scal_litter_num_gpcs_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_NUM_PES_PER_GPC) ==
					proj_scal_litter_num_pes_per_gpc_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_NUM_ZCULL_BANKS) ==
					proj_scal_litter_num_zcull_banks_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_NUM_TPC_PER_GPC) ==
					proj_scal_litter_num_tpc_per_gpc_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_NUM_SM_PER_TPC) ==
					proj_scal_litter_num_sm_per_tpc_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_NUM_FBPS) ==
					proj_scal_litter_num_fbps_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_GPC_BASE) ==
					proj_gpc_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_GPC_STRIDE) ==
					proj_gpc_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_GPC_SHARED_BASE) ==
					proj_gpc_shared_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_TPC_IN_GPC_BASE) ==
					proj_tpc_in_gpc_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_TPC_IN_GPC_STRIDE) ==
					proj_tpc_in_gpc_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_TPC_IN_GPC_SHARED_BASE) ==
					proj_tpc_in_gpc_shared_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_PPC_IN_GPC_BASE) ==
					proj_ppc_in_gpc_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_PPC_IN_GPC_SHARED_BASE) ==
					proj_ppc_in_gpc_shared_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_PPC_IN_GPC_STRIDE) ==
					proj_ppc_in_gpc_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_ROP_BASE) ==
					proj_rop_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_ROP_STRIDE) ==
					proj_rop_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_ROP_SHARED_BASE) ==
					proj_rop_shared_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_HOST_NUM_ENGINES) ==
					proj_host_num_engines_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_HOST_NUM_PBDMA) ==
					proj_host_num_pbdma_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_LTC_STRIDE) ==
					proj_ltc_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_LTS_STRIDE) ==
					proj_lts_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_SM_PRI_STRIDE) ==
					proj_sm_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_SMPC_PRI_BASE) ==
					proj_smpc_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_SMPC_PRI_SHARED_BASE) ==
					proj_smpc_shared_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_SMPC_PRI_UNIQUE_BASE) ==
					proj_smpc_unique_base_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_SMPC_PRI_STRIDE) ==
					proj_smpc_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_NUM_FBPAS) ==
					proj_scal_litter_num_fbpas_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_FBPA_STRIDE) ==
					0);
	assert(gv11b_get_litter_value(g, GPU_LIT_FBPA_BASE) ==
					0);
	assert(gv11b_get_litter_value(g, GPU_LIT_FBPA_SHARED_BASE) ==
					0);
	assert(gv11b_get_litter_value(g, GPU_LIT_COMPUTE_CLASS) ==
					VOLTA_COMPUTE_A);
	assert(gv11b_get_litter_value(g, GPU_LIT_GPFIFO_CLASS) ==
					VOLTA_CHANNEL_GPFIFO_A);
	assert(gv11b_get_litter_value(g, GPU_LIT_I2M_CLASS) ==
					KEPLER_INLINE_TO_MEMORY_B);
	assert(gv11b_get_litter_value(g, GPU_LIT_DMA_COPY_CLASS) ==
					VOLTA_DMA_COPY_A);
	assert(gv11b_get_litter_value(g, GPU_LIT_GPC_PRIV_STRIDE) ==
					proj_gpc_priv_stride_v());
	assert(gv11b_get_litter_value(g, GPU_LIT_PERFMON_PMMGPCTPCA_DOMAIN_START) ==
					2);
	assert(gv11b_get_litter_value(g, GPU_LIT_PERFMON_PMMGPCTPCB_DOMAIN_START) ==
					6);
	assert(gv11b_get_litter_value(g, GPU_LIT_PERFMON_PMMGPCTPC_DOMAIN_COUNT) ==
					4);
	assert(gv11b_get_litter_value(g, GPU_LIT_PERFMON_PMMFBP_LTC_DOMAIN_START) ==
					1);
	assert(gv11b_get_litter_value(g, GPU_LIT_PERFMON_PMMFBP_LTC_DOMAIN_COUNT) ==
					2);
	assert(gv11b_get_litter_value(g, GPU_LIT_PERFMON_PMMFBP_ROP_DOMAIN_START) ==
					3);
	assert(gv11b_get_litter_value(g, GPU_LIT_PERFMON_PMMFBP_ROP_DOMAIN_COUNT) ==
					2);

	if (!EXPECT_BUG(gv11b_get_litter_value(g, U32_MAX))) {
		unit_err(m, "%s: failed to detect INVALID value\n",
			 __func__);
		goto fail;
	}

	return UNIT_SUCCESS;

fail:
	return UNIT_FAIL;
}

int test_can_busy(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;

	nvgpu_set_enabled(g, NVGPU_KERNEL_IS_DYING, false);
	nvgpu_set_enabled(g, NVGPU_DRIVER_IS_DYING, false);
	if (nvgpu_can_busy(g) != 1) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_can_busy() returned 0\n");
	}

	nvgpu_set_enabled(g, NVGPU_KERNEL_IS_DYING, true);
	nvgpu_set_enabled(g, NVGPU_DRIVER_IS_DYING, false);
	if (nvgpu_can_busy(g) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_can_busy() returned 1\n");
	}

	nvgpu_set_enabled(g, NVGPU_KERNEL_IS_DYING, false);
	nvgpu_set_enabled(g, NVGPU_DRIVER_IS_DYING, true);
	if (nvgpu_can_busy(g) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_can_busy() returned 1\n");
	}

	nvgpu_set_enabled(g, NVGPU_KERNEL_IS_DYING, true);
	nvgpu_set_enabled(g, NVGPU_DRIVER_IS_DYING, true);
	if (nvgpu_can_busy(g) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_can_busy() returned 1\n");
	}

	return ret;
}

int test_get_put(struct unit_module *m,
			struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;

	nvgpu_ref_init(&g->refcount);

	if (g != nvgpu_get(g)) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() returned NULL\n");
	}
	if (nvgpu_atomic_read(&g->refcount.refcount) != 2) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() did not increment refcount\n");
	}

	nvgpu_put(g);
	if (nvgpu_atomic_read(&g->refcount.refcount) != 1) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_put() did not decrement refcount\n");
	}

	/* one more to get to 0 to teardown */
	nvgpu_put(g);
	if (nvgpu_atomic_read(&g->refcount.refcount) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_put() did not decrement refcount\n");
	}

	/* This is expected to fail */
	if (nvgpu_get(g) != NULL) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() did not return NULL\n");
	}
	if (nvgpu_atomic_read(&g->refcount.refcount) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() did not increment refcount\n");
	}

	/* start over */
	nvgpu_ref_init(&g->refcount);

	/* to cover the cases where these are set */
	g->remove_support = no_return;
	g->gfree = no_return;
	g->ops.ecc.ecc_remove_support = no_return;
	g->ops.ltc.ltc_remove_support = no_return;

	if (g != nvgpu_get(g)) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() returned NULL\n");
	}
	if (nvgpu_atomic_read(&g->refcount.refcount) != 2) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_get() did not increment refcount\n");
	}

	nvgpu_put(g);
	if (nvgpu_atomic_read(&g->refcount.refcount) != 1) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_put() did not decrement refcount\n");
	}

	/* one more to get to 0 to teardown */
	nvgpu_put(g);
	if (nvgpu_atomic_read(&g->refcount.refcount) != 0) {
		ret = UNIT_FAIL;
		unit_err(m, "nvgpu_put() did not decrement refcount\n");
	}

	return ret;
}

int test_check_gpu_state(struct unit_module *m,
				struct gk20a *g, void *args)
{
	/* Valid state */
	nvgpu_posix_io_writel_reg_space(g, mc_boot_0_r(), MC_BOOT_0_GV11B);
	nvgpu_check_gpu_state(g);

	/*
	 * Test INVALID state. This should cause a kernel_restart() which
	 * is a BUG() in posix, so verify we hit the BUG().
	 */
	nvgpu_posix_io_writel_reg_space(g, mc_boot_0_r(), U32_MAX);
	if (!EXPECT_BUG(nvgpu_check_gpu_state(g))) {
		unit_err(m, "%s: failed to detect INVALID state\n",
			 __func__);
		return UNIT_FAIL;
	}

	return UNIT_SUCCESS;
}

int test_hal_init(struct unit_module *m,
			 struct gk20a *g, void *args)
{
	const u32 invalid_mc_boot_0[] = {
					  GK20A_GPUID_GK20A << 20,
					  GK20A_GPUID_GM20B << 20,
					  GK20A_GPUID_GM20B_B << 20,
					  NVGPU_GPUID_GP10B << 20,
					  NVGPU_GPUID_GV100 << 20,
					  NVGPU_GPUID_TU104 << 20,
					  U32_MAX,
					};
	u32 i;
	struct nvgpu_os_posix *p = nvgpu_os_posix_from_gk20a(g);

	nvgpu_posix_io_writel_reg_space(g, mc_boot_0_r(), MC_BOOT_0_GV11B);
	nvgpu_posix_io_writel_reg_space(g, GP10B_FUSE_OPT_PRIV_SEC_EN, 0x0);
	if (nvgpu_detect_chip(g) != 0) {
		unit_err(m, "%s: failed to init HAL\n", __func__);
		return UNIT_FAIL;
	}

	/* Branch test for check if already inited the hal */
	if (nvgpu_detect_chip(g) != 0) {
		unit_err(m, "%s: failed to init HAL\n", __func__);
		return UNIT_FAIL;
	}

	/* Branch test for check GPU is version a01 */
	p->is_soc_t194_a01 = true;
	g->params.gpu_arch = 0;
	if (nvgpu_detect_chip(g) != 0) {
		unit_err(m, "%s: failed to init HAL\n", __func__);
		return UNIT_FAIL;
	}
	p->is_soc_t194_a01 = false;

	/* Negative testing for secure fuse */
	g->params.gpu_arch = 0;
	nvgpu_posix_io_writel_reg_space(g, GP10B_FUSE_OPT_PRIV_SEC_EN, 0x1);
	if (nvgpu_detect_chip(g) == 0) {
		unit_err(m, "%s: HAL init failed to detect incorrect security\n",
			 __func__);
		return UNIT_FAIL;
	}

	/* Negative testing for invalid GPU version */
	nvgpu_posix_io_writel_reg_space(g, GP10B_FUSE_OPT_PRIV_SEC_EN, 0x0);
	for (i = 0; i < ARRAY_SIZE(invalid_mc_boot_0); i++) {
		nvgpu_posix_io_writel_reg_space(g, mc_boot_0_r(),
						invalid_mc_boot_0[i]);
		g->params.gpu_arch = 0;
		if (nvgpu_detect_chip(g) == 0) {
			unit_err(m, "%s: HAL init failed to detect invalid GPU %08x\n",
				__func__, invalid_mc_boot_0[i]);
			return UNIT_FAIL;
		}
	}

	return UNIT_SUCCESS;
}

/*
 * For the basic init functions that just take a g pointer, we store them in
 * this array so we can just loop over them later
 */
#define MAX_SIMPLE_INIT_FUNC_PTRS 50
typedef int (*simple_init_func_t)(struct gk20a *g);
static simple_init_func_t *simple_init_func_ptrs[MAX_SIMPLE_INIT_FUNC_PTRS];
static unsigned int simple_init_func_ptrs_count;

/* Store into the simple_init_func_ptrs array and initialize to success */
static void setup_simple_init_func_success(simple_init_func_t *f,
					   unsigned int index)
{
	BUG_ON(index >= MAX_SIMPLE_INIT_FUNC_PTRS);
	simple_init_func_ptrs[index] = f;
	*f = return_success;
}

/*
 * Initialize init poweron function pointers in g to return success, but do
 * nothing else.
 */
static void set_poweron_funcs_success(struct gk20a *g)
{
	unsigned int i = 0;

	/* these are the simple case of just taking a g param */
	setup_simple_init_func_success(&g->ops.ecc.ecc_init_support, i++);
	setup_simple_init_func_success(&g->ops.mm.pd_cache_init, i++);
	setup_simple_init_func_success(&g->ops.clk.init_clk_support, i++);
	setup_simple_init_func_success(&g->ops.nvlink.init, i++);
	setup_simple_init_func_success(&g->ops.fb.init_fbpa, i++);
	setup_simple_init_func_success(&g->ops.fb.mem_unlock, i++);
	setup_simple_init_func_success(&g->ops.fifo.reset_enable_hw, i++);
	setup_simple_init_func_success(&g->ops.ltc.init_ltc_support, i++);
	setup_simple_init_func_success(&g->ops.mm.init_mm_support, i++);
	setup_simple_init_func_success(&g->ops.fifo.fifo_init_support, i++);
	setup_simple_init_func_success(&g->ops.therm.elcg_init_idle_filters, i++);
	setup_simple_init_func_success(&g->ops.gr.gr_prepare_sw, i++);
	setup_simple_init_func_success(&g->ops.gr.gr_enable_hw, i++);
	setup_simple_init_func_success(&g->ops.fbp.fbp_init_support, i++);
	setup_simple_init_func_success(&g->ops.gr.gr_init_support, i++);
	setup_simple_init_func_success(&g->ops.ecc.ecc_finalize_support, i++);
	setup_simple_init_func_success(&g->ops.therm.init_therm_support, i++);
	setup_simple_init_func_success(&g->ops.ce.ce_init_support, i++);
	setup_simple_init_func_success(&g->ops.bus.init_hw, i++);
	setup_simple_init_func_success(&g->ops.priv_ring.enable_priv_ring, i++);
	setup_simple_init_func_success(&g->ops.channel.resume_all_serviceable_ch, i++);
	setup_simple_init_func_success(&g->ops.pmu.pmu_early_init, i++);
	setup_simple_init_func_success(&g->ops.acr.acr_init, i++);
	setup_simple_init_func_success(&g->ops.acr.acr_construct_execute, i++);
	simple_init_func_ptrs_count = i;

	/* these are the exceptions */
	g->ops.falcon.falcon_sw_init = falcon_sw_init;
	falcon_fail_on_id = U32_MAX; /* don't fail */
	g->ops.fuse.fuse_status_opt_tpc_gpc = return_u32_u32_param;
	g->ops.tpc.tpc_powergate = return_success_u32_param;
	g->ops.falcon.falcon_sw_free = no_return_u32_param;

	/* used in support functions */
	g->ops.gr.init.detect_sm_arch = no_return;
	g->ops.gr.ecc.detect = no_return;
}

int test_poweron(struct unit_module *m, struct gk20a *g, void *args)
{
	int ret = UNIT_SUCCESS;
	int err;
	unsigned int i;

	nvgpu_set_enabled(g, NVGPU_SEC_PRIVSECURITY, true);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_NVLINK, true);

	/* test where everything returns success */
	set_poweron_funcs_success(g);
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
				 "nvgpu_finalize_poweron returned failure\n");
	}

	/* loop over the simple cases */
	for (i = 0; i < simple_init_func_ptrs_count; i++) {
		*simple_init_func_ptrs[i] = return_fail;
		nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
		err = nvgpu_finalize_poweron(g);
		if (err == 0) {
			unit_return_fail(m,
				"nvgpu_finalize_poweron errantly returned success i=%d\n",
				i);
		}
		*simple_init_func_ptrs[i] = return_success;
	}

	/* handle the exceptions */

	falcon_fail_on_id = FALCON_ID_PMU;
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}

	falcon_fail_on_id = FALCON_ID_FECS;
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	falcon_fail_on_id = U32_MAX; /* stop failing */

	g->ops.tpc.tpc_powergate = return_failure_u32_param;
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	g->ops.tpc.tpc_powergate = return_success_u32_param;

	/* test the case of already being powered on */
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_ON);
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}

	return ret;
}

int test_poweron_branches(struct unit_module *m, struct gk20a *g, void *args)
{
	int err;
	struct nvgpu_posix_fault_inj *kmem_fi =
			nvgpu_kmem_get_fault_injection();

	nvgpu_set_enabled(g, NVGPU_SEC_PRIVSECURITY, false);
	nvgpu_set_enabled(g, NVGPU_SUPPORT_NVLINK, false);

	set_poweron_funcs_success(g);

	/* hit all the NULL pointer checks */
	g->ops.clk.init_clk_support = NULL;
	g->ops.fb.init_fbpa = NULL;
	g->ops.fb.mem_unlock = NULL;
	g->ops.tpc.tpc_powergate = NULL;
	g->ops.therm.elcg_init_idle_filters = NULL;
	g->ops.ecc.ecc_init_support = NULL;
	g->ops.channel.resume_all_serviceable_ch = NULL;
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}

	/* test the syncpoint paths here */
	nvgpu_set_enabled(g, NVGPU_HAS_SYNCPOINTS, true);
	g->syncpt_unit_size = 0UL;
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}
	g->syncpt_unit_size = 2UL;
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}
	/*
	 * This redundant call will hit the case where memory is already
	 * valid
	 */
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
	err = nvgpu_finalize_poweron(g);
	if (err != 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron returned fail\n");
	}
	nvgpu_dma_free(g, &g->syncpt_mem);
	nvgpu_posix_enable_fault_injection(kmem_fi, true, 0);
	nvgpu_set_power_state(g, NVGPU_STATE_POWERED_OFF);
	err = nvgpu_finalize_poweron(g);
	if (err == 0) {
		unit_return_fail(m,
			"nvgpu_finalize_poweron errantly returned success\n");
	}
	nvgpu_posix_enable_fault_injection(kmem_fi, false, 0);
	nvgpu_dma_free(g, &g->syncpt_mem);

	return UNIT_SUCCESS;
}

int test_poweroff(struct unit_module *m, struct gk20a *g, void *args)
{
	unsigned int i = 0;
	int err;

	/* setup everything to succeed */
	setup_simple_init_func_success(&g->ops.channel.suspend_all_serviceable_ch, i++);
	setup_simple_init_func_success(&g->ops.gr.gr_suspend, i++);
	setup_simple_init_func_success(&g->ops.mm.mm_suspend, i++);
	setup_simple_init_func_success(&g->ops.fifo.fifo_suspend, i++);
	simple_init_func_ptrs_count = i;

	g->ops.clk.suspend_clk_support = no_return;
	g->ops.mc.log_pending_intrs = no_return;
	g->ops.mc.intr_mask = no_return;
	g->ops.falcon.falcon_sw_free = no_return_u32_param;

	err = nvgpu_prepare_poweroff(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_prepare_poweroff returned fail\n");
	}

	/* return fail for each case */
	for (i = 0; i < simple_init_func_ptrs_count; i++) {
		*simple_init_func_ptrs[i] = return_fail;
		err = nvgpu_prepare_poweroff(g);
		if (err == 0) {
			unit_return_fail(m,
			    "nvgpu_prepare_poweroff errantly returned pass\n");
		}
		*simple_init_func_ptrs[i] = return_success;
	}

	/* Cover branches for NULL ptr checks */
	g->ops.mc.intr_mask = NULL;
	g->ops.mc.log_pending_intrs = NULL;
	g->ops.channel.suspend_all_serviceable_ch = NULL;
	g->ops.clk.suspend_clk_support = NULL;
	err = nvgpu_prepare_poweroff(g);
	if (err != 0) {
		unit_return_fail(m, "nvgpu_prepare_poweroff returned fail\n");
	}

	return UNIT_SUCCESS;
}

struct unit_module_test init_tests[] = {
	UNIT_TEST(init_setup_env,			test_setup_env,		NULL, 0),
	UNIT_TEST(get_litter_value,			test_get_litter_value,	NULL, 0),
	UNIT_TEST(init_can_busy,			test_can_busy,		NULL, 0),
	UNIT_TEST(init_get_put,				test_get_put,		NULL, 0),
	UNIT_TEST(init_hal_init,			test_hal_init,		NULL, 0),
	UNIT_TEST(init_check_gpu_state,			test_check_gpu_state,	NULL, 0),
	UNIT_TEST(init_poweron,				test_poweron,		NULL, 0),
	UNIT_TEST(init_poweron_branches,		test_poweron_branches,	NULL, 0),
	UNIT_TEST(init_poweroff,			test_poweroff,		NULL, 0),
	UNIT_TEST(init_free_env,			test_free_env,		NULL, 0),
};

UNIT_MODULE(init, init_tests, UNIT_PRIO_NVGPU_TEST);
