#include <stdio.h>
#include <malloc.h>

#include "cpu.h"

int main() {

    MemoryInterface* mem = mem_create();
    Cpu* cpu = cpu_create();
    cpu_set_memory(cpu, mem);

    FILE* f = NULL;
    fopen_s(&f, "../test_nes/nestest.nes", "rb");
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = malloc(filesize);
    fread_s(data, filesize, filesize, 1, f);
    fclose(f);

    for (int i = 0; i < filesize; ++i) {
        mem->write(i, data[i]);
    }

    cpu_run(cpu, 0x10);

}