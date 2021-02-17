#include <iostream>
#include "cpu.hpp"
#include <string.h>

CPU::CPU()
{
    mmu = std::make_unique<MMU>();
    simulate_pif();
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
        mmu->write64(0xA4000000 + ptr, temp);
        //std::cout << (uint64_t)mmu->read64(0xA4000000 + ptr) << std::endl;
    }

    pc = 0xA4000040;
}

uint8_t CPU::get_opcode()
{
    uint32_t opcode = mmu->read32(pc);
    return (opcode >> 56) & 0b1111'1111;
}

void CPU::emulate_cycle()
{
    uint8_t opcode = get_opcode();
    switch (opcode)
    {

    default:
        std::cout << "Instruction: " << std::hex << (int)opcode << " is not implemented";
        exit(-1);
        break;
    }
}