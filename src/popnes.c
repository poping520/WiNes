//
// Created by WangKZ on 2024/4/1.
//

#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"

void pop_nes_init() {
    cart_t cart;
    cart_load_rom("../test_nes/nestest.nes", &cart);

    mapper_t* mapper = mapper_create(&cart);
    ppu_t* ppu = ppu_create(mapper);
    cpu_t* cpu = cpu_create(ppu);
}