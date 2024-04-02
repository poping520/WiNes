//
// Created by WangKZ on 2024/3/28.
//

#include "ppu.h"

// 2KB Video RAM
#define PPU_VRAM_SIZE (2*1024)

typedef union {
    struct {
        uint16_t coarse_x: 5;
        uint16_t coarse_y: 5;
        uint8_t nametable_select: 2;
        uint8_t find_y: 3;
    };
    uint16_t addr: 15;
} inner_reg_t;

struct ppu {

    mapper_t* mapper;

    // Video RAM
    uint8_t vram[PPU_VRAM_SIZE];

    // Object Attribute Memory
    uint8_t oam[256];

    //
    // Memory-mapped registers
    //

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

    //
    // Internal registers
    //

    // Current VRAM address (15 bits)
    inner_reg_t reg_v;

    // Temporary VRAM address (15 bits); can also be thought of as the address of the top left onscreen tile.
    inner_reg_t reg_t;

    // Fine X scroll (3 bits)
    uint8_t reg_x: 3;

    /*
     * First or second write toggle (1 bit)
     *
     * Toggles on each write to either PPUSCROLL or PPUADDR, indicating whether this is the first or second write.
     * Clears on reads of PPUSTATUS. Sometimes called the 'write latch' or 'write toggle'.
     */
    uint8_t reg_w: 1;
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


uint8_t ppu_reg_read(ppu_t* ppu, ppu_reg_t reg) {
    switch (reg) {
        case PPUSTATUS: {  // $2002
            uint8_t ret = ppu->status.val;
            // Reading the status register will clear bit 7 mentioned above
            // and also the address latch used by PPUSCROLL and PPUADDR.
            ppu->status.vblank_has_started = 0;
            ppu->reg_w = 0;
            return ret;
        }

        case OAMDATA: { // $2004
            return ppu->oam[ppu->oam_addr];
        }

        case PPUDATA: { // $2007:
            // VRAM read/write data register.
            // After access, the video memory address will increment by an amount determined by bit 2 of $2000.
            uint8_t ret = mapper_ppu_read(ppu->mapper, ppu->reg_v.addr);
            ppu->reg_v.addr += ppu->ctrl.vram_addr_increment ? 32 : 0;
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
            //    yyy NN YYYYY XXXXX
            // t: ... GH ..... .....  <- d: ......GH
            ppu->reg_t.nametable_select = val & 0b11;
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

        case PPUSCROLL: // $2005
            if (ppu->reg_w == 0) {
                // w is 0

                //    yyy NN YYYYY XXXXX
                // t: .... .. .... ABCDE <- d: ABCDE...
                // x:                FGH <- d: .....FGH
                // w:                    <- 1
                ppu->reg_t.coarse_x = val >> 3;
                ppu->reg_x = val & 0b111;
                ppu->reg_w = 1;
            } else {
                // w is 1

                //    yyy NN YYYYY XXXXX
                // t: FGH .. ABCDE ..... <- d: ABCDEFGH
                // w:                    <- 0
                ppu->reg_t.find_y = val & 0b111;
                ppu->reg_t.coarse_y = val >> 3;
                ppu->reg_w = 0;
            }
            break;

        case PPUADDR: // $2006
            if (ppu->reg_w == 0) {
                // w is 0

                // t: .CDEFGH ........ <- d: ..CDEFGH
                //        <unused>     <- d: AB......
                // t: Z...... ........ <- 0 (bit Z is cleared)
                // w:                  <- 1
                ppu->reg_t.addr &= ~(0x3F << 8); // Clear 8-13 bit
                ppu->reg_t.addr |= (val & 0x3F) << 8; // Set 8-13 bit
                ppu->reg_t.addr &= ~(1 << 14); // Clear bit Z
                ppu->reg_w = 1;
            } else {
                // w is 1

                // t: ....... ABCDEFGH <- d: ABCDEFGH
                // v: <...all bits...> <- t: <...all bits...>
                // w:                  <- 0
                ppu->reg_t.addr &= ~0xFF; // Clear 0-7 bit
                ppu->reg_t.addr |= val; // Set 0-7 bit
                ppu->reg_v.addr = ppu->reg_t.addr;
                ppu->reg_w = 0;
            }
            break;

        case PPUDATA: // $2007
            // VRAM read/write data register.
            // After access, the video memory address will increment by an amount determined by bit 2 of $2000.
            mapper_ppu_write(ppu->mapper, ppu->reg_v.addr, val);
            ppu->reg_v.addr += ppu->ctrl.vram_addr_increment ? 32 : 0;
            break;

        default:
            // not support
            break;
    }
}


void ppu_cycle(ppu_t* ppu) {

}

ppu_t* ppu_create(mapper_t* mapper) {
    ppu_t* ppu = wn_calloc(sizeof(ppu_t));
    ppu->mapper = mapper;
    return ppu;
}