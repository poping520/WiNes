//
// Created by WangKZ on 2024/4/1.
//

#ifndef WINES_MAPPER_H
#define WINES_MAPPER_H

#include "common.h"
#include "cartridge.h"

typedef struct mapper {
    uint8_t m;
} mapper_t;

mapper_t* mapper_create(cart_t* cart);

uint8_t mapper_cpu_read(mapper_t* mapper, addr_t addr);

void mapper_cpu_write(mapper_t* mapper, addr_t addr, uint8_t val);

uint8_t mapper_ppu_read(mapper_t* mapper, addr_t addr);

void mapper_ppu_write(mapper_t* mapper, addr_t addr, uint8_t val);

#endif //WINES_MAPPER_H
