/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_FB_TU104_H
#define NVGPU_FB_TU104_H

#include <nvgpu/types.h>

struct gk20a;
struct nvgpu_mem;

int  fb_tu104_tlb_invalidate(struct gk20a *g, struct nvgpu_mem *pdb);
#ifdef CONFIG_NVGPU_COMPRESSION
struct nvgpu_cbc;
void tu104_fb_cbc_configure(struct gk20a *g, struct nvgpu_cbc *cbc);
#endif
int  tu104_fb_apply_pdb_cache_war(struct gk20a *g);
size_t tu104_fb_get_vidmem_size(struct gk20a *g);
int  tu104_fb_enable_nvlink(struct gk20a *g);

#endif /* NVGPU_FB_TU104_H */
