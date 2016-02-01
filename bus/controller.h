//
// bus/controller.h: System bus controller.
//
// CEN64: Cycle-Accurate Nintendo 64 Emulator.
// Copyright (C) 2015, Tyler J. Stachecki.
//
// This file is subject to the terms and conditions defined in
// 'LICENSE', which is part of this source code package.
//

#ifndef __bus_controller_h__
#define __bus_controller_h__
#include "common.h"
#include "bus/memorymap.h"
#include "mips.h"
#include <setjmp.h>

struct bus_controller {
  Mips * emu;
  size_t mem_size;
  uint8_t *mem;

  // For resolving physical address ranges to devices.
  struct memory_map map;

  // Allows to to pop back out into device_run during simulation.
  // Kind of a hack to put this in with the device "bus", but at
  // least everyone gets access to it this way.
  jmp_buf unwind_data;
};

cen64_cold int bus_init(struct bus_controller *bus,
  uint8_t *mem, size_t mem_size, Mips * emu);

// General-purpose accesssor functions.
cen64_flatten cen64_hot int bus_read_word(void *component,
  uint32_t address, uint32_t *word);

cen64_flatten cen64_hot int bus_write_word(void *component,
  uint32_t address, uint32_t word, uint32_t dqm);

#endif

