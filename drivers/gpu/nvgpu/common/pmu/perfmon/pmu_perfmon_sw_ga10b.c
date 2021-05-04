/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/pmu/pmu_perfmon.h>
#include <nvgpu/log.h>

#include "pmu_perfmon_sw_ga10b.h"

void nvgpu_next_perfmon_sw_init(struct gk20a *g,
		struct nvgpu_pmu_perfmon *perfmon)
{
	nvgpu_log_fn(g, " ");

	perfmon->init_perfmon = nvgpu_pmu_init_perfmon_rpc;
	perfmon->start_sampling = nvgpu_pmu_perfmon_start_sampling_rpc;
	perfmon->stop_sampling = nvgpu_pmu_perfmon_stop_sampling_rpc;
	perfmon->get_samples_rpc = nvgpu_pmu_perfmon_get_samples_rpc;
	perfmon->perfmon_event_handler = nvgpu_pmu_handle_perfmon_event_rpc;
}
