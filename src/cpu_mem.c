//
// Created by WangKZ on 2024/4/1.
//

#include "cpu.h"
#include "ppu.h"

/*
 * CPU Memory Map
 *
 * +----------+--------------------~ $10000
 * |          |    Reset/IRQ/NMI   |
 * |          |--------------------~ $FFFA
 * |          | PRG-ROM Upper Bank |
 * |          |--------------------~ $C000
 * |          | PRG-ROM Lower Bank |
 * |          |--------------------~ $8000
 * |          |        WRAM        |
 * |          |--------------------~ $6000
 * |          |   Expansion ROM    |
 * |----------+--------------------~ $4020
 * |          | Other I/O Register |
 * |          |--------------------~ $4000
 * |  Memory  |      Mirrors       |
 * |  mapped  |   $2000 - $2007    |
 * | register |--------------------~ $2008
 * |          |  PPU I/O Register  |
 * |----------+--------------------~ $2000
 * |          |      Mirrors       |
 * |          |   $0000 - $07FF    |
 * |          |--------------------~ $0800
 * | CPU RAM  |        RAM         |
 * |          |--------------------~ $0200
 * |          |       Stack        |
 * |          |--------------------~ $0100
 * |          |     Zero Page      |
 * +----------+--------------------~ $0000
 */

uint8_t cpu_mem_read(cpu_t* cpu, addr_t addr) {

    if (addr < 0x2000) { // CPU RAM
        if (addr > 0x7FF) { // Mirrors
            addr = addr % 800;
        }
        return cpu->ram[addr];
    } else if (addr < 0x4000) { // PPU registers
        return ppu_reg_read(cpu->ppu, (ppu_reg_t) addr % 8);
    } else if (addr < 0xFFFF) {

    }

    return 0;
}

void cpu_mem_write(cpu_t* cpu, addr_t addr, uint8_t val) {
    if (addr < 0x2000) { // CPU RAM
        if (addr > 0x7FF) { // Mirrors
            addr = addr % 800;
        }
        cpu->ram[addr] = val;
    } else if (addr < 0x4000) { // PPU registers
        ppu_reg_write(cpu->ppu, (ppu_reg_t) addr % 8, val);
    } else if (addr < 0x4020) {
        if (addr == 0x4014) {
            // OAM DMA
            cpu->oam_dma_flag = true;
            cpu->oam_dma_addr = val << 8;
        }
    }
}

