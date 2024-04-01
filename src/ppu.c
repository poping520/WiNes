//
// Created by WangKZ on 2024/3/28.
//

#include "ppu.h"

#include <malloc.h>

// 2KB Video RAM
#define PPU_VRAM_SIZE (2*1024)

struct ppu {

    // Video RAM
    uint8_t vram[PPU_VRAM_SIZE];

    // Object Attribute Memory
    uint8_t oam[256];

    //
    // Memory-mapped registers
    //
    uint16_t vram_addr;

    uint8_t oam_addr;

    // Control register
    union {
        struct {
            uint8_t nametable_addr: 2;
            uint8_t vram_addr_increment: 1;
            uint8_t a1: 1;
            uint8_t a2: 1;
            uint8_t a3: 1;
            uint8_t sprite_size: 1;
            uint8_t a4: 1;
            uint8_t nmi_enable: 1;
        };
        uint8_t val;
    } ctrl;

    // Mask register
    union {
        struct {
            uint8_t greyscale: 1;
            uint8_t show_bgr_left8: 1;
            uint8_t show_spr_left8: 1;
            uint8_t show_bgr: 1;
            uint8_t show_spr: 1;
            uint8_t emphasize_red: 1;
            uint8_t emphasize_green: 1;
            uint8_t emphasize_blue: 1;
        };
        uint8_t val;
    } mask;

    union {
        struct {
            uint8_t open_bus: 5;
            uint8_t spr_overflow: 1;
            uint8_t spr_0_hit: 1;
            uint8_t vblank_has_started: 1;
        };
        uint8_t val;
    } status;

    mapper_t* mapper;

    //
    // Internal registers
    //

    /*
     * Toggles on each write to either PPUSCROLL or PPUADDR, indicating whether this is the first or second write.
     * Clears on reads of PPUSTATUS. Sometimes called the 'write latch' or 'write toggle'.
     */
    uint8_t write_latch: 1;
};


const uint32_t DEFAULT_PALETTES[64] = {
        // 00 -- 0F
        0x626262, 0x001FB2, 0x2404C8, 0x5200B2, 0x730076, 0x800024, 0x730B00, 0x522800,
        0x244400, 0x005700, 0x005C00, 0x005324, 0x003C76, 0x000000, 0x000000, 0x000000,
        // 10 -- 1F
        0xABABAB, 0x0D57FF, 0x4B30FF, 0x8A13FF, 0xBC08D6, 0xD21269, 0xC72E00, 0x9D5400,
        0x607B00, 0x209800, 0x00A300, 0x009942, 0x007DB4, 0x000000, 0x000000, 0x000000,
        // 20 -- 2F
        0xFFFFFF, 0x53AEFF, 0x9085FF, 0xD365FF, 0xFF57FF, 0xFF5DCF, 0xFF7757, 0xFA9E00,
        0xBDC700, 0x7AE700, 0x43F611, 0x26EF7E, 0x2CD5F6, 0x4E4E4E, 0x000000, 0x000000,
        // 30 -- 3F
        0xFFFFFF, 0xB6E1FF, 0xCED1FF, 0xE9C3FF, 0xFFBCFF, 0xFFBDF4, 0xFFC6C3, 0xFFD59A,
        0xE9E681, 0xCEF481, 0xB6FB9A, 0xA9FAC3, 0xA9F0F4, 0xB8B8B8, 0x000000, 0x000000,
};

uint8_t ppu_read(ppu_t* ppu) {

}

void ppu_write(ppu_t* ppu, uint8_t val) {

}


uint8_t ppu_reg_read(ppu_t* ppu, ppu_reg_t reg) {
    switch (reg) {
        case PPUSTATUS: {  // $2002
            uint8_t ret = ppu->status.val;
            // Reading the status register will clear bit 7 mentioned above
            // and also the address latch used by PPUSCROLL and PPUADDR.
            ppu->status.vblank_has_started = 0;
            ppu->write_latch = 0;
            return ret;
        }

        case OAMDATA: { // $2004
            return ppu->oam[ppu->oam_addr];
        }

        case PPUDATA: { // $2007:
            // VRAM read/write data register.
            // After access, the video memory address will increment by an amount determined by bit 2 of $2000.
            uint8_t ret = ppu_read(ppu);
            ppu->vram_addr += ppu->ctrl.vram_addr_increment ? 32 : 0;
            return ret;
        }
        default:
            break;
    }
    return 0;
}

void ppu_reg_write(ppu_t* ppu, ppu_reg_t reg, uint8_t val) {
    switch (reg) {
        case PPUCTRL: // $2000
            ppu->ctrl.val = val;
            break;

        case PPUMASK: // $2001
            ppu->mask.val = val;
            break;

        case OAMADDR: // $2003
            ppu->oam_addr = val;
            break;

        case OAMDATA: // $2004
            ppu->oam[ppu->oam_addr] = val;
            ++ppu->oam_addr;
            break;

        case PPUADDR: // $2006
            if (~ppu->write_latch) {
                // Write upper byte first (w is 0)
                // Clear high 8-bits first
                // Clear 'val' high 2-bits, cause ppu has 16KB address space only ($0000 - $3FFF)
                ppu->vram_addr = (ppu->vram_addr & 0x00FF) | ((val & 0b001111) << 8);
                ppu->write_latch = 0b1;
            } else {
                // Second write (w is 1), clear low 8-bits first
                ppu->vram_addr = (ppu->vram_addr & 0xFF00) | val;
                ppu->write_latch = 0b0;
            }
            break;

        case PPUDATA: // $2007
            // VRAM read/write data register.
            // After access, the video memory address will increment by an amount determined by bit 2 of $2000.
            ppu_write(ppu, val);
            ppu->vram_addr += ppu->ctrl.vram_addr_increment ? 32 : 0;
            break;

        default:
            // not support
            break;
    }
}


ppu_t* ppu_create(mapper_t* mapper) {
    ppu_t* ppu = calloc(1, sizeof(ppu_t));
    ppu->mapper = mapper;
    return ppu;
}