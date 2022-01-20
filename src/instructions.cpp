#include "instructions.hpp"
#include <iostream>
#include <fstream>
#define DEBUG 0

int64_t sign_extension(int init_bits, int goal_bits, uint64_t val)
{
    uint64_t x = val;
    uint64_t mask = 0;
    bool msb = (val >> init_bits - 1) & 1;
    if (msb)
    {
        for (int idx = init_bits; idx < goal_bits; idx++)
        {
            mask |= 1;
            mask <<= 1;
        }
        mask <<= init_bits - 1;
        x |= mask;
    }
    return x;
}

// https://stackoverflow.com/questions/25095741/how-can-i-multiply-64-bit-operands-and-get-128-bit-result-portably/58381061#58381061
/* Prevents a partial vectorization from GCC. */
#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
__attribute__((__target__("no-sse")))
#endif
inline uint64_t multu_64_to_128(uint64_t lhs, uint64_t rhs, uint64_t *high) {
        /*
         * GCC and Clang usually provide __uint128_t on 64-bit targets,
         * although Clang also defines it on WASM despite having to use
         * builtins for most purposes - including multiplication.
         */
#if defined(__SIZEOF_INT128__) && !defined(__wasm__)
        __uint128_t product = (__uint128_t)lhs * (__uint128_t)rhs;
        *high = (uint64_t)(product >> 64);
        return (uint64_t)(product & 0xFFFFFFFFFFFFFFFF);

        /* Use the _umul128 intrinsic on MSVC x64 to hint for mulq. */
#elif defined(_MSC_VER) && defined(_M_IX64)
        #   pragma intrinsic(_umul128)
    /* This intentionally has the same signature. */
    return _umul128(lhs, rhs, high);

#else
    /*
     * Fast yet simple grade school multiply that avoids
     * 64-bit carries with the properties of multiplying by 11
     * and takes advantage of UMAAL on ARMv6 to only need 4
     * calculations.
     */

    /* First calculate all of the cross products. */
    uint64_t lo_lo = (lhs & 0xFFFFFFFF) * (rhs & 0xFFFFFFFF);
    uint64_t hi_lo = (lhs >> 32)        * (rhs & 0xFFFFFFFF);
    uint64_t lo_hi = (lhs & 0xFFFFFFFF) * (rhs >> 32);
    uint64_t hi_hi = (lhs >> 32)        * (rhs >> 32);

    /* Now add the products together. These will never overflow. */
    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    uint64_t upper = (hi_lo >> 32) + (cross >> 32)        + hi_hi;

    *high = upper;
    return (cross << 32) | (lo_lo & 0xFFFFFFFF);
#endif /* portable */
}

/* Prevents a partial vectorization from GCC. */
#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
__attribute__((__target__("no-sse")))
#endif
inline uint64_t mult_64_to_128(int64_t lhs, int64_t rhs, uint64_t *high) {
    /*
     * GCC and Clang usually provide __uint128_t on 64-bit targets,
     * although Clang also defines it on WASM despite having to use
     * builtins for most purposes - including multiplication.
     */
#if defined(__SIZEOF_INT128__) && !defined(__wasm__)
    __int128_t product = (__int128_t)lhs * (__int128_t)rhs;
    *high = (int64_t)(product >> 64);
    return (int64_t)(product & 0xFFFFFFFFFFFFFFFF);

    /* Use the _mul128 intrinsic on MSVC x64 to hint for mulq. */
#elif defined(_MSC_VER) && defined(_M_IX64)
    #   pragma intrinsic(_mul128)
    /* This intentionally has the same signature. */
    return _mul128(lhs, rhs, high);

#else
    /*
     * Fast yet simple grade school multiply that avoids
     * 64-bit carries with the properties of multiplying by 11
     * and takes advantage of UMAAL on ARMv6 to only need 4
     * calculations.
     */

    logfatal("This code will be broken for signed multiplies!");

    /* First calculate all of the cross products. */
    uint64_t lo_lo = (lhs & 0xFFFFFFFF) * (rhs & 0xFFFFFFFF);
    uint64_t hi_lo = (lhs >> 32)        * (rhs & 0xFFFFFFFF);
    uint64_t lo_hi = (lhs & 0xFFFFFFFF) * (rhs >> 32);
    uint64_t hi_hi = (lhs >> 32)        * (rhs >> 32);

    /* Now add the products together. These will never overflow. */
    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    uint64_t upper = (hi_lo >> 32) + (cross >> 32)        + hi_hi;

    *high = upper;
    return (cross << 32) | (lo_lo & 0xFFFFFFFF);
#endif /* portable */
}

void mtc0(CPU &cpu, uint32_t opcode)
{
    //std::cout << "finally, mtc0";
    uint8_t rt = (opcode >> 16) & 0b11'1111;
    uint8_t rd = (opcode >> 11) & 0b11'1111;

    cpu.cp0_regs[rd] = cpu.regs[rt];
}

void mtc1(CPU &cpu, uint32_t opcode)
{
}
void mtc2(CPU &cpu, uint32_t opcode)
{
}
void mtc_handler(CPU &cpu, uint32_t opcode)
{
    uint8_t coprocessor = (opcode >> 26) & 0b11;
    {
        switch (coprocessor)
        {
        case 0b00:
            mtc0(cpu, opcode);
            break;
        case 0b01:
            mtc1(cpu, opcode);
        case 0b10:
            mtc2(cpu, opcode);
        default:
            break;
        }
    }
}
void cop_handler(CPU &cpu, uint32_t opcode)
{
    uint8_t sub_opcode = (opcode >> 21) & 0b1'1111;
    switch (sub_opcode)
    {
    case 0b00100:
        mtc_handler(cpu, opcode);
        break;

    default:
        break;
    }
}

void lui(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint16_t test = opcode & 0xFFFF;
    int32_t imm = test << 16;
    if (DEBUG)
    {
        cpu.debug << std::hex << "lui - " << opcode << "\n rt: " << (int)rt << " imm: " << (int)imm << "\n\n";
    }

    cpu.regs[rt] = (int64_t)imm;
}

void addiu(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);

    if (DEBUG)
    {
        cpu.debug << std::hex << "addiu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }

    int32_t result = (int32_t)cpu.regs[rs] + (int32_t)imm;
    cpu.regs[rt] = (int64_t)result;
}

void lw(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "lw - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    //imm = sign_extension(16, 64, imm);
    addr += (int64_t)imm;

    cpu.regs[rt] = (int64_t)((int32_t)cpu.mmu->read32(addr));
}
void lwl(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base] + imm;

    if (DEBUG)
    {
        cpu.debug << std::hex << "ldl - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    int offset = addr & 7;
    int shift = offset << 3;
    uint64_t mask = 0xFFFFFFFF << shift;
    uint64_t data = cpu.mmu->read32(addr - offset);
    //uint64_t oldreg = get_register(instruction.i.rt);

    cpu.regs[rt] = (cpu.regs[rt] & ~mask) | ((data << shift) & mask);
}
void lwr(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base] + imm;
    if (DEBUG)
    {
        cpu.debug << std::hex << "ld - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    int shift = 8 * ((addr ^ 7) & 7);
    uint64_t mask = 0xFFFFFFFF >> shift;
    uint64_t data = cpu.mmu->read32(addr & ~7);
    //uint64_t oldreg = get_register(instruction.i.rt);

    cpu.regs[rt] = (cpu.regs[rt] & ~mask) | (data >> shift);
}
void bne(CPU &cpu, uint32_t opcode)
{

    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "bne - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(18, 64, branch_addr);

    uint32_t delay_slot = cpu.pc + 4;

    branch_addr += delay_slot;

    if (cpu.regs[rs] != cpu.regs[rt])
    {
        cpu.pc = branch_addr - 4;
    }
    else
    {
        cpu.pc += 4;
    }
    cpu.emulate_cycle(delay_slot); //emulate delay slots
}

void sw(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "sw - " << opcode << "\n rt: " << (int)rt << " base: " << (int)base << " imm: " << (int)imm << "\n\n";
    }

    addr += (int64_t)imm;

    cpu.mmu->write32(addr, cpu.regs[rt]);
    //std::cout << "SW: " << std::hex << (uint32_t)cpu.mmu->read32((uint32_t)cpu.regs[29]) << '\n';
}

void ori(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "ori - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    cpu.regs[rt] = cpu.regs[rs] | imm;
}

void addi(CPU &cpu, uint32_t opcode) //TODO: fix me
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "addi - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }

    uint32_t x = cpu.regs[rs] & 0xFFFFFFFF;
    int32_t tmp = x + (int32_t)imm;
    
    // if (!(tmp >> 64)) //is this u128 lmao
    cpu.regs[rt] = (int64_t)tmp;
}
void sll(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    uint8_t sa = (opcode >> 6) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "sll - " << opcode << "\n rt: " << (int)rt << " rd: " << (int)rd << " sa: " << (int)sa << "\n\n";
    }
    // if (sa == 0)
    //     return; //nop
    uint32_t temp = cpu.regs[rt] & 0xFFFFFFFF;
    int32_t res = temp << sa;
    cpu.regs[rd] = (int64_t)res;
}
void dsll(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    uint8_t sa = (opcode >> 6) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "dsll - " << opcode << "\n rt: " << (int)rt << " rd: " << (int)rd << " sa: " << (int)sa << "\n\n";
    }
   
    int64_t res = cpu.regs[rt] << sa;
    cpu.regs[rd] = res;
}
void dsll32(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    uint8_t sa = (opcode >> 6) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "dsll32 - " << opcode << "\n rt: " << (int)rt << " rd: " << (int)rd << " sa: " << (int)sa << "\n\n";
    }
   
    int64_t res = cpu.regs[rt] << (sa + 32);
    cpu.regs[rd] = res;
}
void srl(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 11) & 0b1111'1;
    uint8_t sa = (opcode >> 6) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "srl - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " sa: " << (int)sa << "\n\n";
    }
    bool sign = (cpu.regs[rt] >> 63);
    //if (sign) //ISSUE
    //  sign = true;
    uint32_t test = cpu.regs[rt] & 0xFFFFFFFF;
    int32_t result = test >> sa;

    cpu.regs[rs] = (int64_t)result;
}
void dsrl(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 11) & 0b1111'1;
    uint8_t sa = (opcode >> 6) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "dsrl - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " sa: " << (int)sa << "\n\n";
    }

    int64_t result = cpu.regs[rt] >> sa;

    cpu.regs[rs] = (int64_t)result;
}
void dsrl32(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 11) & 0b1111'1;
    uint8_t sa = (opcode >> 6) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "dsrl32 - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " sa: " << (int)sa << "\n\n";
    }

    int64_t result = cpu.regs[rt] >> (sa + 32);

    cpu.regs[rs] = (int64_t)result;
}
void _or(CPU &cpu, uint32_t opcode)
{
    uint32_t rs = (opcode >> 21) & 0b1111'1;
    uint32_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "or - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    cpu.regs[rd] = cpu.regs[rs] | cpu.regs[rt];
}
void _nor(CPU &cpu, uint32_t opcode)
{
    uint32_t rs = (opcode >> 21) & 0b1111'1;
    uint32_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "nor - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    cpu.regs[rd] = ~(cpu.regs[rs] | cpu.regs[rt]);
}
void _and(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "and - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    cpu.regs[rd] = cpu.regs[rs] & cpu.regs[rt];
}
void jr(CPU &cpu, uint32_t opcode)
{
    uint32_t delay_slot = cpu.pc + 4;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "jr - " << opcode << "\nrs" << rs << "\n\n";
    }
    uint64_t addr = cpu.regs[rs] - 4;

    //emulate delay slots
    cpu.pc = addr; //this is or really right or really wrong
    cpu.emulate_cycle(delay_slot);
}

void addu(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "addu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    int32_t result = (uint32_t)cpu.regs[rt] + (uint32_t)cpu.regs[rs];
    cpu.regs[rd] = (int64_t)result;
}
void add(CPU &cpu, uint32_t opcode) //TODO: fix me
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "add - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }

    int32_t tmp = (int32_t)cpu.regs[rs] + (int32_t)cpu.regs[rt];
    
    cpu.regs[rd] = (int64_t)tmp;
}
void sub(CPU &cpu, uint32_t opcode) //TODO: fix me
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "sub - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }

    int32_t tmp = (int32_t)cpu.regs[rs] - (int32_t)cpu.regs[rt];
    
    cpu.regs[rd] = (int64_t)tmp;
}
void subu(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "subu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    int32_t result = (uint32_t)cpu.regs[rs] - (uint32_t)cpu.regs[rt];
    cpu.regs[rd] = (int64_t)result;
}
void dsubu(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "subu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    int64_t result = (uint64_t)cpu.regs[rs] - (uint64_t)cpu.regs[rt];
    cpu.regs[rd] = result;
}
void slt(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    int64_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "slt - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }

    //imm = sign_extension(16, 32, imm);
    if ((int64_t)cpu.regs[rs] < (int64_t)cpu.regs[rt])
    {
        cpu.regs[rd] = 1;
    }
    else
    {
        cpu.regs[rd] = 0;
    }
}
void sltu(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    int64_t rd = (opcode >> 11) & 0b1111'1;

    if (DEBUG)
    {
        cpu.debug << std::hex << "sltu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    //int32_t test1 = cpu.regs[rt];
    //int32_t test2 = cpu.regs[rs];
    //imm = sign_extension(16, 32, imm);
    if ((uint64_t)cpu.regs[rs] < (uint64_t)cpu.regs[rt])
    {
        cpu.regs[rd] = 1;
    }
    else
    {
        cpu.regs[rd] = 0;
    }
}
void mult(CPU &cpu, uint32_t opcode)
{
    //#pragma message owo
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    // uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "mult - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: "
                  << "\n\n";
    }
    int64_t x = (int32_t)(cpu.regs[rt] & 0xFFFFFFFF);
    int64_t y = (int32_t)(cpu.regs[rs] & 0xFFFFFFFF);

    int64_t rez = x * y; //change to u128
    int32_t lo = rez & 0xFFFFFFFF;
    int32_t hi = rez >> 32;

    //sus
    cpu.LO = (int64_t)lo;
    cpu.HI = (int64_t)hi;
}
void multu(CPU &cpu, uint32_t opcode)
{
    //#pragma message owo
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    // uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "multu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: "
                  << "\n\n";
    }
    uint64_t x = cpu.regs[rt] & 0xFFFFFFFF;
    uint64_t y = cpu.regs[rs] & 0xFFFFFFFF;

    uint64_t rez = x * y; //change to u128
    int32_t lo = rez & 0xFFFFFFFF;
    int32_t hi = rez >> 32;

    //sus
    cpu.LO = (int64_t)lo;
    cpu.HI = (int64_t)hi;
}
void dmultu(CPU &cpu, uint32_t opcode)
{
    //#pragma message owo
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    // uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "dmultu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: "
                  << "\n\n";
    }
    
    cpu.LO = multu_64_to_128(cpu.regs[rs], cpu.regs[rt], &cpu.HI);
}
void dmult(CPU &cpu, uint32_t opcode)
{
    //#pragma message owo
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    // uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "dmult - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: "
                  << "\n\n";
    }
    
    cpu.LO = mult_64_to_128(cpu.regs[rs], cpu.regs[rt], &cpu.HI);
}
void mflo(CPU &cpu, uint32_t opcode)
{
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    cpu.regs[rd] = cpu.LO;
}
void mfhi(CPU &cpu, uint32_t opcode)
{
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    cpu.regs[rd] = cpu.HI;
}
void srlv(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "srlv - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    uint8_t sa = cpu.regs[rs] & 0b11111'1;
    int32_t result = ((uint32_t)cpu.regs[rt]) >> sa;

    cpu.regs[rd] = (int64_t)result;
}
void dsrlv(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;

    if (DEBUG)
    {
        cpu.debug << std::hex << "srlv - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }

    uint8_t sa = cpu.regs[rs] & 0b11111'1;
    int64_t result = cpu.regs[rt] >> sa;

    cpu.regs[rd] = result;
}
void sllv(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "sllv - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    uint8_t sa = cpu.regs[rs] & 0b11111'1;
    int32_t result = ((uint32_t)cpu.regs[rt]) << sa;
    cpu.regs[rd] = (int64_t)result;

}
void dsllv(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "sllv - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    uint8_t sa = cpu.regs[rs] & 0b11111'1;
    int64_t result = cpu.regs[rt] << sa;
    cpu.regs[rd] = result;

}
void _xor(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "xor - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    cpu.regs[rd] = cpu.regs[rs] ^ cpu.regs[rt];
}
void _div(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;

    int32_t x = (int32_t)cpu.regs[rs];
    int32_t y = (int32_t)cpu.regs[rt];
    // if (DEBUG)
    // {
    //     cpu.debug << std::hex << "div - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    // }
    if (y == 0) 
    {
        cpu.HI = x;
        if (x >= 0) {
            cpu.LO = (int64_t)-1;
        } else {
            cpu.LO = (int64_t)1;
        }
    } 
    else 
    {
        int32_t quotient = x / y;
        int32_t remainder = x % y;

        cpu.LO = (int64_t)quotient;
        cpu.HI = (int64_t)remainder;
    }
}
void divu(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;

    uint32_t x = cpu.regs[rs];
    uint32_t y = cpu.regs[rt];
    // if (DEBUG)
    // {
    //     cpu.debug << std::hex << "div - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    // }
    if (y == 0) 
    {
        cpu.LO = 0xFFFFFFFFFFFFFFFF;
        cpu.HI = (int32_t)x;
    } 
    else 
    {
        int32_t quotient  = x / y;
        int32_t remainder = x % y;

        cpu.LO = quotient;
        cpu.HI = remainder;
    }
}
void ddiv(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;

    int64_t x = cpu.regs[rs];
    int64_t y = cpu.regs[rt];
    // if (DEBUG)
    // {
    //     cpu.debug << std::hex << "div - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    // }
    if (y == 0) 
    {
        cpu.HI = x;
        if (x >= 0) 
        {
            cpu.LO = (int64_t)-1;
        } 
        else 
        {
            cpu.LO = (int64_t)1;
        }
    } 
    else 
    {
        int64_t quotient = x / y;
        int64_t remainder = x % y;

        cpu.LO = (int64_t)quotient;
        cpu.HI = (int64_t)remainder;
    }
}
void ddivu(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;

    uint64_t x = cpu.regs[rs];
    uint64_t y = cpu.regs[rt];
    // if (DEBUG)
    // {
    //     cpu.debug << std::hex << "div - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    // }
    if (y == 0) 
    {
        cpu.LO = 0xFFFFFFFFFFFFFFFF;
        cpu.HI = (int64_t)x;
    } 
    else 
    {
        int64_t quotient  = x / y;
        int64_t remainder = x % y;

        cpu.LO = quotient;
        cpu.HI = remainder;
    }
}
void special_handler(CPU &cpu, uint32_t opcode)
{
    uint8_t instr = opcode & 0b11'1111;
    switch (instr)
    {
    case 0: //sll
    {
        sll(cpu, opcode);
        break;
    }
    case 0x2: //srl
    {
        srl(cpu, opcode);
        break;
    }
    case 0x4: //sllv
    {
        sllv(cpu, opcode);
        break;
    }
    case 0x6: //srlv
    {
        srlv(cpu, opcode);
        break;
    }
    case 0x8: //jr
    {
        jr(cpu, opcode);
        break;
    }
    case 0x9: //jral
    {
        jral(cpu, opcode);
        break;
    }
    case 0xF: //sync
    {
        break;
    }
    case 0x10: //mfhi
    {
        mfhi(cpu, opcode);
        break;
    }
    case 0x12: //mflo
    {
        mflo(cpu, opcode);
        break;
    }
    case 0x14: //dsllv
    {
        dsllv(cpu, opcode);
        break;
    }
    case 0x16: //dsrlv
    {
        dsrlv(cpu, opcode);
        break;
    }
    case 0x18: //mult
    {
        mult(cpu, opcode);
        break;
    }
    case 0x19: //multu
    {
        multu(cpu, opcode);
        break;
    }
    case 0x1b: //divu
    {
        divu(cpu, opcode);
        break;
    }
    case 0x1c: //dmult
    {
        dmult(cpu, opcode);
        break;
    }
    case 0x1d: //dmultu
    {
        dmultu(cpu, opcode);
        break;
    }
    case 0x1e: //ddiv
    {
        ddiv(cpu, opcode);
        break;
    }
    case 0x1f: //ddivu
    {
        ddivu(cpu, opcode);
        break;
    }
    case 0x1a: //div
    {
        _div(cpu, opcode);
        break;
    }
    case 0x20: //add
    {
        add(cpu, opcode);
        break;
    }
    case 0x21: //addu
    {
        addu(cpu, opcode);
        break;
    }
    case 0x22: //sub
    {
        sub(cpu, opcode);
        break;
    }
    case 0x23: //subu
    {
        subu(cpu, opcode);
        break;
    }
    case 0x24: //and
    {
        _and(cpu, opcode);
        break;
    }
    case 0x25: //or
    {
        _or(cpu, opcode);
        break;
    }
    case 0x26: //xor
    {
        _xor(cpu, opcode);
        break;
    }
    case 0x27: //nor
    {
        _nor(cpu, opcode);
        break;
    }
    case 0x2a: //slt
    {
        slt(cpu, opcode);
        break;
    }
    case 0x2b: //sltu
    {
        sltu(cpu, opcode);
        break;
    }
    case 0x2f: //dsubu
    {
        dsubu(cpu, opcode);
        break;
    }
    case 0x38: //dsll
    {
        dsll(cpu, opcode);
        break;
    }
    case 0x3a: //dsrl
    {
        dsrl(cpu, opcode);
        break;
    }
    case 0x3c: //dsll32
    {
        dsll32(cpu, opcode);
        break;
    }
    case 0x3e: //dsrl32
    {
        dsrl32(cpu, opcode);
        break;
    }
    default:
    {
        std::cout << "i'm special: " << std::hex << (int)(instr) << "\n\n";
        exit(0);
    }
    }
}

void beq(CPU &cpu, uint32_t opcode)
{
    uint32_t delay_slot = cpu.pc + 4;

    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "beq - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(18, 64, branch_addr);

    branch_addr += delay_slot;

    if (cpu.regs[rs] == cpu.regs[rt])
    {
        cpu.pc = branch_addr - 4;
    }
    else
    {
        cpu.pc += 4;
    }
    cpu.emulate_cycle(delay_slot); //emulate delay slots
}

void jal(CPU &cpu, uint32_t opcode)
{
    uint32_t target = (opcode & 0x3FFFFFF) << 2;
    uint32_t delay_slot = cpu.pc + 4;
    if (DEBUG)
    {
        cpu.debug << std::hex << "jal - " << opcode << "\ntarget" << target << "\n\n";
    }
    cpu.emulate_cycle(delay_slot); //emulate delay slots
    uint32_t addr = ((delay_slot >> 26) << 26) | target;
    cpu.regs[31] = delay_slot + 4;
    cpu.pc = addr - 4;
}

void slti(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "slti - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }

    if ((int64_t)cpu.regs[rs] < (int64_t)imm)
    {
        cpu.regs[rt] = 1;
    }
    else
    {
        cpu.regs[rt] = 0;
    }
}

void beql(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "beql - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint32_t delay_slot = cpu.pc + 4;
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(18, 64, branch_addr);

    branch_addr += delay_slot;

    if (cpu.regs[rs] == cpu.regs[rt])
    {
        cpu.emulate_cycle(delay_slot); //emulate delay slots
        cpu.pc = branch_addr - 4;
    }
    else
    {
        cpu.pc += 4;
    }
}

void andi(CPU &cpu, uint32_t opcode) 
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint64_t imm = (opcode & 0b1111'1111'1111'1111);

    if (DEBUG)
    {
        cpu.debug << std::hex << "andi - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }

    cpu.regs[rt] = cpu.regs[rs] & imm;
}

void xori(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint64_t imm = (opcode & 0b1111'1111'1111'1111);

    if (DEBUG)
    {
        cpu.debug << std::hex << "xori - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    cpu.regs[rt] = cpu.regs[rs] ^ imm;
}

void bnel(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "beql - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint32_t delay_slot = cpu.pc + 4;
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(18, 64, branch_addr);

    branch_addr += delay_slot;

    if (cpu.regs[rs] != cpu.regs[rt])
    {
        cpu.emulate_cycle(delay_slot); //emulate delay slots
        cpu.pc = branch_addr - 4;
    }
    else
    {
        cpu.pc += 4;
    }
}

void blezl(CPU &cpu, uint32_t opcode)
{
    //uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "beql - " << opcode << "\n rt: "
                  << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint32_t delay_slot = cpu.pc + 4;
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(18, 64, branch_addr);

    branch_addr += delay_slot;

    if ((int64_t)cpu.regs[rs] <= 0)
    {
        cpu.emulate_cycle(delay_slot); //emulate delay slots
        cpu.pc = branch_addr - 4;
    }
    else
    {
        cpu.pc += 4;
    }
}
void bgezal(CPU &cpu, uint32_t opcode)
{
    uint32_t delay_slot = cpu.pc + 4;

    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "bgezal - " << opcode << "\n rt: "
                  << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(18, 64, branch_addr);

    branch_addr += delay_slot;
    cpu.regs[31] = delay_slot + 4;
    if ((int32_t)cpu.regs[rs] >= 0)
    {
        cpu.pc = branch_addr - 4;
    }
    cpu.emulate_cycle(delay_slot); //emulate delay slots
}
void bgezl(CPU &cpu, uint32_t opcode)
{
    uint32_t delay_slot = cpu.pc + 4;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "bgezal - " << opcode << "\n rt: "
                  << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(18, 64, branch_addr);

    branch_addr += delay_slot;
    if ((int32_t)cpu.regs[rs] >= 0)
    {
        cpu.emulate_cycle(delay_slot); //emulate delay slots
        cpu.pc = branch_addr - 4;
    }
    else
    {
        cpu.pc += 4;
    }
}
void regimm_handler(CPU &cpu, uint32_t opcode)
{
    uint8_t instr = (opcode >> 16) & 0b11'1111;
    switch (instr)
    {
    case 0x3: //bgezl
    {
        bgezl(cpu, opcode);
        break;
    }
    case 0x11: //bgezal
    {
        bgezal(cpu, opcode);
        break;
    }
    default:
    {
        std::cout << "REGIMM: " << std::hex << (int)instr << " not implemented!";
        exit(0);
        break;
    }
    }
}
void j(CPU &cpu, uint32_t opcode)
{
    uint32_t target = (opcode & 0x3FFFFFF) << 2;
    uint32_t delay_slot = cpu.pc + 4;
    if (DEBUG)
    {
        cpu.debug << std::hex << "j - " << opcode << "\ntarget" << target << "\n\n";
    }
    cpu.emulate_cycle(delay_slot); //emulate delay slots
    uint32_t addr = ((delay_slot >> 26) << 26) | target;
    cpu.pc = addr - 4;
}

void lbu(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "lbu - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    //imm = sign_extension(16, 64, imm);
    addr += (int64_t)imm;

    cpu.regs[rt] = cpu.mmu->read8(addr);
}

void lb(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "lb - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    //imm = sign_extension(16, 64, imm);
    addr += (int64_t)imm;

    cpu.regs[rt] = (int64_t)((int8_t)cpu.mmu->read8(addr));
}

void bgtz(CPU &cpu, uint32_t opcode)
{
    uint32_t delay_slot = cpu.pc + 4;

    //uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "beql - " << opcode << "\n rt: "
                  << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(18, 64, branch_addr);

    branch_addr += delay_slot;

    if ((int64_t)cpu.regs[rs] > 0)
    {
        cpu.pc = branch_addr - 4;
    }
    else
    {
        cpu.pc += 4;
    }
    cpu.emulate_cycle(delay_slot); //emulate delay slots
}

void lwu(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "lw - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    //imm = sign_extension(16, 64, imm);
    addr += (int64_t)imm;

    cpu.regs[rt] = cpu.mmu->read32(addr);
}
void daddi(CPU &cpu, uint32_t opcode) //TODO: fix me
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "addi - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }

    int64_t tmp = cpu.regs[rs] + (int64_t)imm;
    // if (!(tmp >> 64)) //is this u128 lmao
    cpu.regs[rt] = (int64_t)tmp;
}
void daddiu(CPU &cpu, uint32_t opcode) //TODO: fix me
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "addi - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }

    uint64_t tmp = cpu.regs[rs] + (int64_t)imm;
    // if (!(tmp >> 64)) //is this u128 lmao
    cpu.regs[rt] = tmp;
}
void jral(CPU &cpu, uint32_t opcode)
{
    uint32_t delay_slot = cpu.pc + 4;

    uint8_t rs = (opcode >> 21) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "jr - " << opcode << "\nrs" << rs << "\n\n";
    }
    uint64_t addr = cpu.regs[rs] - 4;
    cpu.regs[31] = delay_slot + 4;
    //emulate delay slots
    cpu.pc = addr; //this is or really right or really wrong
    cpu.emulate_cycle(delay_slot);
}

void sb(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "sb - " << opcode << "\n rt: " << (int)rt << " base: " << (int)base << " imm: " << (int)imm << "\n\n";
    }

    addr += (int64_t)imm;

    cpu.mmu->write8(addr, cpu.regs[rt]);
    //std::cout << "SW: " << std::hex << (uint32_t)cpu.mmu->read32((uint32_t)cpu.regs[29]) << '\n';
}
void lhu(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "lhu - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    //imm = sign_extension(16, 64, imm);
    addr += (int64_t)imm;

    cpu.regs[rt] = cpu.mmu->read16(addr);
}

void ld(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "ld - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    //imm = sign_extension(16, 64, imm);
    addr += (int64_t)imm;

    cpu.regs[rt] = cpu.mmu->read64(addr);
}
void ldl(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base] + imm;

    if (DEBUG)
    {
        cpu.debug << std::hex << "ldl - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    int offset = addr & 7;
    int shift = offset << 3;
    uint64_t mask = 0xFFFFFFFFFFFFFFFF << shift;
    uint64_t data = cpu.mmu->read64(addr - offset);
    //uint64_t oldreg = get_register(instruction.i.rt);

    cpu.regs[rt] = (cpu.regs[rt] & ~mask) | ((data << shift) & mask);
}
void ldr(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base] + imm;
    if (DEBUG)
    {
        cpu.debug << std::hex << "ld - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    int shift = 8 * ((addr ^ 7) & 7);
    uint64_t mask = (uint64_t)0xFFFFFFFFFFFFFFFF >> shift;
    uint64_t data = cpu.mmu->read64(addr & ~7);
    //uint64_t oldreg = get_register(instruction.i.rt);

    cpu.regs[rt] = (cpu.regs[rt] & ~mask) | (data >> shift);
}
void lh(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "lh - " << opcode << "\n rt: " << (int)cpu.regs[rt] << " base: " << (int)cpu.regs[base] << " imm: " << (int)imm << "\n\n";
    }

    //imm = sign_extension(16, 64, imm);
    addr += (int64_t)imm;

    cpu.regs[rt] = (int64_t)((int16_t)cpu.mmu->read16(addr));
}

void sh(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "sh - " << opcode << "\n rt: " << (int)rt << " base: " << (int)base << " imm: " << (int)imm << "\n\n";
    }

    addr += (int64_t)imm;

    cpu.mmu->write16(addr, cpu.regs[rt]);
    //std::cout << "SW: " << std::hex << (uint32_t)cpu.mmu->read32((uint32_t)cpu.regs[29]) << '\n';
}
void sd(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t base = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    uint64_t addr = cpu.regs[base];
    if (DEBUG)
    {
        cpu.debug << std::hex << "sd - " << opcode << "\n rt: " << (int)rt << " base: " << (int)base << " imm: " << (int)imm << "\n\n";
    }

    addr += (int64_t)imm;

    cpu.mmu->write64(addr, cpu.regs[rt]);
    //std::cout << "SW: " << std::hex << (uint32_t)cpu.mmu->read32((uint32_t)cpu.regs[29]) << '\n';
}
