//
// bus/controller.c: System bus controller.
//
// CEN64: Cycle-Accurate Nintendo 64 Emulator.
// Copyright (C) 2015, Tyler J. Stachecki.
//
// This file is subject to the terms and conditions defined in
// 'LICENSE', which is part of this source code package.
//

#include "common.h"
#include "bus/address.h"
#include "bus/controller.h"
#include "bus/memorymap.h"
#include "mips.h"
#include "vr4300/cpu.h"

#define NUM_MAPPINGS 1

static int read_uart(void *opaque, uint32_t address, uint32_t *word) {
  Mips * mips = (Mips *) opaque;

  *word = uart_read(mips, address - UARTBASE);
  return 0;
}

static int write_uart(void *opaque, uint32_t address, uint32_t word, uint32_t dqm) {
  Mips * mips = (Mips *) opaque;

  uart_write(mips, address - UARTBASE, word);
  return 0;
}

// Initializes the bus component.
int bus_init(struct bus_controller *bus,
  uint8_t *mem, size_t mem_size, void *uart) {
  memset(bus, 0, sizeof(*bus));
  bus->mem_size = mem_size;
  bus->mem = mem;

  uart_Reset(&bus->mips);

  create_memory_map(&bus->map);
  map_address_range(&bus->map, UARTBASE, UARTSIZE,
    uart, read_uart, write_uart);

  return 0;
}

// Issues a read request to the bus.
int bus_read_word(void *component, uint32_t address, uint32_t *word) {
  const struct memory_mapping *node;
  struct bus_controller *bus;

  memcpy(&bus, component, sizeof(bus));

  if (address < bus->mem_size) {
    memcpy(word, bus->mem + address, sizeof(*word));
    //*word = byteswap_32(*word);
    return 0;
  }

  else if ((node = resolve_mapped_address(&bus->map, address)) == NULL) {
    debug("bus_read_word: Failed to access: 0x%.8X\n", address);

    *word = 0x00000000U;
    return 0;
  }

  return node->on_read(node->instance, address, word);
}

// Issues a write request to the bus.
int bus_write_word(void *component,
  uint32_t address, uint32_t word, uint32_t dqm) {
  const struct memory_mapping *node;
  struct bus_controller *bus;

  memcpy(&bus, component, sizeof(bus));

  if (address < bus->mem_size) {
    uint32_t orig_word;

    memcpy(&orig_word, bus->mem + address, sizeof(orig_word));
    //orig_word = byteswap_32(orig_word) & ~dqm;
    //word = byteswap_32(orig_word | word);
    orig_word = orig_word & ~dqm;
    word = orig_word | (word & dqm);
    memcpy(bus->mem + address, &word, sizeof(word));
    return 0;
  }

  else if ((node = resolve_mapped_address(&bus->map, address)) == NULL) {
    debug("bus_write_word: Failed to access: 0x%.8X\n", address);

    return 0;
  }

  return node->on_write(node->instance, address, word & dqm, dqm);
}

