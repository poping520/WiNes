//
// Created by WangKZ on 2024/3/28.
//

#include "ppu.h"
#include "cpu.h"

/*
 * PPU Memory Map
 *
 * +------------------------------------~ $10000
 * |                |      Mirrors      |
 * |                |   $0000 - $3FFF   |
 * |----------------|-------------------~ $4000
 * |                |      Mirrors      |
 * |                |   $3F00 - $3F1F   |
 * |    Palettes    |-------------------~ $3F20
 * |                |   Sprite Palette  |
 * |                |-------------------~ $3F10
 * |                |   Image Palette   |
 * |----------------+-------------------~ $3F00
 * |                |      Mirrors      |
 * |                |   $2000 - $2EFF   |
 * |                |-------------------~ $3000
 * |                | Attribute Table 3 |
 * |                |-------------------~ $2FC0
 * |                |   Name Table 3    |
 * |                |-------------------~ $2C00
 * |                | Attribute Table 2 |
 * |  Name Tables   |-------------------~ $2BC0
 * |                |   Name Table 2    |
 * |                |-------------------~ $2800
 * |                | Attribute Table 1 |
 * |                |-------------------~ $27C0
 * |                |   Name Table 1    |
 * |                |-------------------~ $2400
 * |                | Attribute Table 0 |
 * |                |-------------------~ $23C0
 * |                |   Name Table 0    |
 * |----------------+-------------------~ $2000
 * |                |  Pattern Table 1  |
 * | Pattern Tables |-------------------~ $1000
 * |                |  Pattern Table 1  |
 * +----------------+-------------------~ $0000
 */

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


#define ARG_PPU         ppu
#define DECL_ARG_PPU    ppu_t* ARG_PPU

#define ppu_read(addr)          mapper_ppu_read(ARG_PPU->mapper, addr)
#define ppu_write(addr, val)    mapper_ppu_write(ARG_PPU->mapper, addr, val)

uint32_t ppu_render(DECL_ARG_PPU) {

    if (ppu->mask.show_bgr) {
        uint8_t tile_idx = ppu_read(ppu->reg_v.addr);
    }
}

/*
 * Scanline:
 * PPU 每帧渲染 262 条 scanline，每条 scanline 持续 341 个 PPU 时钟周期，每个时钟周期产生一个像素
 *
 * Per-render scanline (-1 or 261)
 *
 * Visible scanlines (0-239)
 *
 * Post-render scanline (240)
 *
 * Vertical blanking lines (241-260)
 *
 * DOC:
 * https://austinmorlan.com/posts/nes_rendering_overview/
 */
void ppu_cycle(ppu_t* ppu) {

    int16_t scanline = ppu->scanline;

    if (scanline >= 0 && scanline <= 239) {
        // [0, 239]

        ppu_render(ppu);
    } else if (scanline == 240) {
        // 240 do nothing
    } else if (scanline <= 260) {
        // vblank scanlines
        // The VBlank flag of the PPU is set at tick 1 (the second tick) of scanline 241, where the VBlank NMI also occurs
        // The PPU makes no memory accesses during these scanlines, so PPU memory can be freely accessed by the program.
        if (scanline == 241 && ppu->tick == 1) {
            ppu->status.vblank_started = BIT_FLAG_SET;
            if (ppu->ctrl.nmi_enable) {
                ppu->cpu->nmi = true;
            }
        }
    }

    ++ppu->tick;
    if (ppu->tick >= 341) {
        ppu->tick = 0;

        // After 341 ppu cycle, scanline +1
        ++ppu->scanline;
        if (ppu->scanline >= 261) {
            ppu->scanline = -1;
        }
    }
}

uint8_t ppu_reg_read(ppu_t* ppu, ppu_reg_t reg) {
    switch (reg) {
        case PPUSTATUS: {  // $2002
            uint8_t ret = ppu->status.val;
            // Reading the status register will clear bit 7 mentioned above
            // and also the address latch used by PPUSCROLL and PPUADDR.
            ppu->status.vblank_started = BIT_FLAG_CLR;
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

ppu_t* ppu_create(mapper_t* mapper) {
    ppu_t* ppu = wn_calloc(sizeof(ppu_t));
    ppu->mapper = mapper;
    return ppu;
}