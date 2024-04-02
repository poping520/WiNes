//
// Created by WangKZ on 2024/4/2.
//

#ifndef WINES_MAPPER0_NROM_H
#define WINES_MAPPER0_NROM_H

#include "../mapper.h"

/*
 * NROM-256 with 32 KiB PRG ROM and 8 KiB CHR ROM
 * NROM-128 with 16 KiB PRG ROM and 8 KiB CHR ROM
 *
 * CPU $8000-$BFFF: First 16 KB of ROM.
 * CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
 *
 * NES 2.0 header
 * .segment "HEADER"
 *     .byte "NES", $1A
 *     .byte 2         ; 1 or 2 for NROM-128 or NROM-256 respectively
 */

#define MAPPER_000_NROM 0

#define IS_NROM_128 (*(bool*) mapper->extra)

static uint8_t mapper0_cpu_read(mapper_t* mapper, addr_t addr) {
    if (IS_NROM_128) {
        addr %= (16 * 1024);
    }
    return mapper->cart->pgr_rom[addr];
}

static void mapper0_cpu_write(mapper_t* mapper, addr_t addr, uint8_t val) {

}

static uint8_t mapper0_ppu_read(mapper_t* mapper, addr_t addr) {
    return mapper->cart->chr_rom[addr];
}

static void mapper0_ppu_write(mapper_t* mapper, addr_t addr, uint8_t val) {

}

static void mapper0_destroy(mapper_t* mapper) {
    wn_free(mapper->extra);
}

mapper_func_t mapper0_nrom_init(mapper_t* mapper) {
    mapper->extra = wn_malloc(sizeof(bool));
    IS_NROM_128 = (mapper->cart->header.pgr_blocks == 1);

    mapper_func_t ret = {
            mapper0_cpu_read,
            mapper0_cpu_write,
            mapper0_ppu_read,
            mapper0_ppu_write,
            mapper0_destroy
    };
    return ret;
}

#endif //WINES_MAPPER0_NROM_H
