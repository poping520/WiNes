//
// Created by WangKZ on 2024/3/26.
//

#ifndef WINES_MEM_H
#define WINES_MEM_H

#include "common.h"
#include "ppu.h"

typedef struct {
    uint8_t (* read)(Addr_t addr);

    void (* write)(Addr_t addr, uint8_t val);
} MemoryInterface;

MemoryInterface* mem_create(ppu_t* ppu);

void mem_release(MemoryInterface* mem);

#endif //WINES_MEM_H
