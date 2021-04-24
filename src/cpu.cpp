#include <iostream>
#include <string.h>
#include "instructions.hpp"
#include "CRC.h"

CPU::CPU()
{
    mmu = std::make_unique<MMU>();
    simulate_pif();
    debug.open("debug.out");

    std::string region = "NTSC";
    switch (mmu->cartbridge_copy[0x3E])
    {
    case 'A':
        region = "NTSC";
        break; //Asia
    case 'B':
        region = "NTSC";
        break; //Brazil
    case 'C':
        region = "NTSC";
        break; //China
    case 'D':
        region = "PAL";
        break; //Germany
    case 'E':
        region = "NTSC";
        break; //North America
    case 'F':
        region = "PAL";
        break; //France
    case 'G':
        region = "NTSC";
        break; //Gateway 64 (NTSC)
    case 'H':
        region = "PAL";
        break; //Netherlands
    case 'I':
        region = "PAL";
        break; //Italy
    case 'J':
        region = "NTSC";
        break; //Japan
    case 'K':
        region = "NTSC";
        break; //Korea
    case 'L':
        region = "PAL";
        break; //Gateway 64 (PAL)
    case 'N':
        region = "NTSC";
        break; //Canada
    case 'P':
        region = "PAL";
        break; //Europe
    case 'S':
        region = "PAL";
        break; //Spain
    case 'U':
        region = "PAL";
        break; //Australia
    case 'W':
        region = "PAL";
        break; //Scandanavia
    case 'X':
        region = "PAL";
        break; //Europe
    case 'Y':
        region = "PAL";
        break; //Europe
    }

    bool ntsc = region == "NTSC";
    std::string cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";
    uint8_t test[0xfff];
    strcpy((char *)test, (char *)&mmu->cartbridge_copy[0x40]);
    std::uint32_t crc32 = CRC::Calculate(test, sizeof(test), CRC::CRC_32());
    if (crc32 == 0x1deb51a9)
        cic = ntsc ? "CIC-NUS-6101" : "CIC-NUS-7102";
    if (crc32 == 0xc08e5bd6)
        cic = ntsc ? "CIC-NUS-6102" : "CIC-NUS-7101";
    if (crc32 == 0x03b8376a)
        cic = ntsc ? "CIC-NUS-6103" : "CIC-NUS-7103";
    if (crc32 == 0xcf7f41dc)
        cic = ntsc ? "CIC-NUS-6105" : "CIC-NUS-7105";
    if (crc32 == 0xd1059c6a)
        cic = ntsc ? "CIC-NUS-6106" : "CIC-NUS-7106";

    if (cic == "CIC-NUS-6101" || cic == "CIC-NUS-7102")
        mmu->write32(0x80000024, 0x00043f3f);
    if (cic == "CIC-NUS-6102" || cic == "CIC-NUS-7101")
        mmu->write32(0x80000024, 0x00003f3f);
    if (cic == "CIC-NUS-6103" || cic == "CIC-NUS-7103")
        mmu->write32(0x80000024, 0x0000783f);
    if (cic == "CIC-NUS-6105" || cic == "CIC-NUS-7105")
        mmu->write32(0x80000024, 0x0000913f);
    if (cic == "CIC-NUS-6106" || cic == "CIC-NUS-7106")
        mmu->write32(0x80000024, 0x0000853f);
}
void CPU::simulate_pif()
{
    for (int i = 0; i < 32; i++)
        regs[i] = 0, floating_regs[i] = 0;

    regs[11] = 0xFFFFFFFFA4000040;
    regs[20] = 0x0000000000000001;
    regs[22] = 0x000000000000003F;
    regs[29] = 0xFFFFFFFFA4001FF0;

    cp0_regs[1] = 0x0000001F;
    cp0_regs[12] = 0x70400004;
    cp0_regs[15] = 0x00000B00;
    cp0_regs[16] = 0x0006E463;

    mmu->write64(0xA4300004, 0x01010101);

    for (int ptr = 0; ptr < 0x1000; ptr++)
    {
        uint64_t temp = mmu->read64(0xB0000000 + ptr);
        mmu->write64(0xFFFFFFFFA4000000 + ptr, temp);
        //std::cout << (uint64_t)mmu->read64(0xA4000000 + ptr) << std::endl;
    }

    pc = 0xFFFFFFFFA4000040;
}

// uint8_t CPU::get_opcode()
// {
//     uint32_t opcode = mmu->read32(pc);
//     return (opcode >> 26) & 0b11'1111;
// }

void CPU::emulate_cycle(int32_t arg)
{
    //std::cout << std::hex << (int)mmu->read8(0xb0000010) << '\n';
    regs[0] = 0;
    int64_t argument = (int64_t)arg;
    uint32_t opcode = mmu->read32(argument);
    uint8_t instr = (opcode >> 26) & 0b11'1111;
    //std::cout << "PC: " << std::hex << arg << " Instruction: " << (int)instr << ": " << (int)opcode << '\n';
    if (pc == 0x800001AC) //40 important
    {
        //todo debug why this has cursed address
        pc = pc;
        //exit(0);
    }

    //std::cout << std::hex << regs[15] << '\n';
    switch (instr)
    {
    case 0x0: //special :deepfried:
    {
        special_handler(*this, opcode);
        break;
    }
    case 0x1: //regimm handler
    {
        regimm_handler(*this, opcode);
        break;
    }
    case 0x3: //jal
    {
        jal(*this, opcode);
        break;
    }
    case 0x4: //beq
    {
        beq(*this, opcode);
        break;
    }
    case 0x5: //bne
    {
        bne(*this, opcode);
        break;
    }
    case 0x8: //addi
    {
        addi(*this, opcode);
        break;
    }
    case 0x9: //addiu
    {
        addiu(*this, opcode);
        break;
    }
    case 0xA: //slti
    {
        slti(*this, opcode);
        break;
    }
    case 0xC: //andi
    {
        andi(*this, opcode);
        break;
    }
    case 0xD: //ori
    {
        ori(*this, opcode);
        break;
    }
    case 0xE: //xori
    {
        xori(*this, opcode);
        break;
    }
    case 0xF: //lui
    {
        lui(*this, opcode);
        break;
    }
    case 0x10: //mtc0
    {
        //std::cout << "cop0 ";
        cop_handler(*this, opcode);
        break;
    }
    case 0x11: //mtc1
    {
        std::cout << "cop1 ";
        cop_handler(*this, opcode);
        break;
    }
    case 0x12: //mtc2
    {
        std::cout << "cop2 ";
        cop_handler(*this, opcode);
        break;
    }
    case 0x14: //beql
    {
        beql(*this, opcode);
        break;
    }
    case 0x15: //bnel
    {
        bnel(*this, opcode);
        break;
    }
    case 0x16: //blezl
    {
        blezl(*this, opcode);
        break;
    }
    case 0x23: //lw
    {
        lw(*this, opcode);
        break;
    }
    case 0x2B: //sw
    {
        sw(*this, opcode);
        break;
    }
    case 0x2f: //cache
    {
        break;
    }
    default:
        std::cout << "PC: " << std::hex << (uint64_t)pc << " Instruction: " << (int)instr << ": " << (int)opcode << " is not implemented";
        exit(-1);
        break;
    }
}