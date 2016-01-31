//
// vr4300/dcache.h: VR4300 instruction cache.
//
// CEN64: Cycle-Accurate Nintendo 64 Emulator.
// Copyright (C) 2015, Tyler J. Stachecki.
//
// This file is subject to the terms and conditions defined in
// 'LICENSE', which is part of this source code package.
//

#ifndef __vr4300_dcache_h__
#define __vr4300_dcache_h__
#include "common.h"

struct vr4300_dcache_line {
  uint8_t data[4 * 4];
  uint32_t metadata;
};

struct vr4300_dcache {
  struct vr4300_dcache_line lines[512];
};

cen64_cold void vr4300_dcache_init(struct vr4300_dcache *dcache);

void vr4300_dcache_create_dirty_exclusive(
  struct vr4300_dcache *dcache, uint64_t vaddr, uint32_t paddr);
void vr4300_dcache_fill(struct vr4300_dcache *dcache,
  uint64_t vaddr, uint32_t paddr, const void *data);
uint32_t vr4300_dcache_get_tag(const struct vr4300_dcache_line *line, uint64_t vaddr);
void vr4300_dcache_invalidate(struct vr4300_dcache_line *line);
void vr4300_dcache_invalidate_hit(struct vr4300_dcache *dcache,
  uint64_t vaddr, uint32_t paddr);
struct vr4300_dcache_line* vr4300_dcache_probe(
  struct vr4300_dcache *dcache, uint64_t vaddr, uint32_t paddr);
void vr4300_dcache_set_dirty(struct vr4300_dcache_line *line);
struct vr4300_dcache_line *vr4300_dcache_should_flush_line(
  struct vr4300_dcache *dcache, uint64_t vaddr);
struct vr4300_dcache_line *vr4300_dcache_wb_invalidate(
  struct vr4300_dcache *dcache, uint64_t vaddr);
uint32_t vr4300_dcache_get_taglo(struct vr4300_dcache *dcache,
  uint64_t vaddr);
void vr4300_dcache_set_taglo(struct vr4300_dcache *dcache,
  uint64_t vaddr, uint32_t tag);

#endif

