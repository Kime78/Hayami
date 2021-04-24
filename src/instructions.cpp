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
    int64_t imm = (int32_t)((opcode & 0b1111'1111'1111'1111) << 16);
    if (DEBUG)
    {
        cpu.debug << std::hex << "lui - " << opcode << "\n rt: " << (int)rt << " imm: " << (int)imm << "\n\n";
    }

    cpu.regs[rt] = imm;
}

void addiu(CPU &cpu, uint32_t opcode)
{
    //uint8_t rt = (opcode >> 16) & 0b1111;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "addiu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }

    //imm = sign_extension(16, 64, imm);
    int64_t result = (int32_t)cpu.regs[rs] + imm;
    cpu.regs[rt] = result;
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
    addr += imm;

    cpu.regs[rt] = (int64_t)((int32_t)cpu.mmu->read32(addr));
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
    uint32_t delay_slot = cpu.pc + 4;
    cpu.emulate_cycle(delay_slot); //emulate delay slots
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(17, 64, branch_addr);

    branch_addr += delay_slot;
    //sus
    // if (cpu.operation_mode == 0)
    // {
    //     branch_addr &= 0b1111'1111'1111'1111'1111'1111'1111'1111;
    // }
    if (cpu.regs[rs] != cpu.regs[rt])
    {
        cpu.pc = branch_addr - 4;
    }
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

    addr += imm;

    cpu.mmu->write32(addr, cpu.regs[rt]);
    //std::cout << "SW: " << std::hex << (uint32_t)cpu.mmu->read32((uint32_t)cpu.regs[29]) << '\n';
}

void ori(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
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

    int32_t tmp = cpu.regs[rs] + imm;
    // if (!(tmp >> 64)) //is this u128 lmao
    cpu.regs[rt] = (int64_t)tmp;
}
void sll(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 11) & 0b1111'1;
    uint8_t sa = (opcode >> 6) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "sll - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " sa: " << (int)sa << "\n\n";
    }
    // if (sa == 0)
    //     return; //nop
    int32_t res = (uint32_t)cpu.regs[rt] << sa;
    cpu.regs[rs] = (int64_t)res;
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

    int32_t result = (uint32_t)cpu.regs[rt] >> sa;

    cpu.regs[rs] = (int64_t)result;
}
void _or(CPU &cpu, uint32_t opcode)
{
    uint32_t rt = (opcode >> 21) & 0b1111'1;
    uint32_t rs = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "or - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    cpu.regs[rd] = cpu.regs[rs] | cpu.regs[rt];
}
void _and(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 21) & 0b1111'1;
    uint8_t rs = (opcode >> 16) & 0b1111'1;
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
    cpu.emulate_cycle(delay_slot);
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "jr - " << opcode << "\nrs" << rs << "\n\n";
    }
    uint64_t addr = cpu.regs[rs] - 4;

    //emulate delay slots
    cpu.pc = addr; //this is or really right or really wrong
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
    int32_t result = (int32_t)cpu.regs[rt] + (int32_t)cpu.regs[rs];
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

void slt(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
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
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
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
void subu(CPU &cpu, uint32_t opcode)
{
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "subu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    int32_t result = (int32_t)cpu.regs[rt] - (int32_t)cpu.regs[rs];
    cpu.regs[rd] = (int64_t)result;
}
void multu(CPU &cpu, uint32_t opcode)
{
    //#pragma message owo
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    // uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "subu - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: "
                  << "\n\n";
    }
    int32_t x = cpu.regs[rt];
    int32_t y = cpu.regs[rs];

    uint64_t rez = cpu.regs[rt] * cpu.regs[rs]; //change to u128
    cpu.LO = rez & 0b1111'1111'1111'1111;
    cpu.HI = rez >> 16;

    //sus
    cpu.LO = sign_extension(32, 64, cpu.LO);
    cpu.HI = sign_extension(32, 64, cpu.HI);
}
void mflo(CPU &cpu, uint32_t opcode)
{
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    cpu.regs[rd] = cpu.LO;
}
void srlv(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 21) & 0b1111'1;
    uint8_t rs = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "srlv - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    uint8_t sa = cpu.regs[rs] & 0b1111'1;
    int32_t result = (uint32_t)cpu.regs[rt] >> sa;

    cpu.regs[rd] = (int64_t)result;
}
void sllv(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 21) & 0b1111'1;
    uint8_t rs = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "sllv - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    uint8_t sa = cpu.regs[rs] & 0b1111'1;
    int32_t result = (uint32_t)cpu.regs[rt] << sa;
    cpu.regs[rd] = (int64_t)result;

    //cpu.regs[rd] = sign_extension(32, 64, cpu.regs[rd]);
}
void _xor(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 21) & 0b1111'1;
    uint8_t rs = (opcode >> 16) & 0b1111'1;
    uint8_t rd = (opcode >> 11) & 0b1111'1;
    if (DEBUG)
    {
        cpu.debug << std::hex << "xor - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " rd: " << (int)rd << "\n\n";
    }
    cpu.regs[rd] = cpu.regs[rs] ^ cpu.regs[rt];
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
    case 0x12: //mflo
    {
        mflo(cpu, opcode);
        break;
    }
    case 0x19: //multu
    {
        multu(cpu, opcode);
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
    cpu.emulate_cycle(delay_slot); //emulate delay slots
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int64_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "beq - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(17, 64, branch_addr);

    branch_addr += delay_slot;
    //sus
    // if (cpu.operation_mode == 0)
    // {
    //     branch_addr &= 0b1111'1111'1111'1111'1111'1111'1111'1111;
    // }

    if (cpu.regs[rs] == cpu.regs[rt])
    {
        cpu.pc = branch_addr - 4;
    }
}

void jal(CPU &cpu, uint32_t opcode)
{
    uint32_t target = (opcode & 0b1111'1111'1111'1111'1111'1111'1) << 2;
    uint32_t delay_slot = cpu.pc + 4;
    if (DEBUG)
    {
        cpu.debug << std::hex << "jal - " << opcode << "\ntarget" << target << "\n\n";
    }
    cpu.emulate_cycle(delay_slot); //emulate delay slots
    uint32_t addr = ((delay_slot >> 24) << 24) | target;
    cpu.regs[31] = delay_slot + 4;
    cpu.pc = addr - 4;
}

void slti(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
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

    branch_addr = sign_extension(17, 64, branch_addr);

    branch_addr += delay_slot;

    //sus
    // if (cpu.operation_mode == 0)
    // {
    //     branch_addr &= 0b1111'1111'1111'1111'1111'1111'1111'1111;
    // }

    if (cpu.regs[rs] == cpu.regs[rt])
    {
        cpu.emulate_cycle(delay_slot); //emulate delay slots
        cpu.pc = branch_addr - 4;
    }
}

void andi(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);

    if (DEBUG)
    {
        cpu.debug << std::hex << "andi - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    cpu.regs[rt] = cpu.regs[rs] & imm;
}

void xori(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);

    if (DEBUG)
    {
        cpu.debug << std::hex << "xori - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    cpu.regs[rt] = cpu.regs[rs] ^ imm;
}

void bnel(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "beql - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint32_t delay_slot = cpu.pc + 4;
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(17, 64, branch_addr);

    branch_addr += delay_slot;
    //sus
    // if (cpu.operation_mode == 0)
    // {
    //     branch_addr &= 0b1111'1111'1111'1111'1111'1111'1111'1111;
    // }

    if (cpu.regs[rs] != cpu.regs[rt])
    {
        cpu.emulate_cycle(delay_slot); //emulate delay slots
        cpu.pc = branch_addr - 4;
    }
}

void blezl(CPU &cpu, uint32_t opcode)
{
    uint8_t rt = (opcode >> 16) & 0b1111'1;
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "beql - " << opcode << "\n rt: " << (int)rt << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint32_t delay_slot = cpu.pc + 4;
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(17, 64, branch_addr);

    branch_addr += delay_slot;
    //sus
    // if (cpu.operation_mode == 0)
    // {
    //     branch_addr &= 0b1111'1111'1111'1111'1111'1111'1111'1111;
    // }

    if (cpu.regs[rs] <= cpu.regs[rt])
    {
        cpu.emulate_cycle(delay_slot); //emulate delay slots
        cpu.pc = branch_addr - 4;
    }
}
void bgezal(CPU &cpu, uint32_t opcode)
{
    uint32_t delay_slot = cpu.pc + 4;
    cpu.emulate_cycle(delay_slot); //emulate delay slots
    uint8_t rs = (opcode >> 21) & 0b1111'1;
    int16_t imm = (opcode & 0b1111'1111'1111'1111);
    if (DEBUG)
    {
        cpu.debug << std::hex << "bgezal - " << opcode << "\n rt: "
                  << " rs: " << (int)rs << " imm: " << (int)imm << "\n\n";
    }
    uint64_t branch_addr = imm << 2;

    branch_addr = sign_extension(17, 64, branch_addr);

    branch_addr += delay_slot;
    //sus
    // if (cpu.operation_mode == 0)
    // {
    //     branch_addr &= 0b1111'1111'1111'1111'1111'1111'1111'1111;
    // }
    if ((int32_t)cpu.regs[rs] >= 0)
    {
        cpu.pc = branch_addr - 4;
    }
}
void regimm_handler(CPU &cpu, uint32_t opcode)
{
    uint8_t instr = (opcode >> 16) & 0b11'1111;
    switch (instr)
    {
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