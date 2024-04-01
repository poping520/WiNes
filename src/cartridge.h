//
// Created by WangKZ on 2024/4/1.
//

#ifndef WINES_CARTRIDGE_H
#define WINES_CARTRIDGE_H

#include "common.h"

// NES 2.0 File header structure
typedef struct nes_header {
    uint8_t magic[4];
    uint8_t pgr_blocks;
    uint8_t chr_blocks;
    union {
        struct {
            uint8_t nametable_arrangement: 1;
            uint8_t battery_backed: 1;
            uint8_t trainer: 1;
            uint8_t alternative_nametables: 1;
            uint8_t mapper_no_lower_nybble: 4;
        };
        uint8_t val;
    } flags6;

    union {
        struct {
            uint8_t console_type: 2;
            uint8_t nes_20: 2;
            uint8_t mapper_no_upper_nybble: 4;
        };
        uint8_t val;
    } flags7;

    uint8_t flag8;
    uint8_t flag9;
    uint8_t flag10;
    uint8_t flag11;
    uint8_t flag12;
    uint8_t flag13;
    uint8_t flag14;
    uint8_t flag15;
} nes_header_t;

typedef struct cart {
    uint32_t pgr_size;
    uint32_t chr_size;
    uint8_t* pgr_rom;
    uint8_t* chr_rom;
    uint8_t mapper_no;
} cart_t;

err_t cart_load_rom(const char* rom_filename, cart_t* out);

void cart_free(cart_t* cart);

#endif //WINES_CARTRIDGE_H
