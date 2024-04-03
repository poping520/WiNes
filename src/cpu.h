//
// Created by WangKZ on 2024/3/26.
//

#ifndef WINES_CPU_H
#define WINES_CPU_H

#include "common.h"


/*
 * MOS 6502 CPU
 *
 * Architecture: 8-bit processor with a 16-bit address bus
 * Addressable memory: Up to 64 KB
 * Data Bus Width: 8-bit
 * Clock rate: 1 MHz - 3 MHz
 * Registers:
 *     8-bit accumulator (A)
 *     Two 8-bit index registers (X and Y)
 *     8-bit stack pointer (SP)
 *     16-bit program counter (PC)
 *     8-bit status register (P)
 *
 * Documents:
 * https://www.nesdev.org/obelisk-6502-guide/index.html
 *
 *
 * Ricoh 2A03 CPU
 *
 * The Ricoh 2A03 contains an unlicensed derivative of the MOS Technology 6502 core,
 * modified to disable the 6502's binary-coded decimal mode (possibly to avoid a MOS Technology patent)
 *
 * Clock rate: 1.79 MHz
 *
 * https://en.wikipedia.org/wiki/Ricoh_2A03
 *
 */

#define CPU_RAM_SIZE (2*1024)   // 2KB CPU ARM

typedef struct mapper mapper_t;
typedef struct ppu ppu_t;

typedef struct cpu {

    uint8_t ram[CPU_RAM_SIZE];

    bool nmi;

    // Program counter
    uint16_t pc;

    // Stack pointer
    uint8_t sp;

    // Accumulator
    uint8_t a;

    // Index registers
    uint8_t x, y;

    // Status register
    uint8_t p;

    uint32_t cycles;

    // Accumulator addressing mode
    bool am_acc_flag;

    bool oam_dma_flag;

    uint16_t oam_dma_addr;

    ppu_t* ppu;

    mapper_t* mapper;
} cpu_t;

/**
 * The processor status register has 8 bits, where 7 are used as flags:
 *
 * Binary:
 *   7  bit  0
 *   NV#B DIZC
 *
 * N = Negative Flag (1 when result is negative)
 * V = Overflow Flag (1 on signed overflow)
 * # = unused (always 1)
 * B = Break Command (1 when interrupt was caused by a BRK)
 * D = Decimal Mode (1 when CPU in BCD mode)
 * I = Interrupt Disable (when 1, no interrupts will occur (exceptions are IRQs forced by BRK and NMIs))
 * Z = Zero Flag (1 when all bits of a result are 0)
 * C = Carry Flag (1 on unsigned overflow)
 */
enum CpuFlag {
    CARRY_FLAG = 1 << 0,
    ZERO_FLAG = 1 << 1,
    INTERRUPT_DISABLE = 1 << 2,
    DECIMAL_MODE = 1 << 3,
    BREAK_COMMAND = 1 << 4,
    UNUSED = 1 << 5,
    OVERFLOW_FLAG = 1 << 6,
    NEGATIVE_FLAG = 1 << 7
};


uint8_t cpu_mem_read(cpu_t* cpu, addr_t addr);

void cpu_mem_write(cpu_t* cpu, addr_t addr, uint8_t val);

cpu_t* cpu_create(ppu_t* ppu, mapper_t* mapper);

void cpu_cycle(cpu_t* cpu);


#endif //WINES_CPU_H
