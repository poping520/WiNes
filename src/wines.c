//
// Created by WangKZ on 2024/4/1.
//

#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"
#include "platform.h"

void pop_nes_init() {
    cart_t cart;
    cart_load_rom("../test_nes/nestest.nes", &cart);

    mapper_t* mapper = mapper_create(&cart);
    ppu_t* ppu = ppu_create(mapper);
    cpu_t* cpu = cpu_create(ppu, mapper);

    while (true) {
        // On NTSC system, three PPU ticks per CPU cycle
        cpu_cycle(cpu);

        ppu_cycle(ppu);
        ppu_cycle(ppu);
        ppu_cycle(ppu);
        wn_msleep(5);
    }
}