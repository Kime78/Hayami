#include "mmu.hpp"
#include <fstream>

uint32_t MMU::virt_to_phys(uint64_t virt)
{
    if ((virt >= 0xFFFFFFFF80000000 && virt <= 0xFFFFFFFF9FFFFFFF) || (virt >= 0xFFFFFFFFA0000000 && virt <= 0xFFFFFFFFBFFFFFFF))
    {
        return virtual_to_physical_direct(virt);
    }
    else
    {
        return virtual_to_physical_tbl(virt);
    }
}

uint32_t MMU::virtual_to_physical_direct(uint64_t virt)
{
    // if (virt >= 0xFFFFFFFF80000000 && virt <= 0xFFFFFFFF9FFFFFFF) //KSEG0
    // {
    //     return virt - 0xFFFFFFFF80000000;
    // }
    // else if (virt >= 0xFFFFFFFFA0000000 && virt <= 0xFFFFFFFFBFFFFFFF) //KSEG1
    // {
    //     return virt - 0xFFFFFFFFA0000000;
    // }
    return virt & 0x1fffffff;
}
uint32_t MMU::virtual_to_physical_tbl(uint64_t virt)
{
    return virt & 0x1fffffff;
}
uint64_t MMU::read64(uint64_t addr)
{
    uint64_t phys = virt_to_phys(addr);
    if (phys >= 0x00000000 && phys <= 0x007FFFFF)
    {
        return (uint64_t)(rdram[phys]) << 56 | (uint64_t)(rdram[phys + 1]) << 48 | (uint64_t)(rdram[phys + 2]) << 40 |
               (uint64_t)(rdram[phys + 3]) << 32 | (uint64_t)(rdram[phys + 4]) << 24 | (uint64_t)(rdram[phys + 5]) << 16 | (uint64_t)(rdram[phys + 6]) << 8 | (uint64_t)(rdram[phys + 7]);
    }

    else if (phys >= 0x03F00000 && phys <= 0x03FFFFFF)
    {
        return (uint64_t)(rdram_regs[phys - 0x03F00000]) << 56 | (uint64_t)(rdram_regs[phys - 0x03F00000 + 1]) << 48 | (uint64_t)(rdram_regs[phys - 0x03F00000 + 2]) << 40 |
               (uint64_t)(rdram_regs[phys - 0x03F00000 + 3]) << 32 | (uint64_t)(rdram_regs[phys - 0x03F00000 + 4]) << 24 | (uint64_t)(rdram_regs[phys - 0x03F00000 + 5]) << 16 |
               (uint64_t)(rdram_regs[phys - 0x03F00000 + 6]) << 8 | (uint64_t)(rdram_regs[phys - 0x03F00000 + 7]);
    }

    else if (phys >= 0x04000000 && phys <= 0x04000FFF)
    {
        return (uint64_t)(sp_dmem[phys - 0x04000000]) << 56 | (uint64_t)(sp_dmem[phys - 0x04000000 + 1]) << 48 | (uint64_t)(sp_dmem[phys - 0x04000000 + 2]) << 40 |
               (uint64_t)(sp_dmem[phys - 0x04000000 + 3]) << 32 | (uint64_t)(sp_dmem[phys - 0x04000000 + 4]) << 24 | (uint64_t)(sp_dmem[phys - 0x04000000 + 5]) << 16 |
               (uint64_t)(sp_dmem[phys - 0x04000000 + 6]) << 8 | (uint64_t)(sp_dmem[phys - 0x04000000 + 7]);
    }

    else if (phys >= 0x04001000 && phys <= 0x04001FFF)
    {
        return (uint64_t)(sp_imem[phys - 0x04001000]) << 56 | (uint64_t)(sp_imem[phys - 0x04001000 + 1]) << 48 | (uint64_t)(sp_imem[phys - 0x04001000 + 2]) << 40 |
               (uint64_t)(sp_imem[phys - 0x04001000 + 3]) << 32 | (uint64_t)(sp_imem[phys - 0x04001000 + 4]) << 24 | (uint64_t)(sp_imem[phys - 0x04001000 + 5]) << 16 |
               (uint64_t)(sp_imem[phys - 0x04001000 + 6]) << 8 | (uint64_t)(sp_imem[phys - 0x04001000 + 7]);
    }

    else if (phys >= 0x04040000 && phys <= 0x040FFFFF)
    {
        return (uint64_t)(sp_regs[phys - 0x04040000]) << 56 | (uint64_t)(sp_regs[phys - 0x04040000 + 1]) << 48 | (uint64_t)(sp_regs[phys - 0x04040000 + 2]) << 40 |
               (uint64_t)(sp_regs[phys - 0x04040000 + 3]) << 32 | (uint64_t)(sp_regs[phys - 0x04040000 + 4]) << 24 | (uint64_t)(sp_regs[phys - 0x04040000 + 5]) << 16 |
               (uint64_t)(sp_regs[phys - 0x04040000 + 6]) << 8 | (uint64_t)(sp_regs[phys - 0x04040000 + 7]);
    }

    else if (phys >= 0x04100000 && phys <= 0x041FFFFF)
    {
        return (uint64_t)(dp_commds[phys - 0x04100000]) << 56 | (uint64_t)(dp_commds[phys - 0x04100000 + 1]) << 48 | (uint64_t)(dp_commds[phys - 0x04100000 + 2]) << 40 |
               (uint64_t)(dp_commds[phys - 0x04100000 + 3]) << 32 | (uint64_t)(dp_commds[phys - 0x04100000 + 4]) << 24 | (uint64_t)(dp_commds[phys - 0x04100000 + 5]) << 16 |
               (uint64_t)(dp_commds[phys - 0x04100000 + 6]) << 8 | (uint64_t)(dp_commds[phys - 0x04100000 + 7]);
    }

    else if (phys >= 0x04300000 && phys <= 0x043FFFFF)
    {
        return (uint64_t)(mips_int[phys - 0x04300000]) << 56 | (uint64_t)(mips_int[phys - 0x04300000 + 1]) << 48 | (uint64_t)(mips_int[phys - 0x04300000 + 2]) << 40 |
               (uint64_t)(mips_int[phys - 0x04300000 + 3]) << 32 | (uint64_t)(mips_int[phys - 0x04300000 + 4]) << 24 | (uint64_t)(mips_int[phys - 0x04300000 + 5]) << 16 |
               (uint64_t)(mips_int[phys - 0x04300000 + 6]) << 8 | (uint64_t)(mips_int[phys - 0x04300000 + 7]);
    }

    else if (phys >= 0x04400000 && phys <= 0x044FFFFF)
    {
        return (uint64_t)(video_int[phys - 0x04400000]) << 56 | (uint64_t)(video_int[phys - 0x04400000 + 1]) << 48 | (uint64_t)(video_int[phys - 0x04400000 + 2]) << 40 |
               (uint64_t)(video_int[phys - 0x04400000 + 3]) << 32 | (uint64_t)(video_int[phys - 0x04400000 + 4]) << 24 | (uint64_t)(video_int[phys - 0x04400000 + 5]) << 16 |
               (uint64_t)(video_int[phys - 0x04400000 + 6]) << 8 | (uint64_t)(video_int[phys - 0x04400000 + 7]);
    }

    else if (phys >= 0x04500000 && phys <= 0x045FFFFF)
    {
        return (uint64_t)(audio_int[phys - 0x04500000]) << 56 | (uint64_t)(audio_int[phys - 0x04500000 + 1]) << 48 | (uint64_t)(audio_int[phys - 0x04500000 + 2]) << 40 |
               (uint64_t)(audio_int[phys - 0x04500000 + 3]) << 32 | (uint64_t)(audio_int[phys - 0x04500000 + 4]) << 24 | (uint64_t)(audio_int[phys - 0x04500000 + 5]) << 16 |
               (uint64_t)(audio_int[phys - 0x04500000 + 6]) << 8 | (uint64_t)(audio_int[phys - 0x04500000 + 7]);
    }

    else if (phys >= 0x04600000 && phys <= 0x046FFFFF)
    {
        return (uint64_t)(periph_int[phys - 0x04600000]) << 56 | (uint64_t)(periph_int[phys - 0x04600000 + 1]) << 48 | (uint64_t)(periph_int[phys - 0x04600000 + 2]) << 40 |
               (uint64_t)(periph_int[phys - 0x04600000 + 3]) << 32 | (uint64_t)(periph_int[phys - 0x04600000 + 4]) << 24 | (uint64_t)(periph_int[phys - 0x04600000 + 5]) << 16 |
               (uint64_t)(periph_int[phys - 0x04600000 + 6]) << 8 | (uint64_t)(periph_int[phys - 0x04600000 + 7]);
    }

    else if (phys >= 0x04700000 && phys <= 0x047FFFFF)
    {
        return (uint64_t)(rdram_int[phys - 0x04700000]) << 56 | (uint64_t)(rdram_int[phys - 0x04700000 + 1]) << 48 | (uint64_t)(rdram_int[phys - 0x04700000 + 2]) << 40 |
               (uint64_t)(rdram_int[phys - 0x04700000 + 3]) << 32 | (uint64_t)(rdram_int[phys - 0x04700000 + 4]) << 24 | (uint64_t)(rdram_int[phys - 0x04700000 + 5]) << 16 |
               (uint64_t)(rdram_int[phys - 0x04700000 + 6]) << 8 | (uint64_t)(rdram_int[phys - 0x04700000 + 7]);
    }

    else if (phys >= 0x04800000 && phys <= 0x048FFFFF)
    {
        return (uint64_t)(serial_int[phys - 0x04800000]) << 56 | (uint64_t)(serial_int[phys - 0x04800000 + 1]) << 48 | (uint64_t)(serial_int[phys - 0x04800000 + 2]) << 40 |
               (uint64_t)(serial_int[phys - 0x04800000 + 3]) << 32 | (uint64_t)(serial_int[phys - 0x04800000 + 4]) << 24 | (uint64_t)(serial_int[phys - 0x04800000 + 5]) << 16 |
               (uint64_t)(serial_int[phys - 0x04800000 + 6]) << 8 | (uint64_t)(serial_int[phys - 0x04800000 + 7]);
    }

    else if (phys >= 0x05000000 && phys <= 0x1FBFFFFF)
    {
        return ((uint64_t)(rom[phys - 0x05000000]) << 56) | ((uint64_t)(rom[phys - 0x05000000 + 1]) << 48) | ((uint64_t)(rom[phys - 0x05000000 + 2] << 40)) |
               ((uint64_t)(rom[phys - 0x05000000 + 3]) << 32) | ((uint64_t)(rom[phys - 0x05000000 + 4]) << 24) | ((uint64_t)(rom[phys - 0x05000000 + 5]) << 16) |
               ((uint64_t)(rom[phys - 0x05000000 + 6]) << 8) | (rom[phys - 0x05000000 + 7]);
    }

    else if (phys >= 0x1FC00000 && phys <= 0x1FC007BF)
    {
        return (uint64_t)(pif_rom[phys - 0x1FC00000]) << 56 | (uint64_t)(pif_rom[phys - 0x1FC00000 + 1]) << 48 | (uint64_t)(pif_rom[phys - 0x1FC00000 + 2]) << 40 |
               (uint64_t)(pif_rom[phys - 0x1FC00000 + 3]) << 32 | (uint64_t)(pif_rom[phys - 0x1FC00000 + 4]) << 24 | (uint64_t)(pif_rom[phys - 0x1FC00000 + 5]) << 16 |
               (uint64_t)(pif_rom[phys - 0x1FC00000 + 6]) << 8 | (uint64_t)(pif_rom[phys - 0x1FC00000 + 7]);
    }

    else if (phys >= 0x1FC007C0 && phys <= 0x1FC007FF)
    {
        return (uint64_t)(pif_ram[phys - 0x1FC007FF]) << 56 | (uint64_t)(pif_ram[phys - 0x1FC007FF + 1]) << 48 | (uint64_t)(pif_ram[phys - 0x1FC007FF + 2]) << 40 |
               (uint64_t)(pif_ram[phys - 0x1FC007FF + 3]) << 32 | (uint64_t)(pif_ram[phys - 0x1FC007FF + 4]) << 24 | (uint64_t)(pif_ram[phys - 0x1FC007FF + 5]) << 16 |
               (uint64_t)(pif_ram[phys - 0x1FC007FF + 6]) << 8 | (uint64_t)(pif_ram[phys - 0x1FC007FF + 7]);
    }

    else if (phys >= 0x1FD00000 && phys <= 0x7FFFFFFF)
    {
        return (uint64_t)(rom[phys - 0x05000000]) << 56 | (uint64_t)(rom[phys - 0x05000000 + 1]) << 48 | (uint64_t)(rom[phys - 0x05000000 + 2]) << 40 |
               (uint64_t)(rom[phys - 0x05000000 + 3]) << 32 | (uint64_t)(rom[phys - 0x05000000 + 4]) << 24 | (uint64_t)(rom[phys - 0x05000000 + 5]) << 16 |
               (uint64_t)(rom[phys - 0x05000000 + 6]) << 8 | (uint64_t)(rom[phys - 0x05000000 + 7]);
    }
}

uint32_t MMU::read32(uint64_t addr)
{
    uint64_t phys = virt_to_phys(addr);
    if (phys >= 0x00000000 && phys <= 0x007FFFFF)
    {
        return (uint32_t)(rdram[phys]) << 24 | (uint32_t)(rdram[phys + 1]) << 16 |
               (uint32_t)(rdram[phys + 2]) << 8 | (uint32_t)(rdram[phys + 3]);
    }

    if (phys >= 0x03F00000 && phys <= 0x03FFFFFF)
    {
        return (uint32_t)(rdram_regs[phys - 0x03F00000]) << 24 | (uint32_t)(rdram_regs[phys - 0x03F00000 + 1]) << 16 |
               (uint32_t)(rdram_regs[phys - 0x03F00000 + 2]) << 8 | (uint32_t)(rdram_regs[phys - 0x03F00000 + 3]);
    }

    if (phys >= 0x04000000 && phys <= 0x04000FFF)
    {
        return (uint32_t)(sp_dmem[phys - 0x04000000]) << 24 | (uint32_t)(sp_dmem[phys - 0x04000000 + 1]) << 16 |
               (uint32_t)(sp_dmem[phys - 0x04000000 + 2]) << 8 | (uint32_t)(sp_dmem[phys - 0x04000000 + 3]);
    }

    if (phys >= 0x04001000 && phys <= 0x04001FFF)
    {
        return (uint32_t)(sp_imem[phys - 0x04001000]) << 24 | (uint32_t)(sp_imem[phys - 0x04001000 + 1]) << 16 |
               (uint32_t)(sp_imem[phys - 0x04001000 + 2]) << 8 | (uint32_t)(sp_imem[phys - 0x04001000 + 3]);
    }

    if (phys >= 0x04040000 && phys <= 0x040FFFFF)
    {
        return (uint32_t)(sp_regs[phys - 0x04040000]) << 24 | (uint32_t)(sp_regs[phys - 0x04040000 + 1]) << 16 |
               (uint32_t)(sp_regs[phys - 0x04040000 + 2]) << 8 | (uint32_t)(sp_regs[phys - 0x04040000 + 3]);
    }

    if (phys >= 0x04100000 && phys <= 0x041FFFFF)
    {
        return (uint32_t)(dp_commds[phys - 0x04100000]) << 24 | (uint32_t)(dp_commds[phys - 0x04100000 + 1]) << 16 |
               (uint32_t)(dp_commds[phys - 0x04100000 + 2]) << 8 | (uint32_t)(dp_commds[phys - 0x04100000 + 3]);
    }

    if (phys >= 0x04300000 && phys <= 0x043FFFFF)
    {
        return (uint32_t)(mips_int[phys - 0x04300000]) << 24 | (uint32_t)(mips_int[phys - 0x04300000 + 1]) << 16 |
               (uint32_t)(mips_int[phys - 0x04300000 + 2]) << 8 | (uint32_t)(mips_int[phys - 0x04300000 + 3]);
    }

    if (phys >= 0x04400000 && phys <= 0x044FFFFF)
    {
        return (uint32_t)(video_int[phys - 0x04400000]) << 24 | (uint32_t)(video_int[phys - 0x04400000 + 1]) << 16 |
               (uint32_t)(video_int[phys - 0x04400000 + 2]) << 8 | (uint32_t)(video_int[phys - 0x04400000 + 3]);
    }

    if (phys >= 0x04500000 && phys <= 0x045FFFFF)
    {
        return (uint32_t)(audio_int[phys - 0x04500000]) << 24 | (uint32_t)(audio_int[phys - 0x04500000 + 5]) << 16 |
               (uint32_t)(audio_int[phys - 0x04500000 + 6]) << 8 | (uint32_t)(audio_int[phys - 0x04500000 + 7]);
    }

    if (phys >= 0x04600000 && phys <= 0x046FFFFF)
    {
        return (uint32_t)(periph_int[phys - 0x04600000]) << 24 | (uint32_t)(periph_int[phys - 0x04600000 + 1]) << 16 |
               (uint32_t)(periph_int[phys - 0x04600000 + 2]) << 8 | (uint32_t)(periph_int[phys - 0x04600000 + 3]);
    }

    if (phys >= 0x04700000 && phys <= 0x047FFFFF)
    {
        return (uint32_t)(rdram_int[phys - 0x04700000]) << 24 | (uint32_t)(rdram_int[phys - 0x04700000 + 1]) << 16 |
               (uint32_t)(rdram_int[phys - 0x04700000 + 2]) << 8 | (uint32_t)(rdram_int[phys - 0x04700000 + 3]);
    }

    if (phys >= 0x04800000 && phys <= 0x048FFFFF)
    {
        return (uint32_t)(serial_int[phys - 0x04800000]) << 24 | (uint32_t)(serial_int[phys - 0x04800000 + 1]) << 16 |
               (uint32_t)(serial_int[phys - 0x04800000 + 2]) << 8 | (uint32_t)(serial_int[phys - 0x04800000 + 3]);
    }

    if (phys >= 0x05000000 && phys <= 0x1FBFFFFF)
    {
        return (uint32_t)(rom[phys - 0x05000000]) << 24 | (uint32_t)(rom[phys - 0x05000000 + 1]) << 16 |
               (uint32_t)(rom[phys - 0x05000000 + 2]) << 8 | (uint32_t)(rom[phys - 0x05000000 + 3]);
    }

    if (phys >= 0x1FC00000 && phys <= 0x1FC007BF)
    {
        return (uint32_t)(pif_rom[phys - 0x1FC00000]) << 24 | (uint32_t)(pif_rom[phys - 0x1FC00000 + 1]) << 16 |
               (uint32_t)(pif_rom[phys - 0x1FC00000 + 2]) << 8 | (uint32_t)(pif_rom[phys - 0x1FC00000 + 3]);
    }

    if (phys >= 0x1FC007C0 && phys <= 0x1FC007FF)
    {
        return (uint32_t)(pif_ram[phys - 0x1FC007FF]) << 24 | (uint32_t)(pif_ram[phys - 0x1FC007FF + 1]) << 16 |
               (uint32_t)(pif_ram[phys - 0x1FC007FF + 2]) << 8 | (uint32_t)(pif_ram[phys - 0x1FC007FF + 3]);
    }

    if (phys >= 0x1FD00000 && phys <= 0x7FFFFFFF)
    {
        return (uint32_t)(rom[phys - 0x05000000]) << 24 | (uint32_t)(rom[phys - 0x05000000 + 1]) << 16 |
               (uint32_t)(rom[phys - 0x05000000 + 2]) << 8 | (uint32_t)(rom[phys - 0x05000000 + 3]);
    }
}

uint16_t MMU::read16(uint64_t addr)
{
    uint64_t phys = virt_to_phys(addr);
    if (phys >= 0x00000000 && phys <= 0x007FFFFF)
    {
        return (uint16_t)(rdram[phys]) << 8 | rdram[phys + 1];
    }

    if (phys >= 0x03F00000 && phys <= 0x3FFFFFFF)
    {
        return (uint16_t)(rdram_regs[phys - 0x03F00000]) << 8 | rdram_regs[phys - 0x03F00000 + 1];
    }

    if (phys >= 0x04000000 && phys <= 0x04000FFF)
    {
        return (uint16_t)(sp_dmem[phys - 0x04000000]) << 8 | sp_dmem[phys - 0x04000000 + 1];
    }

    if (phys >= 0x04001000 && phys <= 0x04001FFF)
    {
        return (uint16_t)(sp_imem[phys - 0x04001000]) << 8 | sp_imem[phys - 0x04001000 + 1];
    }

    if (phys >= 0x04040000 && phys <= 0x040FFFFF)
    {
        return (uint16_t)(sp_regs[phys - 0x04040000]) << 8 | sp_regs[phys - 0x04040000 + 1];
    }

    if (phys >= 0x04100000 && phys <= 0x041FFFFF)
    {
        return (uint16_t)(dp_commds[phys - 0x04100000]) << 8 | dp_commds[phys - 0x04100000 + 1];
    }

    if (phys >= 0x04300000 && phys <= 0x043FFFFF)
    {
        return (uint16_t)(mips_int[phys - 0x04300000]) << 8 | mips_int[phys - 0x04300000 + 1];
    }

    if (phys >= 0x04400000 && phys <= 0x044FFFFF)
    {
        return (uint16_t)(video_int[phys - 0x04400000]) << 8 | video_int[phys - 0x04400000 + 1];
    }

    if (phys >= 0x04500000 && phys <= 0x045FFFFF)
    {
        return (uint16_t)(audio_int[phys - 0x04500000 + 6]) << 8 | audio_int[phys - 0x04500000 + 7];
    }

    if (phys >= 0x04600000 && phys <= 0x046FFFFF)
    {
        return (uint16_t)(periph_int[phys - 0x04600000]) << 8 | periph_int[phys - 0x04600000 + 1];
    }

    if (phys >= 0x04700000 && phys <= 0x047FFFFF)
    {
        return (uint16_t)(rdram_int[phys - 0x04700000]) << 8 | rdram_int[phys - 0x04700000 + 1];
    }

    if (phys >= 0x04800000 && phys <= 0x048FFFFF)
    {
        return (uint16_t)(serial_int[phys - 0x04800000]) << 8 | serial_int[phys - 0x04800000 + 1];
    }

    if (phys >= 0x05000000 && phys <= 0x1FBFFFFF)
    {
        return (uint16_t)(rom[phys - 0x05000000]) << 8 | rom[phys - 0x05000000 + 1];
    }

    if (phys >= 0x1FC00000 && phys <= 0x1FC007BF)
    {
        return (uint16_t)(pif_rom[phys - 0x1FC00000]) << 8 | pif_rom[phys - 0x1FC00000 + 1];
    }

    if (phys >= 0x1FC007C0 && phys <= 0x1FC007FF)
    {
        return (uint16_t)(pif_ram[phys - 0x1FC007FF]) << 8 | pif_ram[phys - 0x1FC007FF + 1];
    }

    if (phys >= 0x1FD00000 && phys <= 0x7FFFFFFF)
    {
        return (uint16_t)(rom[phys - 0x05000000]) << 8 | rom[phys - 0x05000000 + 1];
    }
}

uint8_t MMU::read8(uint64_t addr)
{
    uint64_t phys = virt_to_phys(addr);
    if (phys >= 0x00000000 && phys <= 0x007FFFFF)
    {
        return rdram[phys];
    }

    else if (phys >= 0x03F00000 && phys <= 0x03FFFFFF)
    {
        return rdram_regs[phys - 0x03F00000];
    }

    else if (phys >= 0x04000000 && phys <= 0x04000FFF)
    {
        return sp_dmem[phys - 0x04000000];
    }

    else if (phys >= 0x04001000 && phys <= 0x04001FFF)
    {
        return sp_imem[phys - 0x04001000];
    }

    else if (phys >= 0x04040000 && phys <= 0x040FFFFF)
    {
        return sp_regs[phys - 0x04040000];
    }

    else if (phys >= 0x04100000 && phys <= 0x041FFFFF)
    {
        return dp_commds[phys - 0x04100000];
    }

    else if (phys >= 0x04300000 && phys <= 0x043FFFFF)
    {
        return mips_int[phys - 0x04300000];
    }

    else if (phys >= 0x04400000 && phys <= 0x044FFFFF)
    {
        return video_int[phys - 0x04400000];
    }

    else if (phys >= 0x04500000 && phys <= 0x045FFFFF)
    {
        return audio_int[phys - 0x04500000 + 7];
    }

    else if (phys >= 0x04600000 && phys <= 0x046FFFFF)
    {
        return periph_int[phys - 0x04600000];
    }

    else if (phys >= 0x04700000 && phys <= 0x047FFFFF)
    {
        return rdram_int[phys - 0x04700000];
    }

    else if (phys >= 0x04800000 && phys <= 0x048FFFFF)
    {
        return serial_int[phys - 0x04800000];
    }

    else if (phys >= 0x05000000 && phys <= 0x1FBFFFFF)
    {
        return rom[phys - 0x05000000];
    }

    else if (phys >= 0x1FC00000 && phys <= 0x1FC007BF)
    {
        return pif_rom[phys - 0x1FC00000];
    }

    else if (phys >= 0x1FC007C0 && phys <= 0x1FC007FF)
    {
        return pif_ram[phys - 0x1FC007FF];
    }

    else if (phys >= 0x1FD00000 && phys <= 0x7FFFFFFF)
    {
        return rom[phys - 0x05000000];
    }
}

void MMU::write64(uint64_t addr, uint64_t value)
{
    uint8_t nibble1, nibble2, nibble3, nibble4, nibble5, nibble6, nibble7, nibble8;
    nibble8 = (value >> 56) & 0b1111'1111;
    nibble7 = (value >> 48) & 0b1111'1111;
    nibble6 = (value >> 40) & 0b1111'1111;
    nibble5 = (value >> 32) & 0b1111'1111;
    nibble4 = (value >> 24) & 0b1111'1111;
    nibble3 = (value >> 16) & 0b1111'1111;
    nibble2 = (value >> 8) & 0b1111'1111;
    nibble1 = value & 0b1111'1111;

    uint64_t phys = virt_to_phys(addr);
    if (phys >= 0x00000000 && phys <= 0x007FFFFF)
    {
        rdram[phys] = nibble8;
        rdram[phys + 1] = nibble7;
        rdram[phys + 2] = nibble6;
        rdram[phys + 3] = nibble5;
        rdram[phys + 4] = nibble4;
        rdram[phys + 5] = nibble3;
        rdram[phys + 6] = nibble2;
        rdram[phys + 7] = nibble1;
    }

    if (phys >= 0x03F00000 && phys <= 0x03FFFFFF)
    {
        rdram_regs[phys - 0x03F00000] = nibble8;
        rdram_regs[phys - 0x03F00000 + 1] = nibble7;
        rdram_regs[phys - 0x03F00000 + 2] = nibble6;
        rdram_regs[phys - 0x03F00000 + 3] = nibble5;
        rdram_regs[phys - 0x03F00000 + 4] = nibble4;
        rdram_regs[phys - 0x03F00000 + 5] = nibble3;
        rdram_regs[phys - 0x03F00000 + 6] = nibble2;
        rdram_regs[phys - 0x03F00000 + 7] = nibble1;
    }

    if (phys >= 0x04000000 && phys <= 0x04000FFF)
    {
        sp_dmem[phys - 0x04000000] = nibble8;
        sp_dmem[phys - 0x04000000 + 1] = nibble7;
        sp_dmem[phys - 0x04000000 + 2] = nibble6;
        sp_dmem[phys - 0x04000000 + 3] = nibble5;
        sp_dmem[phys - 0x04000000 + 4] = nibble4;
        sp_dmem[phys - 0x04000000 + 5] = nibble3;
        sp_dmem[phys - 0x04000000 + 6] = nibble2;
        sp_dmem[phys - 0x04000000 + 7] = nibble1;
    }

    if (phys >= 0x04001000 && phys <= 0x04001FFF)
    {
        sp_imem[phys - 0x04001000] = nibble8;
        sp_imem[phys - 0x04001000 + 1] = nibble7;
        sp_imem[phys - 0x04001000 + 2] = nibble6;
        sp_imem[phys - 0x04001000 + 3] = nibble5;
        sp_imem[phys - 0x04001000 + 4] = nibble4;
        sp_imem[phys - 0x04001000 + 5] = nibble3;
        sp_imem[phys - 0x04001000 + 6] = nibble2;
        sp_imem[phys - 0x04001000 + 7] = nibble1;
    }

    if (phys >= 0x04040000 && phys <= 0x040FFFFF)
    {
        sp_regs[phys - 0x04040000] = nibble8;
        sp_regs[phys - 0x04040000 + 1] = nibble7;
        sp_regs[phys - 0x04040000 + 2] = nibble6;
        sp_regs[phys - 0x04040000 + 3] = nibble5;
        sp_regs[phys - 0x04040000 + 4] = nibble4;
        sp_regs[phys - 0x04040000 + 5] = nibble3;
        sp_regs[phys - 0x04040000 + 6] = nibble2;
        sp_regs[phys - 0x04040000 + 7] = nibble1;
    }

    if (phys >= 0x04100000 && phys <= 0x041FFFFF)
    {
        dp_commds[phys - 0x04100000] = nibble8;
        dp_commds[phys - 0x04100000 + 1] = nibble7;
        dp_commds[phys - 0x04100000 + 2] = nibble6;
        dp_commds[phys - 0x04100000 + 3] = nibble5;
        dp_commds[phys - 0x04100000 + 4] = nibble4;
        dp_commds[phys - 0x04100000 + 5] = nibble3;
        dp_commds[phys - 0x04100000 + 6] = nibble2;
        dp_commds[phys - 0x04100000 + 7] = nibble1;
    }

    if (phys >= 0x04300000 && phys <= 0x043FFFFF)
    {
        mips_int[phys - 0x04300000] = nibble8;
        mips_int[phys - 0x04300000 + 1] = nibble7;
        mips_int[phys - 0x04300000 + 2] = nibble6;
        mips_int[phys - 0x04300000 + 3] = nibble5;
        mips_int[phys - 0x04300000 + 4] = nibble4;
        mips_int[phys - 0x04300000 + 5] = nibble3;
        mips_int[phys - 0x04300000 + 6] = nibble2;
        mips_int[phys - 0x04300000 + 7] = nibble1;
    }

    if (phys >= 0x04400000 && phys <= 0x044FFFFF)
    {
        video_int[phys - 0x04400000] = nibble8;
        video_int[phys - 0x04400000 + 1] = nibble7;
        video_int[phys - 0x04400000 + 2] = nibble6;
        video_int[phys - 0x04400000 + 3] = nibble5;
        video_int[phys - 0x04400000 + 4] = nibble4;
        video_int[phys - 0x04400000 + 5] = nibble3;
        video_int[phys - 0x04400000 + 6] = nibble2;
        video_int[phys - 0x04400000 + 7] = nibble1;
    }

    if (phys >= 0x04500000 && phys <= 0x045FFFFF)
    {
        audio_int[phys - 0x04500000] = nibble8;
        audio_int[phys - 0x04500000 + 1] = nibble7;
        audio_int[phys - 0x04500000 + 2] = nibble6;
        audio_int[phys - 0x04500000 + 3] = nibble5;
        audio_int[phys - 0x04500000 + 4] = nibble4;
        audio_int[phys - 0x04500000 + 5] = nibble3;
        audio_int[phys - 0x04500000 + 6] = nibble2;
        audio_int[phys - 0x04500000 + 7] = nibble1;
    }

    if (phys >= 0x04600000 && phys <= 0x046FFFFF)
    {
        periph_int[phys - 0x04600000] = nibble8;
        periph_int[phys - 0x04600000 + 1] = nibble7;
        periph_int[phys - 0x04600000 + 2] = nibble6;
        periph_int[phys - 0x04600000 + 3] = nibble5;
        periph_int[phys - 0x04600000 + 4] = nibble4;
        periph_int[phys - 0x04600000 + 5] = nibble3;
        periph_int[phys - 0x04600000 + 6] = nibble2;
        periph_int[phys - 0x04600000 + 7] = nibble1;
    }

    if (phys >= 0x04700000 && phys <= 0x047FFFFF)
    {
        rdram_int[phys - 0x04700000] = nibble8;
        rdram_int[phys - 0x04700000 + 1] = nibble7;
        rdram_int[phys - 0x04700000 + 2] = nibble6;
        rdram_int[phys - 0x04700000 + 3] = nibble5;
        rdram_int[phys - 0x04700000 + 4] = nibble4;
        rdram_int[phys - 0x04700000 + 5] = nibble3;
        rdram_int[phys - 0x04700000 + 6] = nibble2;
        rdram_int[phys - 0x04700000 + 7] = nibble1;
    }

    if (phys >= 0x04800000 && phys <= 0x048FFFFF)
    {
        serial_int[phys - 0x04800000] = nibble8;
        serial_int[phys - 0x04800000 + 1] = nibble7;
        serial_int[phys - 0x04800000 + 2] = nibble6;
        serial_int[phys - 0x04800000 + 3] = nibble5;
        serial_int[phys - 0x04800000 + 4] = nibble4;
        serial_int[phys - 0x04800000 + 5] = nibble3;
        serial_int[phys - 0x04800000 + 6] = nibble2;
        serial_int[phys - 0x04800000 + 7] = nibble1;
    }

    if (phys >= 0x1FC007C0 && phys <= 0x1FC007FF)
    {
        pif_ram[phys - 0x1FC007C0] = nibble8;
        pif_ram[phys - 0x1FC007C0 + 1] = nibble7;
        pif_ram[phys - 0x1FC007C0 + 2] = nibble6;
        pif_ram[phys - 0x1FC007C0 + 3] = nibble5;
        pif_ram[phys - 0x1FC007C0 + 4] = nibble4;
        pif_ram[phys - 0x1FC007C0 + 5] = nibble3;
        pif_ram[phys - 0x1FC007C0 + 6] = nibble2;
        pif_ram[phys - 0x1FC007C0 + 7] = nibble1;
    }
}

void MMU::write32(uint64_t addr, uint32_t value)
{
    uint8_t nibble1, nibble2, nibble3, nibble4;
    nibble4 = (value >> 24) & 0b1111'1111;
    nibble3 = (value >> 16) & 0b1111'1111;
    nibble2 = (value >> 8) & 0b1111'1111;
    nibble1 = value & 0b1111'1111;

    uint64_t phys = virt_to_phys(addr);
    if (phys >= 0x00000000 && phys <= 0x007FFFFF)
    {
        rdram[phys] = nibble4;
        rdram[phys + 1] = nibble3;
        rdram[phys + 2] = nibble2;
        rdram[phys + 3] = nibble1;
    }

    if (phys >= 0x04000000 && phys <= 0x04000FFF)
    {
        sp_dmem[phys - 0x04000000] = nibble4;
        sp_dmem[phys - 0x04000000 + 1] = nibble3;
        sp_dmem[phys - 0x04000000 + 2] = nibble2;
        sp_dmem[phys - 0x04000000 + 3] = nibble1;
    }

    if (phys >= 0x04001000 && phys <= 0x04001FFF)
    {
        sp_imem[phys - 0x04001000] = nibble4;
        sp_imem[phys - 0x04001000 + 1] = nibble3;
        sp_imem[phys - 0x04001000 + 2] = nibble2;
        sp_imem[phys - 0x04001000 + 3] = nibble1;
    }

    if (phys >= 0x04040000 && phys <= 0x040FFFFF)
    {
        sp_regs[phys - 0x04040000] = nibble4;
        sp_regs[phys - 0x04040000 + 1] = nibble3;
        sp_regs[phys - 0x04040000 + 2] = nibble2;
        sp_regs[phys - 0x04040000 + 3] = nibble1;
    }

    if (phys >= 0x04100000 && phys <= 0x041FFFFF)
    {
        dp_commds[phys - 0x04100000] = nibble4;
        dp_commds[phys - 0x04100000 + 1] = nibble3;
        dp_commds[phys - 0x04100000 + 2] = nibble2;
        dp_commds[phys - 0x04100000 + 3] = nibble1;
    }

    if (phys >= 0x04300000 && phys <= 0x043FFFFF)
    {
        mips_int[phys - 0x04300000] = nibble4;
        mips_int[phys - 0x04300000 + 1] = nibble3;
        mips_int[phys - 0x04300000 + 2] = nibble2;
        mips_int[phys - 0x04300000 + 3] = nibble1;
    }

    if (phys >= 0x04400000 && phys <= 0x044FFFFF)
    {
        video_int[phys - 0x04400000] = nibble4;
        video_int[phys - 0x04400000 + 1] = nibble3;
        video_int[phys - 0x04400000 + 2] = nibble2;
        video_int[phys - 0x04400000 + 3] = nibble1;
    }

    if (phys >= 0x04500000 && phys <= 0x045FFFFF)
    {
        audio_int[phys - 0x04500000] = nibble4;
        audio_int[phys - 0x04500000 + 1] = nibble3;
        audio_int[phys - 0x04500000 + 2] = nibble2;
        audio_int[phys - 0x04500000 + 3] = nibble1;
    }

    if (phys >= 0x04600000 && phys <= 0x046FFFFF)
    {
        if (phys == 0x0460000C)
        {
            //pi dma
            for (uint32_t i = 0; i <= value / 4; i++)
            {
                uint64_t write_to = read32(0x84600004) + 0xFFFFFFFF80000000;
                uint64_t val = read32(write_to + (i * 4));
                uint64_t dest = read32(0xFFFFFFFF84600000) + (i * 4);
                write32(dest + 0xFFFFFFFF80000000, val);
            }
        }
        periph_int[phys - 0x04600000] = nibble4;
        periph_int[phys - 0x04600000 + 1] = nibble3;
        periph_int[phys - 0x04600000 + 2] = nibble2;
        periph_int[phys - 0x04600000 + 3] = nibble1;
    }

    if (phys >= 0x04700000 && phys <= 0x047FFFFF)
    {
        rdram_int[phys - 0x04700000] = nibble4;
        rdram_int[phys - 0x04700000 + 1] = nibble3;
        rdram_int[phys - 0x04700000 + 2] = nibble2;
        rdram_int[phys - 0x04700000 + 3] = nibble1;
    }

    if (phys >= 0x04800000 && phys <= 0x048FFFFF)
    {
        serial_int[phys - 0x04800000] = nibble4;
        serial_int[phys - 0x04800000 + 1] = nibble3;
        serial_int[phys - 0x04800000 + 2] = nibble2;
        serial_int[phys - 0x04800000 + 3] = nibble1;
    }

    if (phys >= 0x1FC007C0 && phys <= 0x1FC007FF)
    {
        pif_ram[phys - 0x1FC007C0] = nibble4;
        pif_ram[phys - 0x1FC007C0 + 1] = nibble3;
        pif_ram[phys - 0x1FC007C0 + 2] = nibble2;
        pif_ram[phys - 0x1FC007C0 + 3] = nibble1;
    }
}

void MMU::write16(uint64_t addr, uint16_t value)
{
    uint8_t nibble1, nibble2;
    nibble2 = (value >> 8) & 0b1111'1111;
    nibble1 = value & 0b1111'1111;

    uint64_t phys = virt_to_phys(addr);
    if (phys >= 0x00000000 && phys <= 0x007FFFFF)
    {
        rdram[phys] = nibble2;
        rdram[phys + 1] = nibble1;
    }

    if (phys >= 0x04000000 && phys <= 0x04000FFF)
    {
        sp_dmem[phys - 0x04000000] = nibble2;
        sp_dmem[phys - 0x04000000 + 1] = nibble1;
    }

    if (phys >= 0x04001000 && phys <= 0x04001FFF)
    {
        sp_imem[phys - 0x04001000] = nibble2;
        sp_imem[phys - 0x04001000 + 1] = nibble1;
    }

    if (phys >= 0x04040000 && phys <= 0x040FFFFF)
    {
        sp_regs[phys - 0x04040000] = nibble2;
        sp_regs[phys - 0x04040000 + 1] = nibble1;
    }

    if (phys >= 0x04100000 && phys <= 0x041FFFFF)
    {
        dp_commds[phys - 0x04100000] = nibble2;
        dp_commds[phys - 0x04100000 + 1] = nibble1;
    }

    if (phys >= 0x04300000 && phys <= 0x043FFFFF)
    {
        mips_int[phys - 0x04300000] = nibble2;
        mips_int[phys - 0x04300000 + 1] = nibble1;
    }

    if (phys >= 0x04400000 && phys <= 0x044FFFFF)
    {
        video_int[phys - 0x04400000] = nibble2;
        video_int[phys - 0x04400000 + 1] = nibble1;
    }

    if (phys >= 0x04500000 && phys <= 0x045FFFFF)
    {
        audio_int[phys - 0x04500000] = nibble2;
        audio_int[phys - 0x04500000 + 1] = nibble1;
    }

    if (phys >= 0x04600000 && phys <= 0x046FFFFF)
    {
        periph_int[phys - 0x04600000] = nibble2;
        periph_int[phys - 0x04600000 + 1] = nibble1;
    }

    if (phys >= 0x04700000 && phys <= 0x047FFFFF)
    {
        rdram_int[phys - 0x04700000] = nibble2;
        rdram_int[phys - 0x04700000 + 1] = nibble1;
    }

    if (phys >= 0x04800000 && phys <= 0x048FFFFF)
    {
        serial_int[phys - 0x04800000] = nibble2;
        serial_int[phys - 0x04800000 + 1] = nibble1;
    }

    if (phys >= 0x1FC007C0 && phys <= 0x1FC007FF)
    {
        pif_ram[phys - 0x1FC007C0] = nibble2;
        pif_ram[phys - 0x1FC007C0 + 1] = nibble1;
    }
}

void MMU::write8(uint64_t addr, uint8_t value)
{
    uint8_t nibble1;
    nibble1 = value & 0b1111'1111;

    uint64_t phys = virt_to_phys(addr);
    if (phys >= 0x00000000 && phys <= 0x007FFFFF)
    {
        rdram[phys] = nibble1;
    }

    if (phys >= 0x04000000 && phys <= 0x04000FFF)
    {
        sp_dmem[phys - 0x04000000] = nibble1;
    }

    if (phys >= 0x04001000 && phys <= 0x04001FFF)
    {
        sp_imem[phys - 0x04001000] = nibble1;
    }

    if (phys >= 0x04040000 && phys <= 0x040FFFFF)
    {
        sp_regs[phys - 0x04040000] = nibble1;
    }

    if (phys >= 0x04100000 && phys <= 0x041FFFFF)
    {
        dp_commds[phys - 0x04100000] = nibble1;
    }

    if (phys >= 0x04300000 && phys <= 0x043FFFFF)
    {
        mips_int[phys - 0x04300000] = nibble1;
    }

    if (phys >= 0x04400000 && phys <= 0x044FFFFF)
    {
        video_int[phys - 0x04400000] = nibble1;
    }

    if (phys >= 0x04500000 && phys <= 0x045FFFFF)
    {
        audio_int[phys - 0x04500000] = nibble1;
    }

    if (phys >= 0x04600000 && phys <= 0x046FFFFF)
    {
        periph_int[phys - 0x04600000] = nibble1;
    }

    if (phys >= 0x04700000 && phys <= 0x047FFFFF)
    {
        rdram_int[phys - 0x04700000] = nibble1;
    }

    if (phys >= 0x04800000 && phys <= 0x048FFFFF)
    {
        serial_int[phys - 0x04800000] = nibble1;
    }

    if (phys >= 0x1FC007C0 && phys <= 0x1FC007FF)
    {
        pif_ram[phys - 0x1FC007C0] = nibble1;
    }
}

void MMU::load_rom(std::string path)
{
    rom.resize(268'435'456);

    FILE *file = fopen(path.c_str(), "rb");

    if(file == 0)
    {
        fprintf(stderr, "ROM not found!");
        exit(EXIT_FAILURE);
    }
    
    int pos = 0;

    while (fread(&rom[pos + 0x0B000000], 1, 1, file))
    {
        pos++;
    }

    for (int i = 0; i < 0xfff; i++)
    {
        cartbridge_copy[i] = rom[i + 0x0B000000];
    }
}

MMU::MMU()
{
    load_rom("./roms/missing/daddiu.z64");
}