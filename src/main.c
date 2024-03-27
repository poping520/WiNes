#include <stdio.h>

#include "cpu.h"

int main() {

    int a = 10;
    int b = a-- + 10;

    printf("b: %d, a:%d\n", b, a);

    MemoryInterface* mem = mem_create();
    Cpu* cpu = cpu_create();
    cpu_set_memory(cpu, mem);

    {
        // 69 50  ADC #$69
        mem->write(0, 0x69);
        mem->write(1, 0x50);
        cpu_run(cpu);
    }
}