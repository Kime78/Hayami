#include "instructions.hpp"
#include <iostream>
#include <fstream>
#define DEBUG 1
std::ofstream debug("debug.out");
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
    if (DEBUG)
    {
        debug << std::hex << "lui - " << opcode << "\n rt: " << (int)rt << " imm: " << (int)imm << "\n\n";
    }

    //64 bit magic
    if (cpu.operation_mode)
        imm = sign_extension(32, 64, imm);
    cpu.regs[rt] = imm;
}

void addiu(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t rs = (opcode >> 20) & 0b1111;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        debug << std::hex << "addiu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
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
    uint8_t base = (opcode >> 21) & 0b1111;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        debug << std::hex << "lw - " << opcode << "\n rt: " << (int)rt << " base: " << (int)base << " imm: " << (int)imm << "\n\n";
    }
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
    if (DEBUG)
    {
        debug << std::hex << "bne - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint32_t delay_slot = cpu.pc + 4;
    cpu.emulate_cycle(delay_slot); //emulate delay slots
    uint64_t branch_addr = imm << 2;
    if (cpu.operation_mode)
    {
        imm = sign_extension(18, 64, imm);
    }
    else
    {
        imm = sign_extension(18, 32, imm);
    }

    branch_addr += delay_slot;
    if (cpu.regs[rs] != cpu.regs[rt])
    {
        cpu.pc = branch_addr;
    }
}

void sw(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t base = (opcode >> 21) & 0b1111;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        debug << std::hex << "sw - " << opcode << "\n rt: " << (int)rt << " base: " << (int)base << " imm: " << (int)imm << "\n\n";
    }
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
    cpu.mmu->write32(cpu.regs[base], cpu.regs[rt]);
}

void ori(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t rs = (opcode >> 20) & 0b1111;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        debug << std::hex << "ori - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    cpu.regs[rt] = cpu.regs[rs] || imm;
}

void addi(CPU &cpu, uint32_t opcode) //TODO: fix me
{
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t rs = (opcode >> 20) & 0b1111;
    uint64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        debug << std::hex << "addi - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    if (cpu.operation_mode)
    {
        imm = sign_extension(16, 64, imm);
        uint64_t tmp = cpu.regs[rs] + imm;
        if (!(tmp >> 64)) //is this u128 lmao
            cpu.regs[rt] = tmp;
    }
    else
    {
        imm = (int32_t)(sign_extension(16, 32, imm));
        uint64_t tmp = cpu.regs[rs] + imm;
        if (!(tmp >> 32))
            cpu.regs[rt] = tmp;
    }
}
void sll(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t rs = (opcode >> 11) & 0b1111;
    uint8_t sa = (opcode >> 6) & 0b1111;
    if (DEBUG)
    {
        debug << std::hex << "sll - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " sa: " << (int)sa << "\n\n";
    }
    // if (sa == 0)
    //     return; //nop

    cpu.regs[rs] = cpu.regs[rt] << sa;
    if (cpu.operation_mode)
        cpu.regs[rs] = sign_extension(32, 64, cpu.regs[rs]);
}
void _or(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 21) & 0b1111;
    uint8_t rs = (opcode >> 16) & 0b1111;
    uint8_t rd = (opcode >> 11) & 0b1111;
    if (DEBUG)
    {
        debug << std::hex << "or - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    cpu.regs[rd] = cpu.regs[rs] | cpu.regs[rt];
}
void special_handler(CPU &cpu, uint32_t opcode)
{
    uint8_t instr = opcode & 0b11'1111;
    switch (instr)
    {
    case 0:
    {
        sll(cpu, opcode);
        break;
    }
    case 0x25:
    {
        _or(cpu, opcode);
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
    uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t rs = (opcode >> 20) & 0b1111;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        debug << std::hex << "beq - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint32_t delay_slot = cpu.pc + 4;
    cpu.emulate_cycle(delay_slot); //emulate delay slots
    uint64_t branch_addr = imm << 2;
    if (cpu.operation_mode)
    {
        imm = sign_extension(18, 64, imm);
    }
    else
    {
        imm = sign_extension(18, 32, imm);
    }

    branch_addr += delay_slot;
    if (cpu.regs[rs] == cpu.regs[rt])
    {
        cpu.pc = branch_addr;
    }
}