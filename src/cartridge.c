//
// Created by WangKZ on 2024/4/1.
//

#include "cartridge.h"
#include "platform.h"

#define PRG_ROM_BLOCK_SIZE 0x4000   // 16KB
#define CHR_ROM_BLOCK_SIZE 0x2000   // 8KB

err_t cart_load_rom(const char* rom_filename, cart_t* cart) {
    if (rom_filename == NULL || cart == NULL) {
        return ERR_NULLPTR;
    }

    wn_file_t* file = open_file(rom_filename, "rb");
    nes_header_t* header = &cart->header;
    file->read(file, header, sizeof(nes_header_t));

    union {
        struct {
            char _0, _1, _2, _3;
        };
        uint32_t val;
    } NES_MAGIC = {._0 = 'N', ._1 = 'E', ._2= 'S', ._3=0x1A};

    // Check magic number
    if (*(uint32_t*) header->magic != NES_MAGIC.val) {
        return ERR_INVALID_ROM;
    }

    // Size of PRG ROM in 16 KB units
    if (header->pgr_blocks == 0) {
        return ERR_NES_FORMAT;
    }

    cart->pgr_size = header->pgr_blocks * PRG_ROM_BLOCK_SIZE;
    // Size of CHR ROM in 8 KB units (value 0 means the board uses CHR RAM)
    cart->chr_size = header->chr_blocks * CHR_ROM_BLOCK_SIZE;

    cart->pgr_rom = wn_malloc(cart->pgr_size);
    cart->chr_rom = wn_malloc(cart->chr_size);

    file->read(file, cart->pgr_rom, cart->pgr_size);
    file->read(file, cart->chr_rom, cart->chr_size);

    cart->mapper_no = (header->flags7.mapper_no_upper_nybble << 4) | (header->flags6.mapper_no_lower_nybble);

    file->close(file);
    return ERR_OK;
}

void cart_free(cart_t* cart) {
    if (cart != NULL) {
        wn_free(cart->pgr_rom);
        wn_free(cart->chr_rom);
        wn_free(cart);
    }
}