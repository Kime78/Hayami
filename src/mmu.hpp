#include <stdint.h>
#include <array>
#include <vector>
#include <string>

class MMU //make this an unique ptr
{
public:
    void load_rom(std::string path);
    uint64_t read64(uint64_t addr);
    uint32_t read32(uint64_t addr);
    uint16_t read16(uint64_t addr);
    uint8_t read8(uint64_t addr);
    void write64(uint64_t addr, uint64_t value);
    void write32(uint64_t addr, uint32_t value);
    void write16(uint64_t addr, uint16_t value);
    void write8(uint64_t addr, uint8_t value);
    MMU();

private:
    std::array<uint8_t, 0x800000> rdram;
    std::array<uint8_t, 0xFFFFF> rdram_regs;
    std::array<uint8_t, 0xFFF> sp_dmem;
    std::array<uint8_t, 0xFFF> sp_imem;
    std::array<uint8_t, 0xBFFFF> sp_regs;
    std::array<uint8_t, 0xFFFFF> dp_commds;
    std::array<uint8_t, 0xFFFFF> span_regs;
    std::array<uint8_t, 0xFFFFF> mips_int;
    std::array<uint8_t, 0xFFFFF> video_int;
    std::array<uint8_t, 0xFFFFF> audio_int;
    std::array<uint8_t, 0xFFFFF> periph_int;
    std::array<uint8_t, 0xFFFFF> rdram_int;
    std::array<uint8_t, 0xFFFFF> serial_int;
    std::array<uint8_t, 0x7BF> pif_rom;
    std::array<uint8_t, 0x3F> pif_ram;
    std::vector<uint8_t> rom;
    uint32_t virtual_to_physical_direct(uint64_t virt); //KSEG0 KSEG1
    uint32_t virtual_to_physical_tbl(uint64_t virt);    //KUSEG KSSEG KSEG3
    uint32_t virt_to_phys(uint64_t virt);
};