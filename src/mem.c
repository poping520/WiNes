//
// Created by WangKZ on 2024/3/26.
//

#include "mem.h"

#include <malloc.h>

#define MEM_SIZE (64*1024)

// 64kb
uint8_t* mm;

static uint8_t read(Addr_t addr) {
    return mm[addr];
}

static void write(Addr_t addr, uint8_t val) {
    mm[addr] = val;
}

MemoryInterface* mem_create() {
    MemoryInterface* mem = malloc(sizeof(MemoryInterface));
    mem->read = read;
    mem->write = write;
    mm = malloc(MEM_SIZE);
    return mem;
}

void mem_release(MemoryInterface* mem) {
    if (mem) {
        free(mem);
    }
    if (mm) {
        free(mm);
    }
}