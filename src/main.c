#include <stdio.h>
#include <malloc.h>

#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"

int main() {

//    ppu_t* ppu = ppu_create();
//    cpu_t* cpu = cpu_create(ppu);

    cart_t cart;
    cart_load_rom("../test_nes/nestest.nes", &cart);

//    FILE* f = NULL;
//    fopen_s(&f, "../test_nes/nestest.nes", "rb");
//    fseek(f, 0, SEEK_END);
//    long filesize = ftell(f);
//    fseek(f, 0, SEEK_SET);
//
//    char* data = malloc(filesize);
//    fread_s(data, filesize, filesize, 1, f);
//    fclose(f);
//
//    for (int i = 0; i < filesize; ++i) {
//        mem->write(i, data[i]);
//    }
//
//    cpu_run(cpu, 0x10);

}