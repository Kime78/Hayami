#include "instructions.hpp"
#include <iostream>
int64_t sign_extension(int init_bits, int goal_bits, uint64_t val)
{
    uint64_t x = val;
    uint64_t mask = 0;
    bool msb = (val >> init_bits) & 0b1;
    for (int idx = init_bits + 1; idx < goal_bits; idx++)
    {
        mask |= msb;
        mask <<= 1;
    }
    mask <<= init_bits;
    if (msb)
        x |= mask;
    else
        x &= mask;
    return x;
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
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint64_t imm = (opcode & 0b1111'1111'1111'1111) << 16;
    //64 bit magic
    if (cpu.operation_mode = 1)
        imm = sign_extension(32, 64, imm);
    cpu.regs[rt] = imm;
}

void addiu(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t rs = (opcode >> 20) & 0b1111;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);

    if (cpu.operation_mode)
    {
        imm = sign_extension(16, 64, imm);
        uint64_t tmp = cpu.regs[rs] + imm;
        cpu.regs[rt] = tmp;
    }
    else
    {
        int32_t tmp = (int32_t)(sign_extension(16, 32, imm));
        cpu.regs[rt] = cpu.regs[rs] + tmp;
    }
}

void lw(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t base = (opcode >> 20) & 0b1111;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (cpu.operation_mode)
    {
        imm = sign_extension(16, 64, imm);
        cpu.regs[base] += imm;
    }
    else
    {
        imm = sign_extension(16, 32, imm);
        cpu.regs[base] += imm;
    }

    cpu.regs[rt] = cpu.mmu->read32(cpu.regs[base]);
}

void bne(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t rs = (opcode >> 20) & 0b1111;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
}