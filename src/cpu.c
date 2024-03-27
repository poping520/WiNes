//
// Created by WangKZ on 2024/3/26.
//

#include "cpu.h"
#include <malloc.h>


/**
 * CPU Memory Map
 *
 * ~---------------~ $2000
 * |    Mirrors    |
 * | $0000 - $07FF |
 * ~---------------~ $0800
 * |      RAM      |
 * ~---------------~ $0200
 * |     Stack     |
 * ~---------------~ $0100
 * |   Zero Page   |
 * ~---------------~ $0000
 */

struct Cpu {

    uint16_t pc;

    uint8_t sp;

    // Accumulator
    uint8_t a;

    // Index registers
    uint8_t x, y;

    // Status register
    uint8_t p;

    MemoryInterface* mem;

    uint32_t cycles;
};

Cpu* cpu_create() {
    Cpu* cpu = calloc(1, sizeof(Cpu));
    return cpu;
}

void cpu_set_memory(Cpu* cpu, MemoryInterface* mem) {
    if (cpu) {
        cpu->mem = mem;
    }
}


inline void _set_flag(Cpu* cpu, enum CpuFlag flag, bool value) {
    if (value) {
        // 0001 0000 & 1110 0101 -> 1111 0101
        cpu->p |= flag;
    } else {
        // ~(0001 0000) -> 1110 1111 & 1111 0101 -> 1110 0101
        cpu->p &= ~flag;
    }
}

inline uint8_t _get_flag(Cpu* cpu, enum CpuFlag flag) {
    // 0001_0000 & 0101_0101 -> 0001_0000
    // 0001_0000 & 0100_0101 -> 0000_0000
    return (cpu->p & flag) > 0 ? CPU_FLAG_SET : CPU_FLAG_CLR;
}

#define STACK_BASE 0x100

#define ARG_CPU         cpu
#define ARG_ADDR        addr
#define DECL_ARG_CPU    Cpu* ARG_CPU
#define DECL_ARG_ADDR   Addr_t ARG_ADDR
#define DECL_AM(M)      static uint16_t (AM_##M)(DECL_ARG_CPU)
#define DECL_OP(M)      static void (OP_##M)(DECL_ARG_CPU, DECL_ARG_ADDR)


#define set_flag(flag, val) _set_flag(ARG_CPU, flag, val)
#define get_flag(flag)      _get_flag(ARG_CPU, flag)

// xxxx xxxx & 1000 0000
#define set_zn_flag(val)        \
    set_flag(ZERO_FLAG, (val) == 0);   \
    set_flag(NEGATIVE_FLAG, ((val) >> 7) & 0b1)


#define PC (ARG_CPU->pc)
#define SP (ARG_CPU->sp)
#define RA (ARG_CPU->a)
#define RX (ARG_CPU->x)
#define RY (ARG_CPU->y)
#define RP (ARG_CPU->p)
#define CYCLES (ARG_CPU->cycles)


#define mem_read(addr)          ARG_CPU->mem->read(addr)
#define mem_write(addr, val)    ARG_CPU->mem->write(addr, val)

#define mem_read_operand()      mem_read(ARG_ADDR)
#define mem_write_operand(val)  mem_write(ARG_ADDR, val)

#define mem_push_stack(val)     mem_write((Addr_t)(SP-- + STACK_BASE), val)
#define mem_pop_stack()         mem_read(++SP + STACK_BASE)

/**
 * Memory max offset: 0xFFFF
 * Memory page size : 0x100  -> 2^8
 *
 * Address: 0x 12 34
 *             |  |-- Low  8 bit: In-Page offset
 *             |----- High 8 bit: Page number
 */
#define is_same_page(addr1, addr2) (((addr1) >> 8) == ((addr2) >> 8))

// addressing mode
DECL_AM(IMP) {

}

DECL_AM(ACC) {

}

/**
 *
 */
DECL_AM(IMM) {
    return PC++;
}

/**
 * Zero Page
 * 2 byte
 */
DECL_AM(ZP) {
    return mem_read(PC++);
}

// zero page X
DECL_AM(ZPX) {

}

// zero page Y
DECL_AM(ZPY) {

}

DECL_AM(IZX) {

}

DECL_AM(IZY) {

}

DECL_AM(ABS) {

}

DECL_AM(ABX) {

}

DECL_AM(ABY) {

}

DECL_AM(IND) {

}

DECL_AM(REL) {

}

/*
 * Implements 6502 opcodes
 *
 * Docs:
 * https://www.nesdev.org/obelisk-6502-guide/instructions.html
 * https://www.nesdev.org/obelisk-6502-guide/reference.html
 */


// Load/Store Operations
DECL_OP(LDA) {

}

DECL_OP(LDX) {

}

DECL_OP(LDY) {

}

DECL_OP(STA) {

}

DECL_OP(STX) {

}

DECL_OP(STY) {

}

//
// Register Transfers
//

/**
 * TAX - Transfer Accumulator to X
 * X = A
 * Copies the current contents of the accumulator into the X register and sets the zero and negative flags as appropriate.
 *
 * Zero Flag	    Set if X = 0
 * Negative Flag	Set if bit 7 of X is set
 */
DECL_OP(TAX) {
    RX = RA;
    set_zn_flag(RX);
}

/**
 * TAY - Transfer Accumulator to Y
 * Y = A
 *
 * See also: TAX
 */
DECL_OP(TAY) {
    RY = RA;
    set_zn_flag(RY);
}

/**
 * TXA - Transfer X to Accumulator
 * A = X
 *
 * See also: TAX
 */
DECL_OP(TXA) {
    RA = RX;
    set_zn_flag(RA);
}

/**
 * TYA - Transfer Y to Accumulator
 * A = Y
 *
 * See also: TAX
 */
DECL_OP(TYA) {
    RA = RY;
    set_zn_flag(RA);
}

//
// Stack Operations
//

/**
 * TSX - Transfer Stack Pointer to X
 * X = S
 * Copies the current contents of the stack register into the X register and sets the zero and negative flags as appropriate.
 *
 * Zero Flag:   	Set if X = 0
 * Negative Flag:	Set if bit 7 of X is set
 */
DECL_OP(TSX) {
    RX = SP;
    set_zn_flag(RX);
}

/**
 * TXS - Transfer X to Stack Pointer
 * S = X
 * Copies the current contents of the X register into the stack register.
 */
DECL_OP(TXS) {
    SP = RX;
}

/**
 * PHA - Push Accumulator
 * Pushes a copy of the accumulator on to the stack.
 */
DECL_OP(PHA) {
    mem_push_stack(RA);
}

/**
 * PHP - Push Processor Status
 * Pushes a copy of the status flags on to the stack.
 */
DECL_OP(PHP) {
    mem_push_stack(RP | BREAK_COMMAND);
}

/**
 * PLA - Pull Accumulator
 * Pulls an 8 bit value from the stack and into the accumulator. The zero and negative flags are set as appropriate.
 *
 * Zero Flag:   	Set if A = 0
 * Negative Flag:	Set if bit 7 of A is set
 */
DECL_OP(PLA) {
    RA = mem_pop_stack();
    set_zn_flag(RA);
}

/**
 * PLP - Pull Processor Status
 * Pulls an 8 bit value from the stack and into the processor flags. The flags will take on new states as determined by the value pulled.
 */
DECL_OP(PLP) {
    RP = mem_pop_stack();
}

//
// Logical
//

/**
 * AND - Logical AND
 * A logical AND is performed, bit by bit, on the accumulator contents using the contents of a byte of memory.
 *
 * set Z, N Flags
 */
DECL_OP(AND) {
    RA = RA & mem_read_operand();
    set_zn_flag(RA);
}

/**
 * EOR - Exclusive OR
 * An exclusive OR is performed, bit by bit, on the accumulator contents using the contents of a byte of memory.
 *
 * set Z, N Flags
 */
DECL_OP(EOR) {
    RA = RA ^ mem_read_operand();
    set_zn_flag(RA);
}

/**
 * ORA - Logical Inclusive OR
 * An inclusive OR is performed, bit by bit, on the accumulator contents using the contents of a byte of memory.
 *
 * set Z, N Flags
 */
DECL_OP(ORA) {
    RA = RA | mem_read_operand();
    set_zn_flag(RA);
}

/**
 * BIT - Bit Test
 * This instructions is used to test if one or more bits are set in a target memory location.
 * The mask pattern in A is ANDed with the value in memory to set or clear the zero flag, but the result is not kept.
 * Bits 7 and 6 of the value from memory are copied into the N and V flags.
 *
 * Zero Flag:	    Set if the result if the AND is zero
 * Overflow Flag:	Set to bit 6 of the memory value
 * Negative Flag:	Set to bit 7 of the memory value
 */
DECL_OP(BIT) {
    uint8_t operand = mem_read_operand();
    uint8_t result = RA & operand;
    set_flag(ZERO_FLAG, result == 0);
    set_flag(OVERFLOW_FLAG, (operand >> 6) & 0b1);
    set_flag(NEGATIVE_FLAG, (operand >> 7) & 0b1);
}

//
// Arithmetic
//

// ADC implements
__forceinline void adc_operand(Cpu* ARG_CPU, uint8_t operand) {
    uint16_t sum = RA + operand + get_flag(CARRY_FLAG);
    set_flag(OVERFLOW_FLAG, (~(RA ^ operand) & (RA ^ sum)) & 0x80);

    RA = (uint8_t) sum;
    set_flag(CARRY_FLAG, sum > 0xFF);
    set_zn_flag(RA);
}

/**
 * ADC - Add with Carry
 *
 * This instruction adds the contents of a memory location to the accumulator together with the carry bit.
 * If overflow occurs the carry bit is set, this enables multiple byte addition to be performed.
 *
 * Carry Flag:   	Set if overflow in bit 7
 * Zero Flag:	    Set if A = 0
 * Overflow Flag:	Set if sign bit is incorrect
 * Negative Flag:	Set if bit 7 set
 */
DECL_OP(ADC) {
    uint8_t operand = mem_read_operand();
    adc_operand(ARG_CPU, operand);
}

/**
 * SBC - Subtract with Carry
 * See also: ADC
 */
DECL_OP(SBC) {
    uint8_t operand = ~mem_read_operand();
    adc_operand(ARG_CPU, operand);
}

// CMP implements
__forceinline void cmp_reg_val(Cpu* ARG_CPU, uint16_t ARG_ADDR, uint8_t reg_val) {
    uint8_t operand = mem_read_operand();
    uint8_t result = reg_val - operand;
    set_flag(CARRY_FLAG, reg_val >= operand);
    set_zn_flag(result);
}

/**
 * CMP - Compare
 * This instruction compares the contents of the accumulator with another memory held value and sets the zero and carry flags as appropriate.
 *
 * Carry Flag:	    Set if A >= M
 * Zero Flag:   	Set if A = M
 * Negative Flag:	Set if bit 7 of the result is set
 */
DECL_OP(CMP) {
    cmp_reg_val(ARG_CPU, ARG_ADDR, RA);
}

/**
 * CPX - Compare X Register
 * see also: CMP
 */
DECL_OP(CPX) {
    cmp_reg_val(ARG_CPU, ARG_ADDR, RX);
}

/**
 * CPX - Compare Y Register
 * see also: CMP
 */
DECL_OP(CPY) {
    cmp_reg_val(ARG_CPU, ARG_ADDR, RY);
}

//
// Increments & Decrements
//

/**
 * INC - Increment Memory \n
 * Adds one to the value held at a specified memory location setting the zero and negative flags as appropriate \n
 * Zero Flag:	    Set if result is zero
 * Negative Flag:	Set if bit 7 of the result is set
 */
DECL_OP(INC) {
    uint8_t result = mem_read_operand() + 1;
    mem_write_operand(result);
    set_zn_flag(result);
}

/**
 * INX - Increment X Register \n
 * Adds one to the X register setting the zero and negative flags as appropriate \n
 * set Z, N Flags
 */
DECL_OP(INX) {
    RX += 1;
    set_zn_flag(RX);
}

/**
 * INY - Increment Y Register
 * Adds one to the Y register setting the zero and negative flags as appropriate
 * set Z, N Flags
 */
DECL_OP(INY) {
    RY += 1;
    set_zn_flag(RY);
}

/**
 * DEC - Decrement Memory \n
 * Subtracts one from the value held at a specified memory location setting the zero and negative flags as appropriate \n
 * set Z, N Flags
 */
DECL_OP(DEC) {
    uint8_t result = mem_read_operand() - 1;
    mem_write_operand(result);
    set_zn_flag(result);
}

/**
 * DEX - Decrement X Register \n
 * Subtracts one from the X register setting the zero and negative flags as appropriate \n
 * set Z, N Flags
 */
DECL_OP(DEX) {
    RX -= 1;
    set_zn_flag(RX);
}

/**
 * DEY - Decrement Y Register \n
 * Subtracts one from the Y register setting the zero and negative flags as appropriate \n
 * set Z, N Flags
 */
DECL_OP(DEY) {
    RY -= 1;
    set_zn_flag(RY);
}

//
// Shifts
//

/**
 * ASL - Arithmetic Shift Left
 * This operation shifts all the bits of the accumulator or memory contents one bit left.
 * Bit 0 is set to 0 and bit 7 is placed in the carry flag.
 * The effect of this operation is to multiply the memory contents by 2 (ignoring 2's complement considerations),
 * setting the carry if the result will not fit in 8 bits
 *
 * Carry Flag:      Set to contents of old bit 7
 * Zero Flag:       Set if A = 0
 * Negative Flag:   Set if bit 7 of the result is set
 */
DECL_OP(ASL) {

}

DECL_OP(LSR) {

}

DECL_OP(ROL) {

}

DECL_OP(ROR) {

}

//
// Jumps & Calls
//

/**
 * JMP - Jump
 * Sets the program counter to the address specified by the operand.
 */
DECL_OP(JMP) {

}

DECL_OP(JSR) {

}

DECL_OP(RTS) {

}

//
// Branches
//

__forceinline void _branch(DECL_ARG_CPU, DECL_ARG_ADDR, bool cond) {
    if (cond) {
        ++CYCLES;
        uint8_t operand = mem_read_operand();
        Addr_t new_addr = PC + operand;
        if (!is_same_page(PC, new_addr)) {
            ++CYCLES;
        }
        PC = new_addr;
    } else {
        ++PC;
    }
}

#define branch_if_set(flag) _branch(ARG_CPU, ARG_ADDR, get_flag(flag) == CPU_FLAG_SET)
#define branch_if_clr(flag) _branch(ARG_CPU, ARG_ADDR, get_flag(flag) == CPU_FLAG_CLR)

/**
 * BCC - Branch if Carry Clear
 * If the carry flag is clear then add the relative displacement to the program counter to cause a branch to a new location.
 */
DECL_OP(BCC) {
    branch_if_clr(CARRY_FLAG);
}

/**
 * BCS - Branch if Carry Set
 * If the carry flag is set then add the relative displacement to the program counter to cause a branch to a new location.
 */
DECL_OP(BCS) {
    branch_if_set(CARRY_FLAG);
}

/**
 * BEQ - Branch if Equal
 * If the zero flag is set then add the relative displacement to the program counter to cause a branch to a new location.
 */
DECL_OP(BEQ) {
    branch_if_set(ZERO_FLAG);
}

/**
 * BMI - Branch if Minus
 * If the negative flag is set then add the relative displacement to the program counter to cause a branch to a new location.
 */
DECL_OP(BMI) {
    branch_if_set(NEGATIVE_FLAG);
}

/**
 * BNE - Branch if Not Equal
 * If the zero flag is clear then add the relative displacement to the program counter to cause a branch to a new location.
 */
DECL_OP(BNE) {
    branch_if_clr(ZERO_FLAG);
}

/**
 * BPL - Branch if Positive
 * If the negative flag is clear then add the relative displacement to the program counter to cause a branch to a new location.
 */
DECL_OP(BPL) {
    branch_if_clr(NEGATIVE_FLAG);
}

/**
 * BVC - Branch if Overflow Clear
 * If the overflow flag is clear then add the relative displacement to the program counter to cause a branch to a new location.
 */
DECL_OP(BVC) {
    branch_if_clr(OVERFLOW_FLAG);
}

/**
 * BVS - Branch if Overflow Set
 * If the overflow flag is set then add the relative displacement to the program counter to cause a branch to a new location.
 */
DECL_OP(BVS) {
    branch_if_set(OVERFLOW_FLAG);
}

//
// Status Flag Changes
//

// Clear Carry Flag
DECL_OP(CLC) {
    set_flag(CARRY_FLAG, false);
}

// Clear Decimal Mode
DECL_OP(CLD) {
    set_flag(DECIMAL_MODE, false);
}

// Clear Interrupt Disable
DECL_OP(CLI) {
    set_flag(INTERRUPT_DISABLE, false);
}

// Clear Overflow Flag
DECL_OP(CLV) {
    set_flag(OVERFLOW_FLAG, false);
}

// Set Carry Flag
DECL_OP(SEC) {
    set_flag(CARRY_FLAG, true);
}

// Set Decimal Flag
DECL_OP(SED) {
    set_flag(DECIMAL_MODE, true);
}

DECL_OP(SEI) {
    set_flag(INTERRUPT_DISABLE, true);
}

// System Functions

DECL_OP(BRK) {

}

DECL_OP(NOP) {

}

DECL_OP(RTI) {


}


typedef struct {
    void (* op_func)(Cpu* cpu, uint16_t);

    uint16_t (* am_func)(Cpu*);

    uint8_t cycles;
} CpuOperation;


CpuOperation op_table[] = {
        {OP_BRK, AM_IMP}, // $00
        {OP_ORA, AM_IZX, 6}, //$01
        {NULL, NULL}, // $02
        {NULL, NULL}, // $03
        {NULL, NULL}, // $04
        {OP_ORA, AM_ZP,  3}, // $05
        {OP_ASL, AM_ZP}, // $06
        {NULL, NULL}, // $07
        {OP_PHP, AM_IMP, 3}, // $08
        {OP_ORA, AM_IMM, 2}, // $09
        {OP_ASL, AM_IMP}, // $0A
        {NULL, NULL}, // $0B
        {NULL, NULL}, // $0C
        {OP_ORA, AM_ABS, 4}, // $0D
        {OP_ASL, AM_ABS}, // $0E
        {NULL, NULL}, // $0F
        {OP_BPL, AM_REL, 2},// $10
        {OP_ORA, AM_IZY, 5}, // $11
        {NULL, NULL}, // $12
        {NULL, NULL}, // $13
        {NULL, NULL}, // $14
        {OP_ORA, AM_ZPX, 4}, // $15
        {OP_ASL, AM_ZPX}, // $16
        {NULL, NULL}, // $17
        {OP_CLC, AM_IMP, 2},// $18
        {OP_ORA, AM_ABY, 4}, // $19
        {NULL, NULL}, // $1A
        {NULL, NULL}, // $1B
        {NULL, NULL}, // $1C
        {OP_ORA, AM_ABX, 4}, // $1D
        {OP_ASL, AM_ABX}, // $1E
        {NULL, NULL}, // $1F
        {OP_JSR, AM_ABS}, // $20
        {OP_AND, AM_IZX, 6}, // $21
        {NULL, NULL},  // $22
        {NULL, NULL}, // $23
        {OP_BIT, AM_ZP,  3},// $24
        {OP_AND, AM_ZP,  3}, // $25
        {OP_ROL, AM_ZP}, // $26
        {NULL, NULL}, // $27
        {OP_PLP, AM_IMP, 4}, // $28
        {OP_AND, AM_IMM, 2}, // $29
        {OP_ROL, AM_IMP}, // $2A
        {NULL, NULL}, // $2B
        {OP_BIT, AM_ABS, 4},// $2C
        {OP_AND, AM_ABS, 4}, // $2D
        {OP_ROL, AM_ABS}, // $2E
        {NULL, NULL}, // $2F
        {OP_BMI, AM_REL, 2},// $30
        {OP_AND, AM_IZY, 5}, // $31
        {NULL, NULL}, // $32
        {NULL, NULL}, // $33
        {NULL, NULL}, // $34
        {OP_AND, AM_ZPX, 4}, // $35
        {OP_ROL, AM_ZPX}, // $36
        {NULL, NULL}, // $37
        {OP_SEC, AM_IMP, 2}, // $38
        {OP_AND, AM_ABY, 4}, // $39
        {NULL, NULL}, // $3A
        {NULL, NULL}, // $3B
        {NULL, NULL}, // $3C
        {OP_AND, AM_ABX, 4}, // $3D
        {OP_ROL, AM_ABX}, // $3E
        {NULL, NULL}, // $3F
        {OP_RTI, AM_IMP}, // $40
        {OP_EOR, AM_IZX, 6}, // $41
        {NULL, NULL}, // $42
        {NULL, NULL}, // $43
        {NULL, NULL}, // $44
        {OP_EOR, AM_ZP,  3}, // $45
        {OP_LSR, AM_ZP}, // $46
        {NULL, NULL}, // $47
        {OP_PHA, AM_IMP, 3}, // $48
        {OP_EOR, AM_IMM, 2}, // $49
        {OP_LSR, AM_IMP}, // $4A
        {NULL, NULL}, // $4B
        {OP_JMP, AM_ABS, 3}, // $4C
        {OP_EOR, AM_ABS, 4}, // $4D
        {OP_LSR, AM_ABS}, // $4E
        {NULL, NULL}, // $4F
        {OP_BVC, AM_REL, 2}, // $50
        {OP_EOR, AM_IZY, 5}, // $51
        {NULL, NULL}, // $52
        {NULL, NULL}, // $53
        {NULL, NULL}, // $54
        {OP_EOR, AM_ZPX, 4}, // $55
        {OP_LSR, AM_ZPX}, // $56
        {NULL, NULL}, // $57
        {OP_CLI, AM_IMP, 2}, // $58
        {OP_EOR, AM_ABY}, // $59
        {NULL, NULL}, // $5A
        {NULL, NULL}, // $5B
        {NULL, NULL}, // $5C
        {OP_EOR, AM_ABX, 5}, // $5D
        {OP_LSR, AM_ABX}, // $5E
        {NULL, NULL}, // $5F
        {OP_RTS, AM_IMP}, // $60
        {OP_ADC, AM_IZX}, // $61
        {NULL, NULL}, // $62
        {NULL, NULL}, // $63
        {NULL, NULL}, // $64
        {OP_ADC, AM_ZP}, // $65
        {OP_ROR, AM_ZP}, // $66
        {NULL, NULL}, // $67
        {OP_PLA, AM_IMP, 4},  // $68
        {OP_ADC, AM_IMM}, // $69
        {OP_ROR, AM_ACC}, // $6A
        {NULL, NULL}, // $6B
        {OP_JMP, AM_IND, 5}, // $6C
        {OP_ADC, AM_ABS}, // $6D
        {OP_ROR, AM_ABS}, // $6E
        {NULL, NULL}, // $6F
        {OP_BVS, AM_REL, 2}, // $70
        {OP_ADC, AM_IZY}, // $71
        {NULL, NULL}, // $72
        {NULL, NULL}, // $73
        {NULL, NULL}, // $74
        {OP_ADC, AM_ZPX}, // $75
        {OP_ROR, AM_ZPX}, // $76
        {NULL, NULL}, // $77
        {OP_SEI, AM_IMP, 2},   // $78
        {OP_ADC, AM_ABY}, // $79
        {NULL, NULL}, // $7A
        {NULL, NULL}, // $7B
        {NULL, NULL}, // $7C
        {OP_ADC, AM_ABX},// $7D
        {OP_ROR, AM_ABX}, // $7E
        {NULL, NULL}, // $7F
        {NULL, NULL}, // $80
        {OP_STA, AM_IZX}, // $81
        {NULL, NULL}, // $82
        {NULL, NULL}, // $83
        {OP_STY, AM_ZP}, // $84
        {OP_STA, AM_ZP}, // $85
        {OP_STX, AM_ZP}, // $86
        {NULL, NULL}, // $87
        {OP_DEY, AM_IMP, 2}, // $88
        {NULL, NULL}, // $89
        {OP_TXA, AM_IMP, 2}, // $8A
        {NULL, NULL}, // $8B
        {OP_STY, AM_ABS}, // $8C
        {OP_STA, AM_ABS}, // $8D
        {OP_STX, AM_ABS}, // $8E
        {NULL, NULL}, // $8F
        {OP_BCC, AM_REL, 2}, // $90
        {OP_STA, AM_IZY}, // $91
        {NULL, NULL}, // $92
        {NULL, NULL}, // $93
        {OP_STY, AM_ZPX}, // $94
        {OP_STA, AM_ZPX}, // $95
        {OP_STX, AM_ZPY}, // $96
        {NULL, NULL}, // $97
        {OP_TYA, AM_IMP}, // $98
        {OP_STA, AM_ABY}, // $99
        {OP_TXS, AM_IMP, 2}, // $9A
        {NULL, NULL}, // $9B
        {NULL, NULL}, // $9C
        {OP_STA, AM_ABX}, // $9D
        {NULL, NULL}, // $9E
        {NULL, NULL}, // $9F
        {OP_LDY, AM_IMM}, // $A0
        {OP_LDA, AM_IZX}, // $A1
        {OP_LDX, AM_IMM}, // $A2
        {NULL, NULL}, // $A3
        {OP_LDY, AM_ZP}, // $A4
        {OP_LDA, AM_ZP}, // $A5
        {OP_LDX, AM_ZP}, // $A6
        {NULL, NULL}, // $A7
        {OP_TAY, AM_IMP, 2}, // $A8
        {OP_LDA, AM_IMM}, // $A9
        {OP_TAX, AM_IMP, 2},  // $AA
        {NULL, NULL}, // $AB
        {OP_LDY, AM_ABS}, // $AC
        {OP_LDA, AM_ABS}, // $AD
        {OP_LDX, AM_ABS}, // $AE
        {NULL, NULL}, // $AF
        {OP_BCS, AM_REL, 2}, // $B0
        {OP_LDA, AM_IZY}, // $B1
        {NULL, NULL}, // $B2
        {NULL, NULL}, // $B3
        {OP_LDY, AM_ZPX}, // $B4
        {OP_LDA, AM_ZPX}, // $B5
        {OP_LDX, AM_ZPY}, // $B6
        {NULL, NULL}, // $B7
        {OP_CLV, AM_IMP, 2}, // $B8
        {OP_LDA, AM_ABY}, // $B9
        {OP_TSX, AM_IMP, 2},  // $BA
        {NULL, NULL}, // $BB
        {OP_LDY, AM_ABX}, // $BC
        {OP_LDA, AM_ABX}, // $BD
        {OP_LDX, AM_ABY}, // $BE
        {NULL, NULL}, // $BF
        {OP_CPY, AM_IMM}, // $C0
        {OP_CMP, AM_IZX}, // $C1
        {NULL, NULL}, // $C2
        {NULL, NULL}, // $C3
        {OP_CPY, AM_ZP}, // $C4
        {OP_CMP, AM_ZP}, // $C5
        {OP_DEC, AM_ZP,  5}, // $C6
        {NULL, NULL}, // $C7
        {OP_INY, AM_IMP}, // $C8
        {OP_CMP, AM_IMM}, // $C9
        {OP_DEX, AM_IMP, 2}, // $CA
        {NULL, NULL}, // $CB
        {OP_CPY, AM_ABS}, // $CC
        {OP_CMP, AM_ABS}, // $CD
        {OP_DEC, AM_ABS}, // $CE
        {NULL, NULL}, // $CF
        {OP_BNE, AM_REL, 2}, // $D0
        {OP_CMP, AM_IZY}, // $D1
        {NULL, NULL}, // $D2
        {NULL, NULL}, // $D3
        {NULL, NULL}, // $D4
        {OP_CMP, AM_ZPX}, // $D5
        {OP_DEC, AM_ZPX}, // $D6
        {NULL, NULL}, // $D7
        {OP_CLD, AM_IMP, 2}, // $D8
        {OP_CMP, AM_ABY}, // $D9
        {NULL, NULL}, // $DA
        {NULL, NULL}, // $DB
        {NULL, NULL}, // $DC
        {OP_CMP, AM_ABX}, // $DD
        {OP_DEC, AM_ABX}, // $DE
        {NULL, NULL}, // $DF
        {OP_CPX, AM_IMM}, // $E0
        {OP_SBC, AM_IZX}, // $E1
        {NULL, NULL}, // $E2
        {NULL, NULL}, // $E3
        {OP_CPX, AM_ZP}, // $E4
        {OP_SBC, AM_ZP}, // $E5
        {OP_INC, AM_ZP}, // $E6
        {NULL, NULL}, // $E7
        {OP_INX, AM_IMP}, // $E8
        {OP_SBC, AM_IMM}, // $E9
        {OP_NOP, AM_IMP},  // $EA
        {NULL, NULL}, // $EB
        {OP_CPX, AM_ABS}, // $EC
        {OP_SBC, AM_ABS}, // $ED
        {OP_INC, AM_ABS}, // $EE
        {NULL, NULL}, // $EF
        {OP_BEQ, AM_REL, 2},// $F0
        {OP_SBC, AM_IZY}, // $F1
        {NULL, NULL}, // $F2
        {NULL, NULL}, // $F3
        {NULL, NULL}, // $F4
        {OP_SBC, AM_ZPX}, // $F5
        {OP_INC, AM_ZPX}, // $F6
        {NULL, NULL}, // $F7
        {OP_SED, AM_IMP, 2},  // $F8
        {OP_SBC, AM_ABY}, // $F9
        {NULL, NULL}, // $FA
        {NULL, NULL}, // $FB
        {NULL, NULL}, // $FC
        {OP_SBC, AM_ABX}, // $FD
        {OP_INC, AM_ABX}, // $FE
        {NULL, NULL}, // $FF
};

void cpu_run(Cpu* ARG_CPU) {
    uint8_t opcode = mem_read(PC++);
    CpuOperation opera = op_table[opcode];

    uint8_t addr = opera.am_func(ARG_CPU);
    opera.op_func(ARG_CPU, ARG_ADDR);
}