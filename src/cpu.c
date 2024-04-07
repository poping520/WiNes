//
// Created by WangKZ on 2024/3/26.
//

#include "cpu.h"
#include "mapper.h"
#include "ppu.h"

#include <malloc.h>
#include <stdbool.h>


#define STACK_BASE      0x100

// Non-Maskable Interrupt
#define VECTOR_NIM      0xFFFA
#define VECTOR_RESET    0xFFFC
#define VECTOR_IQR      0xFFFE

#define CPU_FLAG_SET 1
#define CPU_FLAG_CLR 0

FORCE_INLINE void fn_set_flag(cpu_t* cpu, enum CpuFlag flag, bool value) {
    if (value) {
        // 0001 0000 & 1110 0101 -> 1111 0101
        cpu->p |= flag;
    } else {
        // ~(0001 0000) -> 1110 1111 & 1111 0101 -> 1110 0101
        cpu->p &= ~flag;
    }
}

FORCE_INLINE uint8_t fn_get_flag(cpu_t* cpu, enum CpuFlag flag) {
    // 0001_0000 & 0101_0101 -> 0001_0000
    // 0001_0000 & 0100_0101 -> 0000_0000
    return (cpu->p & flag) > 0 ? CPU_FLAG_SET : CPU_FLAG_CLR;
}


#define ARG_CPU         cpu
#define ARG_OP_ADDR     op_addr         // Address of operand
#define DECL_ARG_CPU    cpu_t* ARG_CPU
#define DECL_ARG_ADDR   addr_t ARG_OP_ADDR
#define DECL_FUN_AM(N)  static addr_t AM_##N(DECL_ARG_CPU)
#define DECL_FUN_OP(N)  static void OP_##N(DECL_ARG_CPU, DECL_ARG_ADDR)

#define set_flag(flag, val) fn_set_flag(ARG_CPU, flag, val)
#define get_flag(flag)      fn_get_flag(ARG_CPU, flag)

/**
 * Zero Flag:	    Set if A = 0 \n
 * Negative Flag:	Set if bit 7 set
 *
 * xxxx xxxx & 1000 0000
 */
#define set_zn_flag(val)                \
    set_flag(ZERO_FLAG, (val) == 0);   \
    set_flag(NEGATIVE_FLAG, ((val) >> 7) & 0b1)


#define PC          (ARG_CPU->pc)
#define SP          (ARG_CPU->sp)
#define A           (ARG_CPU->a)
#define X           (ARG_CPU->x)
#define Y           (ARG_CPU->y)
#define P           (ARG_CPU->p)
#define CYCLES      (ARG_CPU->cycles)
#define AM_ACC_FLAG (ARG_CPU->am_acc_flag)


#define mem_read(addr)          cpu_mem_read(ARG_CPU, addr)
#define mem_write(addr, val)    cpu_mem_write(ARG_CPU, addr, val)
// ((high 8 bit) << 8) | (low 8 bit)
#define mem_read16(addr)        ((mem_read(addr + 1) << 8) | mem_read(addr))

#define mem_read_pc()           mem_read(PC++)

// Read/Write operand address of instruction
#define mem_read_op_addr()      mem_read(ARG_OP_ADDR)
#define mem_write_op_addr(val)  mem_write(ARG_OP_ADDR, val)

#define mem_push_stack(val)     mem_write((addr_t)(SP-- + STACK_BASE), val)
#define mem_pop_stack()         mem_read((addr_t)(++SP + STACK_BASE))

FORCE_INLINE void fn_mem_push_stack16(DECL_ARG_CPU, uint16_t val) {
    mem_push_stack((uint8_t) (val >> 8));
    mem_push_stack((uint8_t) val);
}

FORCE_INLINE uint16_t fn_mem_pop_stack16(DECL_ARG_CPU) {
    uint16_t low = mem_pop_stack();
    uint16_t high = mem_pop_stack();
    return (high << 8) | low;
}

#define mem_push_stack16(val)   fn_mem_push_stack16(ARG_CPU, val)
#define mem_pop_stack16()       fn_mem_pop_stack16(ARG_CPU)

/*
 * Memory max offset: 0xFFFF
 * Memory page size : 0x100  -> 2^8
 *
 * Address: 0x 12 34
 *             |  |-- Low  8 bit: In-Page offset
 *             |----- High 8 bit: Page number
 */
#define is_same_page(addr1, addr2) (((addr1) >> 8) == ((addr2) >> 8))

#pragma mark Addressing mode

/**
 * Implicit \n
 * \n
 * For many 6502 instructions the source and destination of the information to be manipulated is implied \n
 * directly by the function of the instruction itself and no further operand needs to be specified. \n
 * Operations like 'Clear Carry Flag' (CLC) and 'Return from Subroutine' (RTS) are implicit. \n
 */
DECL_FUN_AM(IMP) {
    return 0;
}

/**
 * Accumulator \n
 * \n
 * Some instructions have an option to operate directly upon the accumulator. \n
 * The programmer specifies this by using a special operand value, 'A'. For example: \n
 * \n
 * LSR A           ;Logical shift right one bit \n
 * ROR A           ;Rotate right one bit
 */
DECL_FUN_AM(ACC) {
    AM_ACC_FLAG = true;
    return 0;
}

/**
 * Immediate \n
 * \n
 * Immediate addressing allows the programmer to directly specify an 8 bit constant within the instruction. \n
 * It is indicated by a '#' symbol followed by an numeric expression. For example: \n
 * \n
 * LDA #10
 */
DECL_FUN_AM(IMM) {
    return PC++;
}

/*
 * 零页寻址
 * 地址的高字节为固定 0，零页寻址的指令只需要 1 个字节的操作数，所以使用零页寻址的指令更省空间，执行更快。
 * 但是这限制了它只能寻址内存空间的前 256 个字节（$0000 - $00FF）
 *
 * LDA   $12    ;将 $0012 地址处的值加载到 A
 */
DECL_FUN_AM(ZP) {
    addr_t zp_addr = mem_read_pc();
    return zp_addr;
}

/**
 * Zero Page,X \n
 * Zero Page,Y \n
 * \n
 * The address to be accessed by an instruction using indexed zero page addressing is calculated \n
 * by taking the 8 bit zero page address from the instruction and adding the current value of the X register to it
 */
DECL_FUN_AM(ZPX) {
    // The address calculation wraps around if the sum of the base address and the register exceed $FF
    // Using 8 bit save result
    addr_t zpx_addr = mem_read_pc() + X;
    return zpx_addr;
}

DECL_FUN_AM(ZPY) {
    // Using 8 bit save result
    addr_t zpy_addr = mem_read_pc() + Y;
    return zpy_addr;
}

/**
 * Indirect \n
 * \n
 * JMP is the only 6502 instruction to support indirection. \n
 * The instruction contains a 16 bit address which identifies the location of the least significant \n
 * byte of another 16 bit memory address which is the real target of the instruction.
 */
DECL_FUN_AM(IND) {
    addr_t a_addr = mem_read16(PC);
    PC += 2;

    addr_t addr;
    if ((a_addr & 0xFF) == 0xFF) {
        // page boundary
        addr = (mem_read(a_addr & 0xFF00) << 8) | mem_read(a_addr);
    } else {
        addr = mem_read16(a_addr);
    }
    return addr;
}

/**
 * Indirect,X \n
 * \n
 * Indexed indirect addressing is normally used in conjunction with a table of address held on zero page. \n
 * The address of the table is taken from the instruction and the X register added to it (with zero page wrap around) \n
 * to give the location of the least significant byte of the target address. \n
 * \n
 * LDA ($40,X)     ;Load a byte indirectly from memory
 */
DECL_FUN_AM(IZX) {
    addr_t zp_addr = mem_read_pc();
    addr_t addr_x = zp_addr + X;
    return mem_read16(addr_x);
}

/**
 * Indirect,Y \n
 * \n
 * Indirect indirect addressing is the most common indirection mode used on the 6502. \n
 * In instruction contains the zero page location of the least significant byte of 16 bit address. \n
 * The Y register is dynamically added to this value to generated the actual target address for operation. \n
 * \n
 * LDA ($40),Y     ;Load a byte indirectly from memory
 */
DECL_FUN_AM(IZY) {
    addr_t zp_addr = mem_read_pc();
    addr_t addr = mem_read16(zp_addr);
    addr_t addr_y = addr + Y;
    if (!is_same_page(addr, addr_y)) {
        ++CYCLES;
    }
    return addr_y;
}

/**
 * Absolute \n
 * \n
 * Instructions using absolute addressing contain a full 16 bit address to identify the target location.
 */
DECL_FUN_AM(ABS) {
    addr_t addr = mem_read16(PC);
    PC += 2;
    return addr;
}

/**
 * Absolute,X \n
 * Absolute,Y \n
 * \n
 * The address to be accessed by an instruction using X/Y register indexed absolute addressing is computed
 * by taking the 16 bit address from the instruction and added the contents of the X/Y register.
 */
DECL_FUN_AM(ABX) {
    addr_t addr = mem_read16(PC);
    PC += 2;
    addr_t addr_x = addr + X;
    if (!is_same_page(addr, addr_x)) {
        ++CYCLES;
    }
    return addr_x;
}

DECL_FUN_AM(ABY) {
    addr_t addr = mem_read16(PC);
    PC += 2;
    addr_t addr_y = addr + Y;
    if (!is_same_page(addr, addr_y)) {
        ++CYCLES;
    }
    return addr_y;
}

/**
 * Relative \n
 * \n
 * Relative addressing mode is used by branch instructions (e.g. BEQ, BNE, etc.) which contain a signed 8 bit
 * relative offset (e.g. -128 to +127) which is added to program counter if the condition is true.
 * As the program counter itself is incremented during instruction execution by two the effective address
 * range for the target instruction must be with -126 to +129 bytes of the branch.
 *
 * BEQ   $12
 */
DECL_FUN_AM(REL) {
    int8_t offset = mem_read_pc();
    // Branch addr
    return PC + offset;
}

/**
 * Implements 6502 opcodes \n
 * \n
 * Docs: \n
 * https://www.nesdev.org/obelisk-6502-guide/instructions.html \n
 * https://www.nesdev.org/obelisk-6502-guide/reference.html
 */
#pragma mark Instruction Set - Load/Store Operations

/**
 * LDA - Load Accumulator \n
 * LDX - Load X Register \n
 * LDY - Load Y Register \n
 * \n
 * Loads a byte of memory into the A/X/Y, setting the zero and negative flags as appropriate. \n
 * \n
 * Zero Flag:	    Set if A/X/Y = 0 \n
 * Negative Flag:	Set if bit 7 of A/X/Y is set
 */
DECL_FUN_OP(LDA) {
    A = mem_read_op_addr();
    set_zn_flag(A);
}

DECL_FUN_OP(LDX) {
    X = mem_read_op_addr();
    set_zn_flag(X);
}

DECL_FUN_OP(LDY) {
    Y = mem_read_op_addr();
    set_zn_flag(Y);
}

/**
 * STA - Store Accumulator \n
 * STX - Store X Register \n
 * STY - Store Y Register \n
 * \n
 * Stores the contents of the A/X/Y into memory.
 */
DECL_FUN_OP(STA) {
    mem_write(ARG_OP_ADDR, A);
}

DECL_FUN_OP(STX) {
    mem_write(ARG_OP_ADDR, X);
}

DECL_FUN_OP(STY) {
    mem_write(ARG_OP_ADDR, Y);
}


#pragma mark Instruction Set - Register Transfers

/**
 * TAX - Transfer Accumulator to X      X = A \n
 * TAY - Transfer Accumulator to Y      Y = A \n
 * TXA - Transfer X to Accumulator      A = X \n
 * TYA - Transfer Y to Accumulator      A = Y \n
 * \n
 * Copies the current contents of the A/X/Y into the A/X/Y register and sets the zero and negative flags as appropriate. \n
 * \n
 * Set Z,N Flags
 */
DECL_FUN_OP(TAX) {
    X = A;
    set_zn_flag(X);
}

DECL_FUN_OP(TAY) {
    Y = A;
    set_zn_flag(Y);
}

DECL_FUN_OP(TXA) {
    A = X;
    set_zn_flag(A);
}

DECL_FUN_OP(TYA) {
    A = Y;
    set_zn_flag(A);
}

#pragma mark Instruction Set - Stack Operations

/**
 * TSX - Transfer Stack Pointer to X    X = S \n
 * \n
 * Copies the current contents of the stack register into the X register and sets the zero and negative flags as appropriate. \n
 * \n
 * Set Z,N Flags
 */
DECL_FUN_OP(TSX) {
    X = SP;
    set_zn_flag(X);
}

/**
 * TXS - Transfer X to Stack Pointer    S = X \n
 * \n
 * Copies the current contents of the X register into the stack register.
 */
DECL_FUN_OP(TXS) {
    SP = X;
}

/**
 * PHA - Push Accumulator \n
 * \n
 * Pushes a copy of the accumulator on to the stack.
 */
DECL_FUN_OP(PHA) {
    mem_push_stack(A);
}

/**
 * PHP - Push Processor Status \n
 * \n
 * Pushes a copy of the status flags on to the stack.
 */
DECL_FUN_OP(PHP) {
    mem_push_stack(P | BREAK_COMMAND);
}

/**
 * PLA - Pull Accumulator \n
 * \n
 * Pulls an 8 bit value from the stack and into the accumulator. The zero and negative flags are set as appropriate. \n
 * \n
 * Set Z,N Flags
 */
DECL_FUN_OP(PLA) {
    A = mem_pop_stack();
    set_zn_flag(A);
}

/**
 * PLP - Pull Processor Status \n
 * \n
 * Pulls an 8 bit value from the stack and into the processor flags. The flags will take on new states as determined by the value pulled.
 */
DECL_FUN_OP(PLP) {
    P = mem_pop_stack();
}

#pragma mark Instruction Set - Logical

/**
 * AND - Logical AND \n
 * \n
 * A logical AND is performed, bit by bit, on the accumulator contents using the contents of a byte of memory.
 * \n
 * set Z, N Flags
 */
DECL_FUN_OP(AND) {
    A = A & mem_read_op_addr();
    set_zn_flag(A);
}

/**
 * EOR - Exclusive OR \n
 * \n
 * An exclusive OR is performed, bit by bit, on the accumulator contents using the contents of a byte of memory.
 * \n
 * set Z, N Flags
 */
DECL_FUN_OP(EOR) {
    A = A ^ mem_read_op_addr();
    set_zn_flag(A);
}

/**
 * ORA - Logical Inclusive OR \n
 * \n
 * An inclusive OR is performed, bit by bit, on the accumulator contents using the contents of a byte of memory.
 * \n
 * set Z, N Flags
 */
DECL_FUN_OP(ORA) {
    A = A | mem_read_op_addr();
    set_zn_flag(A);
}

/**
 * BIT - Bit Test \n
 * \n
 * This instructions is used to test if one or more bits are set in a target memory location. \n
 * The mask pattern in A is ANDed with the value in memory to set or clear the zero flag, but the result is not kept. \n
 * Bits 7 and 6 of the value from memory are copied into the N and V flags. \n
 * \n
 * Zero Flag:	    Set if the result if the AND is zero \n
 * Overflow Flag:	Set to bit 6 of the memory value \n
 * Negative Flag:	Set to bit 7 of the memory value
 */
DECL_FUN_OP(BIT) {
    uint8_t operand = mem_read_op_addr();
    uint8_t result = A & operand;
    set_flag(ZERO_FLAG, result == 0);
    set_flag(OVERFLOW_FLAG, (operand >> 6) & 0b1);
    set_flag(NEGATIVE_FLAG, (operand >> 7) & 0b1);
}

#pragma mark Instruction Set - Arithmetic

/**
 * ADC - Add with Carry \n
 * \n
 * This instruction adds the contents of a memory location to the accumulator together with the carry bit. \n
 * If overflow occurs the carry bit is set, this enables multiple byte addition to be performed. \n
 * \n
 * Carry Flag:   	Set if overflow in bit 7 \n
 * Overflow Flag:	Set if sign bit is incorrect \n
 * Set Z,N Flags
 */

// ADC/SBC implements
FORCE_INLINE void adc_impl(cpu_t* ARG_CPU, uint8_t operand) {
    uint16_t sum = A + operand + get_flag(CARRY_FLAG);
    set_flag(OVERFLOW_FLAG, (~(A ^ operand) & (A ^ sum)) & 0x80);

    A = (uint8_t) sum;
    set_flag(CARRY_FLAG, sum > 0xFF);
    set_zn_flag(A);
}

DECL_FUN_OP(ADC) {
    uint8_t operand = mem_read_op_addr();
    adc_impl(ARG_CPU, operand);
}

/**
 * SBC - Subtract with Carry \n
 * \n
 * See also: ADC
 */
DECL_FUN_OP(SBC) {
    uint8_t operand = ~mem_read_op_addr();
    adc_impl(ARG_CPU, operand);
}


/**
 * CMP - Compare \n
 * CPX - Compare X Register \n
 * CPX - Compare Y Register \n
 * \n
 * This instruction compares the contents of the A/X/Y with another memory held value and sets the zero and carry flags as appropriate. \n
 * \n
 * Carry Flag:      Set if A/X/Y >= M \n
 * Set Z, N Flags
 */
FORCE_INLINE void cmp_impl(DECL_ARG_CPU, DECL_ARG_ADDR, uint8_t reg_val) {
    uint8_t operand = mem_read_op_addr();
    uint8_t result = reg_val - operand;
    set_flag(CARRY_FLAG, reg_val >= operand);
    set_zn_flag(result);
}

#define cmp_with_reg(reg_val)  cmp_impl(ARG_CPU, ARG_OP_ADDR, reg_val)

DECL_FUN_OP(CMP) {
    cmp_with_reg(A);
}

DECL_FUN_OP(CPX) {
    cmp_with_reg(X);
}

DECL_FUN_OP(CPY) {
    cmp_with_reg(Y);
}

#pragma mark Instruction Set - Increments & Decrements

/**
 * INC - Increment Memory \n
 * \n
 * Adds one to the value held at a specified memory location setting the zero and negative flags as appropriate \n
 * \n
 * set Z, N Flags
 */
DECL_FUN_OP(INC) {
    uint8_t result = mem_read_op_addr() + 1;
    mem_write_op_addr(result);
    set_zn_flag(result);
}

/**
 * INX - Increment X Register \n
 * INY - Increment Y Register \n
 * \n
 * Adds one to the X/Y register setting the zero and negative flags as appropriate \n
 * \n
 * set Z, N Flags
 */
DECL_FUN_OP(INX) {
    X += 1;
    set_zn_flag(X);
}

DECL_FUN_OP(INY) {
    Y += 1;
    set_zn_flag(Y);
}

/**
 * DEX - Decrement X Register \n
 * DEY - Decrement Y Register \n
 * \n
 * Subtracts one from the X/Y register setting the zero and negative flags as appropriate \n
 * \n
 * set Z, N Flags
 */
DECL_FUN_OP(DEX) {
    X -= 1;
    set_zn_flag(X);
}

DECL_FUN_OP(DEY) {
    Y -= 1;
    set_zn_flag(Y);
}

/**
 * DEC - Decrement Memory \n
 * \n
 * Subtracts one from the value held at a specified memory location setting the zero and negative flags as appropriate \n
 * \n
 * set Z, N Flags
 */
DECL_FUN_OP(DEC) {
    uint8_t result = mem_read_op_addr() - 1;
    mem_write_op_addr(result);
    set_zn_flag(result);
}

#pragma mark Instruction Set - Shifts

/**
 * ASL - Arithmetic Shift Left \n
 * \n
 * This operation shifts all the bits of the accumulator or memory contents one bit left. \n
 * Bit 0 is set to 0 and bit 7 is placed in the carry flag. \n
 * The effect of this operation is to multiply the memory contents by 2 (ignoring 2's complement considerations), \n
 * setting the carry if the result will not fit in 8 bits \n
 * \n
 * Carry Flag:      Set to contents of old bit 7 \n
 * Set Z,N Flags
 */
FORCE_INLINE uint8_t asl_impl(DECL_ARG_CPU, uint8_t val) {
    uint8_t ret = val << 1;
    // old bit_7 -> CARRY
    set_flag(CARRY_FLAG, val >> 7);
    set_zn_flag(ret);
    return ret;
}

DECL_FUN_OP(ASL) {
    if (AM_ACC_FLAG) {
        A = asl_impl(ARG_CPU, A);
    } else {
        mem_write_op_addr(asl_impl(ARG_CPU, mem_read_op_addr()));
    }
    AM_ACC_FLAG = false;
}

/**
 * LSR - Logical Shift Right \n
 * \n
 * Each of the bits in A or M is shift one place to the right. \n
 * The bit that was in bit 0 is shifted into the carry flag. Bit 7 is set to zero. \n
 * \n
 * Carry Flag	Set to contents of old bit 0 \n
 * Set Z,N Flags
 */
FORCE_INLINE uint8_t lsr_impl(DECL_ARG_CPU, uint8_t val) {
    uint8_t ret = val >> 1;
    // old bit_0 -> CARRY
    set_flag(CARRY_FLAG, val & 0b1);
    set_zn_flag(ret);
    return ret;
}

DECL_FUN_OP(LSR) {
    if (AM_ACC_FLAG) {
        A = lsr_impl(ARG_CPU, A);
    } else {
        mem_write_op_addr(lsr_impl(ARG_CPU, mem_read_op_addr()));
    }
    AM_ACC_FLAG = false;
}

/**
 * ROL - Rotate Left \n
 * Move each of the bits in either A or M one place to the left. \n
 * Bit 0 is filled with the current value of the carry flag whilst the old bit 7 becomes the new carry flag value.
 */
FORCE_INLINE uint8_t rol_impl(DECL_ARG_CPU, uint8_t val) {
    // CARRY -> bit_0
    uint8_t result = (val << 1) | get_flag(CARRY_FLAG);
    // old bit_7 -> CARRY
    set_flag(CARRY_FLAG, val >> 7);
    set_zn_flag(result);
    return result;
}

DECL_FUN_OP(ROL) {
    if (AM_ACC_FLAG) {
        A = rol_impl(ARG_CPU, A);
    } else {
        mem_write_op_addr(rol_impl(ARG_CPU, mem_read_op_addr()));
    }
    AM_ACC_FLAG = false;
}

/**
 * ROR - Rotate Right \n
 * Move each of the bits in either A or M one place to the right. \n
 * Bit 7 is filled with the current value of the carry flag whilst the old bit 0 becomes the new carry flag value.
 */
FORCE_INLINE uint8_t ror_impl(DECL_ARG_CPU, uint8_t val) {
    // CARRY -> bit_7
    uint8_t result = (val >> 1) | (get_flag(CARRY_FLAG) << 7);
    // old bit_0 -> CARRY
    set_flag(CARRY_FLAG, val & 0b1);
    set_zn_flag(result);
    return result;
}

DECL_FUN_OP(ROR) {
    if (AM_ACC_FLAG) {
        A = ror_impl(ARG_CPU, A);
    } else {
        uint8_t operand = mem_read_op_addr();
        operand = ror_impl(ARG_CPU, operand);
        mem_write_op_addr(operand);
    }
    AM_ACC_FLAG = false;
}

#pragma mark Instruction Set - Jumps & Calls
/**
 * JMP - Jump \n
 * \n
 * Sets the program counter to the address specified by the operand.
 */
DECL_FUN_OP(JMP) {
    PC = ARG_OP_ADDR;
}

/*
 * JSR - Jump to Subroutine
 *
 * 将返回地址（PC - 1）压栈，然后将 PC 设置为目标内存地址。
 */
DECL_FUN_OP(JSR) {
    addr_t return_addr = PC - 1;
    mem_push_stack16(return_addr);
    PC = ARG_OP_ADDR;
}

/**
 * RTS - Return from Subroutine \n
 * \n
 * The RTS instruction is used at the end of a subroutine to return to the calling routine. \n
 * It pulls the program counter (minus one) from the stack.
 */
DECL_FUN_OP(RTS) {
    PC = mem_pop_stack16();
    ++PC;
}


#pragma mark Instruction Set - Braches

/**
 * BCC - Branch if Carry Clear \n
 * BCS - Branch if Carry Set \n
 * \n
 * BEQ - Branch if Equal \n
 * BNE - Branch if Not Equal \n
 * \n
 * BMI - Branch if Minus \n
 * BPL - Branch if Positive \n
 * \n
 * BVC - Branch if Overflow Clear \n
 * BVS - Branch if Overflow Set \n
 * \n
 * If the C/Z/N/V Flag is clear/set then add the relative displacement to the program counter to cause a branch to a new location.
 */
FORCE_INLINE void branch_impl(DECL_ARG_CPU, DECL_ARG_ADDR, bool cond) {
    if (cond) {
        ++CYCLES;
        // ARG_OP_ADDR -> Branch addr
        if (!is_same_page(PC, ARG_OP_ADDR)) {
            ++CYCLES;
        }
        PC = ARG_OP_ADDR;
    }
}

#define branch_if_set(flag) branch_impl(ARG_CPU, ARG_OP_ADDR, get_flag(flag) == CPU_FLAG_SET)
#define branch_if_clr(flag) branch_impl(ARG_CPU, ARG_OP_ADDR, get_flag(flag) == CPU_FLAG_CLR)

DECL_FUN_OP(BCC) {
    branch_if_clr(CARRY_FLAG);
}

DECL_FUN_OP(BCS) {
    branch_if_set(CARRY_FLAG);
}

DECL_FUN_OP(BEQ) {
    branch_if_set(ZERO_FLAG);
}

DECL_FUN_OP(BNE) {
    branch_if_clr(ZERO_FLAG);
}

DECL_FUN_OP(BMI) {
    branch_if_set(NEGATIVE_FLAG);
}

DECL_FUN_OP(BPL) {
    branch_if_clr(NEGATIVE_FLAG);
}

DECL_FUN_OP(BVC) {
    branch_if_clr(OVERFLOW_FLAG);
}

DECL_FUN_OP(BVS) {
    branch_if_set(OVERFLOW_FLAG);
}


#pragma mark Instruction Set - Status Flag Changes

/**
 * CLC - Clear Carry Flag \n
 * CLD - Clear Decimal Mode \n
 * CLI - Clear Interrupt Disable \n
 * CLV - Clear Overflow Flag \n
 * SEC - Set Carry Flag \n
 * SED - Set Decimal Flag \n
 * SEI - Set Interrupt Disable  \n
 */
DECL_FUN_OP(CLC) {
    set_flag(CARRY_FLAG, false);
}

DECL_FUN_OP(CLD) {
    set_flag(DECIMAL_MODE, false);
}

DECL_FUN_OP(CLI) {
    set_flag(INTERRUPT_DISABLE, false);
}

DECL_FUN_OP(CLV) {
    set_flag(OVERFLOW_FLAG, false);
}

DECL_FUN_OP(SEC) {
    set_flag(CARRY_FLAG, true);
}

DECL_FUN_OP(SED) {
    set_flag(DECIMAL_MODE, true);
}

DECL_FUN_OP(SEI) {
    set_flag(INTERRUPT_DISABLE, true);
}

#pragma mark Instruction Set - System Functions
/**
 * BRK - Force Interrupt \n
 * \n
 * The BRK instruction forces the generation of an interrupt request. \n
 * The program counter and processor status are pushed on the stack \n
 * then the IRQ interrupt vector at $FFFE/F is loaded into the PC and the break flag in the status set to one.
 */
DECL_FUN_OP(BRK) {
    set_flag(BREAK_COMMAND, true);

}

/**
 * NOP - No Operation \n
 * \n
 * The NOP instruction causes no changes to the processor other than the normal \n
 * incrementing of the program counter to the next instruction.
 */
DECL_FUN_OP(NOP) {
}

/**
 * RTI - Return from Interrupt \n
 * \n
 * The RTI instruction is used at the end of an interrupt processing routine. \n
 * It pulls the processor flags from the stack followed by the program counter.
 */
DECL_FUN_OP(RTI) {
    P = mem_pop_stack();
    PC = mem_pop_stack16();
}

typedef struct {
    void (* op_func)(cpu_t*, addr_t);

    addr_t (* am_func)(cpu_t*);

    uint8_t cycles;
} CpuOperation;


CpuOperation op_table[] = {
        {OP_BRK, AM_IMP, 1}, // $00
        {OP_ORA, AM_IZX, 6}, //$01
        {NULL, NULL}, // $02
        {NULL, NULL}, // $03
        {NULL, NULL}, // $04
        {OP_ORA, AM_ZP,  3}, // $05
        {OP_ASL, AM_ZP,  5}, // $06
        {NULL, NULL}, // $07
        {OP_PHP, AM_IMP, 3}, // $08
        {OP_ORA, AM_IMM, 2}, // $09
        {OP_ASL, AM_ACC, 2}, // $0A
        {NULL, NULL}, // $0B
        {NULL, NULL}, // $0C
        {OP_ORA, AM_ABS, 4}, // $0D
        {OP_ASL, AM_ABS, 6}, // $0E
        {NULL, NULL}, // $0F
        {OP_BPL, AM_REL, 2},// $10
        {OP_ORA, AM_IZY, 5}, // $11
        {NULL, NULL}, // $12
        {NULL, NULL}, // $13
        {NULL, NULL}, // $14
        {OP_ORA, AM_ZPX, 4}, // $15
        {OP_ASL, AM_ZPX, 6}, // $16
        {NULL, NULL}, // $17
        {OP_CLC, AM_IMP, 2},// $18
        {OP_ORA, AM_ABY, 4}, // $19
        {NULL, NULL}, // $1A
        {NULL, NULL}, // $1B
        {NULL, NULL}, // $1C
        {OP_ORA, AM_ABX, 4}, // $1D
        {OP_ASL, AM_ABX, 7}, // $1E
        {NULL, NULL}, // $1F
        {OP_JSR, AM_ABS, 6}, // $20
        {OP_AND, AM_IZX, 6}, // $21
        {NULL, NULL},  // $22
        {NULL, NULL}, // $23
        {OP_BIT, AM_ZP,  3},// $24
        {OP_AND, AM_ZP,  3}, // $25
        {OP_ROL, AM_ZP,  5}, // $26
        {NULL, NULL}, // $27
        {OP_PLP, AM_IMP, 4}, // $28
        {OP_AND, AM_IMM, 2}, // $29
        {OP_ROL, AM_ACC, 2}, // $2A
        {NULL, NULL}, // $2B
        {OP_BIT, AM_ABS, 4},// $2C
        {OP_AND, AM_ABS, 4}, // $2D
        {OP_ROL, AM_ABS, 6}, // $2E
        {NULL, NULL}, // $2F
        {OP_BMI, AM_REL, 2},// $30
        {OP_AND, AM_IZY, 5}, // $31
        {NULL, NULL}, // $32
        {NULL, NULL}, // $33
        {NULL, NULL}, // $34
        {OP_AND, AM_ZPX, 4}, // $35
        {OP_ROL, AM_ZPX, 6}, // $36
        {NULL, NULL}, // $37
        {OP_SEC, AM_IMP, 2}, // $38
        {OP_AND, AM_ABY, 4}, // $39
        {NULL, NULL}, // $3A
        {NULL, NULL}, // $3B
        {NULL, NULL}, // $3C
        {OP_AND, AM_ABX, 4}, // $3D
        {OP_ROL, AM_ABX, 7}, // $3E
        {NULL, NULL}, // $3F
        {OP_RTI, AM_IMP, 6}, // $40
        {OP_EOR, AM_IZX, 6}, // $41
        {NULL, NULL}, // $42
        {NULL, NULL}, // $43
        {NULL, NULL}, // $44
        {OP_EOR, AM_ZP,  3}, // $45
        {OP_LSR, AM_ZP,  5}, // $46
        {NULL, NULL}, // $47
        {OP_PHA, AM_IMP, 3}, // $48
        {OP_EOR, AM_IMM, 2}, // $49
        {OP_LSR, AM_ACC, 2}, // $4A
        {NULL, NULL}, // $4B
        {OP_JMP, AM_ABS, 3}, // $4C
        {OP_EOR, AM_ABS, 4}, // $4D
        {OP_LSR, AM_ABS, 6}, // $4E
        {NULL, NULL}, // $4F
        {OP_BVC, AM_REL, 2}, // $50
        {OP_EOR, AM_IZY, 5}, // $51
        {NULL, NULL}, // $52
        {NULL, NULL}, // $53
        {NULL, NULL}, // $54
        {OP_EOR, AM_ZPX, 4}, // $55
        {OP_LSR, AM_ZPX, 6}, // $56
        {NULL, NULL}, // $57
        {OP_CLI, AM_IMP, 2}, // $58
        {OP_EOR, AM_ABY, 4}, // $59
        {NULL, NULL}, // $5A
        {NULL, NULL}, // $5B
        {NULL, NULL}, // $5C
        {OP_EOR, AM_ABX, 5}, // $5D
        {OP_LSR, AM_ABX, 7}, // $5E
        {NULL, NULL}, // $5F
        {OP_RTS, AM_IMP, 6}, // $60
        {OP_ADC, AM_IZX, 6}, // $61
        {NULL, NULL}, // $62
        {NULL, NULL}, // $63
        {NULL, NULL}, // $64
        {OP_ADC, AM_ZP,  3}, // $65
        {OP_ROR, AM_ZP,  5}, // $66
        {NULL, NULL}, // $67
        {OP_PLA, AM_IMP, 4},  // $68
        {OP_ADC, AM_IMM, 2}, // $69
        {OP_ROR, AM_ACC, 2}, // $6A
        {NULL, NULL}, // $6B
        {OP_JMP, AM_IND, 5}, // $6C
        {OP_ADC, AM_ABS, 4}, // $6D
        {OP_ROR, AM_ABS, 6}, // $6E
        {NULL, NULL}, // $6F
        {OP_BVS, AM_REL, 2}, // $70
        {OP_ADC, AM_IZY, 5}, // $71
        {NULL, NULL}, // $72
        {NULL, NULL}, // $73
        {NULL, NULL}, // $74
        {OP_ADC, AM_ZPX, 4}, // $75
        {OP_ROR, AM_ZPX, 6}, // $76
        {NULL, NULL}, // $77
        {OP_SEI, AM_IMP, 2},   // $78
        {OP_ADC, AM_ABY, 4}, // $79
        {NULL, NULL}, // $7A
        {NULL, NULL}, // $7B
        {NULL, NULL}, // $7C
        {OP_ADC, AM_ABX, 4},// $7D
        {OP_ROR, AM_ABX, 7}, // $7E
        {NULL, NULL}, // $7F
        {NULL, NULL}, // $80
        {OP_STA, AM_IZX, 6}, // $81
        {NULL, NULL}, // $82
        {NULL, NULL}, // $83
        {OP_STY, AM_ZP,  3}, // $84
        {OP_STA, AM_ZP,  3}, // $85
        {OP_STX, AM_ZP,  3}, // $86
        {NULL, NULL}, // $87
        {OP_DEY, AM_IMP, 2}, // $88
        {NULL, NULL}, // $89
        {OP_TXA, AM_IMP, 2}, // $8A
        {NULL, NULL}, // $8B
        {OP_STY, AM_ABS, 4}, // $8C
        {OP_STA, AM_ABS, 4}, // $8D
        {OP_STX, AM_ABS, 4}, // $8E
        {NULL, NULL}, // $8F
        {OP_BCC, AM_REL, 2}, // $90
        {OP_STA, AM_IZY, 6}, // $91
        {NULL, NULL}, // $92
        {NULL, NULL}, // $93
        {OP_STY, AM_ZPX, 4}, // $94
        {OP_STA, AM_ZPX, 4}, // $95
        {OP_STX, AM_ZPY, 4}, // $96
        {NULL, NULL}, // $97
        {OP_TYA, AM_IMP}, // $98
        {OP_STA, AM_ABY, 5}, // $99
        {OP_TXS, AM_IMP, 2}, // $9A
        {NULL, NULL}, // $9B
        {NULL, NULL}, // $9C
        {OP_STA, AM_ABX, 5}, // $9D
        {NULL, NULL}, // $9E
        {NULL, NULL}, // $9F
        {OP_LDY, AM_IMM, 2}, // $A0
        {OP_LDA, AM_IZX, 6}, // $A1
        {OP_LDX, AM_IMM, 2}, // $A2
        {NULL, NULL}, // $A3
        {OP_LDY, AM_ZP,  3}, // $A4
        {OP_LDA, AM_ZP,  3}, // $A5
        {OP_LDX, AM_ZP,  3}, // $A6
        {NULL, NULL}, // $A7
        {OP_TAY, AM_IMP, 2}, // $A8
        {OP_LDA, AM_IMM, 2}, // $A9
        {OP_TAX, AM_IMP, 2},  // $AA
        {NULL, NULL}, // $AB
        {OP_LDY, AM_ABS, 4}, // $AC
        {OP_LDA, AM_ABS, 4}, // $AD
        {OP_LDX, AM_ABS, 4}, // $AE
        {NULL, NULL}, // $AF
        {OP_BCS, AM_REL, 2}, // $B0
        {OP_LDA, AM_IZY, 5}, // $B1
        {NULL, NULL}, // $B2
        {NULL, NULL}, // $B3
        {OP_LDY, AM_ZPX, 4}, // $B4
        {OP_LDA, AM_ZPX, 4}, // $B5
        {OP_LDX, AM_ZPY, 4}, // $B6
        {NULL, NULL}, // $B7
        {OP_CLV, AM_IMP, 2}, // $B8
        {OP_LDA, AM_ABY, 4}, // $B9
        {OP_TSX, AM_IMP, 2},  // $BA
        {NULL, NULL}, // $BB
        {OP_LDY, AM_ABX, 4}, // $BC
        {OP_LDA, AM_ABX, 4}, // $BD
        {OP_LDX, AM_ABY, 4}, // $BE
        {NULL, NULL}, // $BF
        {OP_CPY, AM_IMM, 2}, // $C0
        {OP_CMP, AM_IZX, 6}, // $C1
        {NULL, NULL}, // $C2
        {NULL, NULL}, // $C3
        {OP_CPY, AM_ZP,  3}, // $C4
        {OP_CMP, AM_ZP,  3}, // $C5
        {OP_DEC, AM_ZP,  5}, // $C6
        {NULL, NULL}, // $C7
        {OP_INY, AM_IMP, 2}, // $C8
        {OP_CMP, AM_IMM, 2}, // $C9
        {OP_DEX, AM_IMP, 2}, // $CA
        {NULL, NULL}, // $CB
        {OP_CPY, AM_ABS, 4}, // $CC
        {OP_CMP, AM_ABS, 4}, // $CD
        {OP_DEC, AM_ABS, 6}, // $CE
        {NULL, NULL}, // $CF
        {OP_BNE, AM_REL, 2}, // $D0
        {OP_CMP, AM_IZY, 5}, // $D1
        {NULL, NULL}, // $D2
        {NULL, NULL}, // $D3
        {NULL, NULL}, // $D4
        {OP_CMP, AM_ZPX, 4}, // $D5
        {OP_DEC, AM_ZPX, 6}, // $D6
        {NULL, NULL}, // $D7
        {OP_CLD, AM_IMP, 2}, // $D8
        {OP_CMP, AM_ABY, 4}, // $D9
        {NULL, NULL}, // $DA
        {NULL, NULL}, // $DB
        {NULL, NULL}, // $DC
        {OP_CMP, AM_ABX, 4}, // $DD
        {OP_DEC, AM_ABX, 7}, // $DE
        {NULL, NULL}, // $DF
        {OP_CPX, AM_IMM, 2}, // $E0
        {OP_SBC, AM_IZX, 6}, // $E1
        {NULL, NULL}, // $E2
        {NULL, NULL}, // $E3
        {OP_CPX, AM_ZP,  3}, // $E4
        {OP_SBC, AM_ZP,  3}, // $E5
        {OP_INC, AM_ZP,  5}, // $E6
        {NULL, NULL}, // $E7
        {OP_INX, AM_IMP, 2}, // $E8
        {OP_SBC, AM_IMM, 2}, // $E9
        {OP_NOP, AM_IMP, 1},  // $EA
        {NULL, NULL}, // $EB
        {OP_CPX, AM_ABS, 4}, // $EC
        {OP_SBC, AM_ABS, 4}, // $ED
        {OP_INC, AM_ABS, 6}, // $EE
        {NULL, NULL}, // $EF
        {OP_BEQ, AM_REL, 2},// $F0
        {OP_SBC, AM_IZY, 5}, // $F1
        {NULL, NULL}, // $F2
        {NULL, NULL}, // $F3
        {NULL, NULL}, // $F4
        {OP_SBC, AM_ZPX, 4}, // $F5
        {OP_INC, AM_ZPX, 6}, // $F6
        {NULL, NULL}, // $F7
        {OP_SED, AM_IMP, 2},  // $F8
        {OP_SBC, AM_ABY, 4}, // $F9
        {NULL, NULL}, // $FA
        {NULL, NULL}, // $FB
        {NULL, NULL}, // $FC
        {OP_SBC, AM_ABX, 4}, // $FD
        {OP_INC, AM_ABX, 7}, // $FE
        {NULL, NULL}, // $FF
};


static void cpu_interrupt_nmi(cpu_t* cpu) {
    mem_push_stack16(PC);
    set_flag(BREAK_COMMAND, BIT_FLAG_CLR);
    set_flag(INTERRUPT_DISABLE, BIT_FLAG_SET);
    mem_push_stack(P);
    PC = mem_read16(VECTOR_NIM);
    cpu->cycles = 8;
}

static void cpu_reset(cpu_t* cpu) {
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFD;
    PC = mem_read16(VECTOR_RESET);
    CYCLES = 8;
}

#include <stdio.h>

uint64_t cycle_count = 0;

void cpu_cycle(cpu_t* cpu) {
    if (cpu->cycles == 0) {

        if (cpu->nmi) {
            cpu->nmi = false;
            cpu_interrupt_nmi(cpu);
        }

        uint8_t opcode = mem_read_pc();
        printf("opcode: %d, pc: %#x, cycle_count: %lld\n", opcode, PC, cycle_count);
        CpuOperation operation = op_table[opcode];
        if (operation.am_func) {
            cpu->cycles += operation.cycles;

            addr_t op_addr = operation.am_func(cpu);
            operation.op_func(cpu, op_addr);
        }
    }

    ++cycle_count;
    --cpu->cycles;
}

cpu_t* cpu_create(ppu_t* ppu, mapper_t* mapper) {
    cpu_t* cpu = calloc(1, sizeof(cpu_t));
    cpu->ppu = ppu;
    cpu->mapper = mapper;

    cpu_reset(cpu);
    return cpu;
}