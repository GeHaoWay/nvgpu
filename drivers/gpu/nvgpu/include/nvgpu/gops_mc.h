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
#ifndef NVGPU_GOPS_MC_H
#define NVGPU_GOPS_MC_H

#include <nvgpu/types.h>
#include <nvgpu/mc.h>

/**
 * @file
 *
 * MC HAL interface.
 */
struct gk20a;

/**
 * MC HAL operations.
 *
 * @see gpu_ops.
 */
struct gops_mc {
	/**
	 * @brief Clear the GPU device interrupts at master level.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked before powering off or finishing
	 * SW quiesce of nvgpu driver.
	 *
	 * Steps:
	 * - Write U32_MAX to the stalling interrupts enable clear register.
	 *   mc_intr_en_clear_r are write only registers which clear
	 *   the corresponding bit in INTR_EN whenever a 1 is written
	 *   to it.
	 * - Write U32_MAX to the non-stalling interrupts enable clear register.
	 */
	void (*intr_mask)(struct gk20a *g);

	/**
	 * @brief Enable the applicable GPU device interrupts at master level.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked during #nvgpu_finalize_poweron before
	 * enabling the individual HW units interrupts.
	 *
	 * Steps:
	 * - Get the engine interrupts mask for supported FIFO engines by
	 *   calling #nvgpu_engine_interrupt_mask.
	 * - Clear the GPU device interrupts.
	 * - Set the master level interrupts masks to be used for
	 *   enabling/disabling the interrupts at runtime in
	 *   #intr_stall_pause, #intr_stall_resume,
	 *   #intr_nonstall_pause and #intr_nonstall_resume.
	 *   - Initialize the stalling interrupts bitmask
	 *     #mc_intr_mask_restore[#NVGPU_MC_INTR_STALLING] with various
	 *     units (FIFO, HUB, PRIV_RING, PBUS, LTC) OR'ing with engine
	 *     interrupts mask.
	 *   - Initialize the non-stalling interrupts bitmask
	 *     #mc_intr_mask_restore[#NVGPU_MC_INTR_NONSTALLING] with FIFO
	 *     unit OR'ing with engine interrupts mask.
	 * - Write the bitmasks to the stalling and the non-stalling interrupts
	 *   enable registers respectively (mc_intr_en_set_r()).
	 *
	 * @return 0 in case of success, < 0 in case of failure.
	 */
	int (*intr_enable)(struct gk20a *g);

	/**
	 * @brief Read the the stalling interrupts status register.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked to get the stalling interrupts reported
	 * by the GPU before invoking the ISR.
	 *
	 * Steps:
	 * - Read and return the value of the register
	 *   mc_intr_r(#NVGPU_MC_INTR_STALLING).
	 *
	 * @return value read from mc_intr_r(#NVGPU_MC_INTR_STALLING).
	 */
	u32 (*intr_stall)(struct gk20a *g);

	/**
	 * @brief Disable/Pause the stalling interrupts.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked to disable the stalling interrupts before
	 * the ISR is executed.
	 *
	 * Steps:
	 * - Write U32_MAX to the stalling interrupts enable clear register
	 *   (mc_intr_en_clear_r(#NVGPU_MC_INTR_STALLING)).
	 */
	void (*intr_stall_pause)(struct gk20a *g);

	/**
	 * @brief Interrupt Service Routine (ISR) for handling the stalling
	 *        interrupts.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is called by OS interrupt unit on receiving
	 * stalling interrupt for servicing it.
	 *
	 * Steps:
	 * - Read mc_intr_r(#NVGPU_MC_INTR_STALLING) register to get the
	 *   stalling interrupts reported.
	 * - For the FIFO engines with pending interrupt invoke corresponding
	 *   handlers.
	 *   - Invoke g->ops.gr.intr.stall_isr if GR interrupt is pending.
	 *   - Invoke g->ops.ce.isr_stall if CE interrupt is pending.
	 * - For other units with pending interrupt invoke corresponding
	 *   handlers.
	 *   - Invoke g->ops.fb.intr.isr if HUB interrupt is pending, determined
	 *     by calling g->ops.mc.is_intr_hub_pending.
	 *   - Invoke g->ops.fifo.intr_0_isr if FIFO interrupt is pending. The
	 *     FIFO interrupt bit in mc_intr_r(#NVGPU_MC_INTR_STALLING) is
	 *     mc_intr_pfifo_pending_f.
	 *   - Invoke g->ops.priv_ring.isr if PRIV_RING interrupt is pending.
	 *     The PRIV_RING interrupt bit in mc_intr_r(#NVGPU_MC_INTR_STALLING)
	 *     is mc_intr_priv_ring_pending_f.
	 *   - Invoke g->ops.mc.ltc_isr if LTC interrupt is pending. The
	 *     LTC interrupt bit in mc_intr_r(#NVGPU_MC_INTR_STALLING) is
	 *     mc_intr_ltc_pending_f.
	 *   - Invoke g->ops.bus.isr if BUS interrupt is pending. The
	 *     BUS interrupt bit in mc_intr_r(#NVGPU_MC_INTR_STALLING) is
	 *     mc_intr_pbus_pending_f.
	 */
	void (*isr_stall)(struct gk20a *g);

	/**
	 * @brief Enable/Resume the stalling interrupts.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked to enable the stalling interrupts after
	 * the ISR is executed.
	 *
	 * Steps:
	 * - Enable the stalling interrupts as configured during #intr_enable.
	 *   Write #mc_intr_mask_restore[#NVGPU_MC_INTR_STALLING] to the
	 *   stalling interrupts enable set register
	 *   (mc_intr_en_set_r(#NVGPU_MC_INTR_STALLING)).
	 */
	void (*intr_stall_resume)(struct gk20a *g);

	/**
	 * @brief Read the non-stalling interrupts status register.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked to get the non-stalling interrupts reported
	 * by the GPU before invoking the ISR.
	 *
	 * Steps:
	 * - Read and return the value of the register
	 *   mc_intr_r(#NVGPU_MC_INTR_NONSTALLING).
	 *
	 * @return value read from mc_intr_r(#NVGPU_MC_INTR_NONSTALLING).
	 */
	u32 (*intr_nonstall)(struct gk20a *g);

	/**
	 * @brief Disable/Pause the non-stalling interrupts.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked to disable the non-stalling interrupts
	 * before the ISR is executed.
	 *
	 * Steps:
	 * - Write U32_MAX to the non-stalling interrupts enable clear register
	 *   (mc_intr_en_clear_r(#NVGPU_MC_INTR_NONSTALLING)).
	 */
	void (*intr_nonstall_pause)(struct gk20a *g);

	/**
	 * @brief Interrupt Service Routine (ISR) for handling the non-stalling
	 *        interrupts.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is called by OS interrupt unit on receiving
	 * non-stalling interrupt for servicing it.
	 *
	 * Steps:
	 * - Read mc_intr_r(#NVGPU_MC_INTR_NONSTALLING) register to get the
	 *   non-stalling interrupts reported.
	 * - Invoke g->ops.fifo.intr_1_isr if FIFO non-stalling interrupt
	 *   is pending, determined by calling mc_intr_pfifo_pending_f.
	 * - For the FIFO engines with pending interrupt invoke corresponding
	 *   handlers.
	 *   - Invoke g->ops.gr.intr.nonstall_isr if GR interrupt is pending.
	 *   - Invoke g->ops.ce.isr_nonstall if CE interrupt is pending.
	 *   These functions return bitmask of operations that are executed on
	 *   non-stall workqueue.
	 *
	 * @return bitmask of operations that are executed on non-stall
	 * workqueue.
	 */
	u32 (*isr_nonstall)(struct gk20a *g);

	/**
	 * @brief Enable/Resume the non-stalling interrupts.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked to enable the non-stalling interrupts after
	 * the ISR is executed.
	 *
	 * Steps:
	 * - Enable the non-stalling interrupts as configured during
	 *   #intr_enable.
	 *   Write #mc_intr_mask_restore[#NVGPU_MC_INTR_NONSTALLING]
	 *   to the non-stalling interrupts enable set register
	 *   (mc_intr_en_set_r(#NVGPU_MC_INTR_NONSTALLING)).
	 */
	void (*intr_nonstall_resume)(struct gk20a *g);

	/**
	 * @brief Check if stalling or engine interrupts are pending.
	 *
	 * @param g [in]			The GPU driver struct.
	 * @param act_eng_id [in]		Active engine id.
	 * @param eng_intr_pending [out]	Indicates if engine interrupt
	 *					is pending.
	 *
	 * This function is invoked while polling for preempt completion.
	 *
	 * Steps:
	 * - Read mc_intr_r(#NVGPU_MC_INTR_STALLING) register to get
	 *   the interrupts reported.
	 * - Get the engine interrupt mask corresponding to \a act_eng_id.
	 * - Check if the bits for engine interrupt mask are set in the
	 *   mc_intr_r(#NVGPU_MC_INTR_STALLING) register by AND'ing values
	 *   read in above two steps. Store the result in \a eng_intr_pending.
	 * - Initialize the stalling interrupt mask with bitmask for FIFO, HUB,
	 *   PRIV_RING, PBUS, LTC unit interrupts.
	 * - Return true if bits from above stalling interrupt mask or the
	 *   engine interrupt mask are set in the
	 *   mc_intr_r(#NVGPU_MC_INTR_STALLING) register. Else, return false.
	 *
	 * @return true if stalling or engine interrupt is pending, else false.
	 */
	bool (*is_stall_and_eng_intr_pending)(struct gk20a *g,
				u32 act_eng_id, u32 *eng_intr_pending);

	/**
	 * @brief Reset the HW unit/engine.
	 *
	 * @param g [in]	The GPU driver struct.
	 * @param units [in]	Bitmask of values designating GPU HW engines
	 *			controlled by MC. This is used to update bits in
	 *			the mc_enable_r register.
	 *			- Supported values are:
	 *			  - #NVGPU_UNIT_FIFO
	 *			  - #NVGPU_UNIT_PERFMON
	 *			  - #NVGPU_UNIT_GRAPH
	 *			  - #NVGPU_UNIT_BLG
	 *			  - Reset id of supported engines from the
	 *                          device info. For e.g. GR engine has reset
	 *                          id of 12. @see #nvgpu_device_info.
	 *
	 * This function is invoked to reset the engines while initializing
	 * FIFO, GR and other engines during #nvgpu_finalize_poweron.
	 *
	 * Steps:
	 * - Disable the HW unit/engine.
	 *   - Acquire g->mc_enable_lock spinlock.
	 *   - Read mc_enable_r register and clear the bits in the read value
	 *     corresponding to HW unit to be disabled.
	 *   - Write mc_enable_r with the updated value.
	 *   - Release g->mc_enable_lock spinlock.
	 * - Sleep/wait for 500us if resetting CE engines else sleep for 20us.
	 * - Enable the HW unit/engine.
	 *   - Acquire g->mc_enable_lock spinlock.
	 *   - Read mc_enable_r register and set the bits in the read value
	 *     corresponding to HW unit to be disabled.
	 *   - Write mc_enable_r with the updated value.
	 *   - Read back mc_enable_r.
	 *   - Release g->mc_enable_lock spinlock.
	 *   - Sleep/wait for 20us.
	 */
	void (*reset)(struct gk20a *g, u32 units);

	/**
	 * @brief Get the reset mask for the HW unit/engine.
	 *
	 * @param g [in]	The GPU driver struct.
	 * @param unit [in]	Value designating the GPU HW unit/engine
	 *                      controlled by MC. Supported values are:
	 *			  - NVGPU_UNIT_FIFO
	 *			  - NVGPU_UNIT_PERFMON
	 *			  - NVGPU_UNIT_GRAPH
	 *			  - NVGPU_UNIT_BLG
	 *
	 * This function is invoked to get the reset mask of the engines for
	 * resetting CE, GR, FIFO during #nvgpu_finalize_poweron.
	 *
	 * Steps:
	 * - If \a unit is #NVGPU_UNIT_FIFO, return mc_enable_pfifo_enabled_f.
	 * - else if \a unit is #NVGPU_UNIT_PERFMON,
	 *   return mc_enable_perfmon_enabled_f.
	 * - else if \a unit is #NVGPU_UNIT_GRAPH,
	 *   return mc_enable_pgraph_enabled_f.
	 * - else if \a unit is #NVGPU_UNIT_BLG, return mc_enable_blg_enabled_f.
	 * - else return 0.
	 *
	 * @return bitmask corresponding to supported engines, else 0.
	 */
	u32 (*reset_mask)(struct gk20a *g, enum nvgpu_unit unit);

	/**
	 * @brief Interrupt Service Routine (ISR) for handling the Level Two
	 *        Cache (LTC) interrupts.
	 *
	 * @param g [in]	The GPU driver struct.
	 *
	 * This function is invoked to handle the LTC interrupts from
	 * #isr_stall.
	 *
	 * Steps:
	 * - Read mc_intr_ltc_r register to get the interrupts status for LTCs.
	 * - For each ltc from index 0 to nvgpu_ltc_get_ltc_count(\a g)
	 *   - If interrupt bitmask is set in the interrupts status register
	 *     - Invoke g->ops.ltc.intr.isr.
	 */
	void (*ltc_isr)(struct gk20a *g);

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */

	void (*enable)(struct gk20a *g, u32 units);

	void (*disable)(struct gk20a *g, u32 units);

	bool (*is_enabled)(struct gk20a *g, enum nvgpu_unit unit);

	bool (*is_intr1_pending)(struct gk20a *g, enum nvgpu_unit unit,
				 u32 mc_intr_1);

	bool (*is_mmu_fault_pending)(struct gk20a *g);

	bool (*is_intr_hub_pending)(struct gk20a *g, u32 mc_intr);

	void (*log_pending_intrs)(struct gk20a *g);

	void (*fb_reset)(struct gk20a *g);

#ifdef CONFIG_NVGPU_DGPU
	bool (*is_intr_nvlink_pending)(struct gk20a *g, u32 mc_intr);
	void (*fbpa_isr)(struct gk20a *g);
#endif

#ifdef CONFIG_NVGPU_LS_PMU
	void (*intr_pmu_unit_config)(struct gk20a *g,
			bool enable);
#endif
	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */
};

#endif
