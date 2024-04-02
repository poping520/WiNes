//
// Created by WangKZ on 2024/3/28.
//

#ifndef WINES_PPU_H
#define WINES_PPU_H

#include "common.h"
#include "mapper.h"

/*
 * The PPU exposes eight memory-mapped registers to the CPU.
 *
 * These nominally sit at $2000 through $2007 in the CPU's address space,
 * but because their addresses are incompletely decoded, they're mirrored in every 8 bytes from $2008 through $3FFF.
 *
 * PPUCTRL: $2000, write
 *  flags:  7 -bit- 0
 *          VPHB SINN
 *
 *      NN: Base nametable address
 *          (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
 *
 *      I:  VRAM address increment per CPU read/write of PPUDATA
 *          (0: add 1, going across; 1: add 32, going down)
 *
 *      S:  Sprite pattern table address for 8x8 sprites
 *          (0: $0000; 1: $1000; ignored in 8x16 mode)
 *
 *      B:  Background pattern table address
 *          (0: $0000; 1: $1000)
 *
 *      H:  Sprite size
 *          (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
 *
 *      P:  PPU master/slave select
 *          (0: read backdrop from EXT pins; 1: output color on EXT pins)
 *
 *      V:  Generate an NMI at the start of the vertical blanking interval
 *          (0: off; 1: on)
 *
 * PPUMASK: $2001, write
 *  flags:  7 -bit- 0
 *          BGRs bMnG
 *
 *      G:  Greyscale
 *          (0: normal color, 1: produce a greyscale display)
 *
 *      m:  1: Show background in leftmost 8 pixels of screen, 0: Hide
 *
 *      M:  1: Show sprites in leftmost 8 pixels of screen, 0: Hide
 *
 *      b:  1: Show background
 *
 *      s:  1: Show sprites
 *
 *      R:  Emphasize red (green on PAL/Dendy)
 *
 *      G:  Emphasize green (red on PAL/Dendy)
 *
 *      B:  Emphasize blue
 *
 * PPUSTATUS: $2002, read
 *
 *
 * OAMADDR: $2003, write
 *      Write the address of OAM you want to access here. Most games just write $00 here and then use OAMDMA.
 *      (DMA is implemented in the 2A03/7 chip and works by repeatedly writing to OAMDATA)
 *
 * PPUSCROLL: $2005, write x2
 *
 * PPUADDR: $2006, write x2
 *      Because the CPU and the PPU are on separate buses, neither has direct access to the other's memory.
 *      The CPU writes to VRAM through a pair of registers on the PPU by first loading an address into PPUADDR and then it writing data repeatedly to PPUDATA
 *
 * PPUDATA: $2007, read/write
 *      VRAM read/write data register. After access, the video memory address will increment by an amount determined by bit 2 of $2000.
 *
 *
 * Reference:
 * https://www.nesdev.org/wiki/PPU_registers
 */
typedef enum {
    PPUCTRL = 0,
    PPUMASK,        // $2001 >  write
    PPUSTATUS,      // $2002 <  read
    OAMADDR,        // $2003 >  write
    OAMDATA,        // $2004 <> read/write
    PPUSCROLL,      // $2005 >> write x2
    PPUADDR,        // $2006 >> write x2
    PPUDATA,        // $2007 <> read/write
} ppu_reg_t;


typedef struct ppu ppu_t;

void ppu_cycle(ppu_t* ppu);

uint8_t ppu_reg_read(ppu_t* ppu, ppu_reg_t reg);

void ppu_reg_write(ppu_t* ppu, ppu_reg_t reg, uint8_t val);

ppu_t* ppu_create(mapper_t* mapper);

void ppu_destroy(ppu_t* ppu);

#endif //WINES_PPU_H
