//
// Created by WangKZ on 2024/4/1.
//

#ifndef WINES_MAPPER_H
#define WINES_MAPPER_H

#include "common.h"
#include "cartridge.h"

typedef struct mapper mapper_t;

typedef struct {
    uint8_t (* cpu_read)(mapper_t*, addr_t);

    void (* cpu_write)(mapper_t*, addr_t, uint8_t);

    uint8_t (* ppu_read)(mapper_t*, addr_t);

    void (* ppu_write)(mapper_t*, addr_t, uint8_t);

    void (* destroy)(mapper_t*);
} mapper_func_t;

struct mapper {
    mapper_func_t func;
    cart_t* cart;
    void* extra;
};

uint8_t mapper_cpu_read(mapper_t* mapper, addr_t addr);

void mapper_cpu_write(mapper_t* mapper, addr_t addr, uint8_t val);

uint8_t mapper_ppu_read(mapper_t* mapper, addr_t addr);

void mapper_ppu_write(mapper_t* mapper, addr_t addr, uint8_t val);


mapper_t* mapper_create(cart_t* cart);

void mapper_destroy(mapper_t* mapper);

#endif //WINES_MAPPER_H
