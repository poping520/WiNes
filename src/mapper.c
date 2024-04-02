//
// Created by WangKZ on 2024/4/1.
//

#include "mapper.h"
#include "mappers/mapper0_nrom.h"

uint8_t mapper_cpu_read(mapper_t* mapper, addr_t addr) {
    return mapper->func.cpu_read(mapper, addr);
}

void mapper_cpu_write(mapper_t* mapper, addr_t addr, uint8_t val) {
    mapper->func.cpu_write(mapper, addr, val);
}

uint8_t mapper_ppu_read(mapper_t* mapper, addr_t addr) {
    return mapper->func.ppu_read(mapper, addr);
}

void mapper_ppu_write(mapper_t* mapper, addr_t addr, uint8_t val) {
    mapper->func.ppu_write(mapper, addr, val);
}

mapper_t* mapper_create(cart_t* cart) {
    mapper_t* mapper = wn_calloc(sizeof(mapper_t));
    mapper->cart = cart;

    switch (cart->mapper_no) {
        case MAPPER_000_NROM:
            mapper->func = mapper0_nrom_init(mapper);
            break;
        default:
            wn_free(mapper);
            break;
    }
    return mapper;
}

void mapper_destroy(mapper_t* mapper) {
    if (mapper != NULL) {
        if (mapper->func.destroy != NULL) {
            mapper->func.destroy(mapper);
        }
        wn_free(mapper);
    }

}