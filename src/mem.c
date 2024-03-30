//
// Created by WangKZ on 2024/3/26.
//

#include "mem.h"

#include <malloc.h>


/*
 * CPU Memory Map
 *
 * +---------+--------------------~ $10000
 * |         |    Reset/IRQ/NMI   |
 * |         |--------------------~ $FFFA
 * |         | PRG-ROM Upper Bank |
 * |         |--------------------~ $C000
 * |         | PRG-ROM Lower Bank |
 * |         |--------------------~ $8000
 * |         |        WRAM        |
 * |         |--------------------~ $6000
 * |         |   Expansion ROM    |
 * |         |--------------------~ $4020
 * |         | Other I/O Register |
 * |         |--------------------~ $4000
 * |         |      Mirrors       |
 * |         |   $2000 - $2007    |
 * |         |--------------------~ $2008
 * |         |  PPU I/O Register  |
 * +---------+--------------------~ $2000
 * |         |      Mirrors       |
 * |         |   $0000 - $07FF    |
 * |         |--------------------~ $0800
 * | CPU RAM |        RAM         |
 * |         |--------------------~ $0200
 * |         |       Stack        |
 * |         |--------------------~ $0100
 * |         |     Zero Page      |
 * +---------+--------------------~ $0000
 */

// 2KB CPU ARM
#define CPU_RAM_SIZE (2*1024)

typedef struct Memory {
    uint8_t cpu_ram[CPU_RAM_SIZE];
    ppu_t* ppu;
} Memory_t;

Memory_t* memory = NULL;

static uint8_t read(Addr_t addr) {
    if (memory == NULL) {
        return 0;
    }

    if (addr < 0x2000) { // CPU RAM
        if (addr > 0x7FF) { // Mirrors
            addr = addr % 800;
        }
        return memory->cpu_ram[addr];
    } else if (addr < 0x4000) { // PPU registers
        return ppu_reg_read(memory->ppu, (ppu_reg_t) addr % 8);
    }

    return 0;
}

static void write(Addr_t addr, uint8_t val) {
    if (memory == NULL) {
        return;
    }

    if (addr < 0x2000) { // CPU RAM
        if (addr > 0x7FF) { // Mirrors
            addr = addr % 800;
        }
        memory->cpu_ram[addr] = val;
    } else if (addr < 0x4000) { // PPU registers
        ppu_reg_write(memory->ppu, (ppu_reg_t) addr % 8, val);
    }

}

MemoryInterface* mem_create(ppu_t* ppu) {
    MemoryInterface* mem = malloc(sizeof(MemoryInterface));
    mem->read = read;
    mem->write = write;

    memory = calloc(1, sizeof(Memory_t));
    memory->ppu = ppu;
    return mem;
}

void mem_release(MemoryInterface* mem) {
    if (mem) {
        free(mem);
    }
    if (memory) {
        free(memory);
    }
}