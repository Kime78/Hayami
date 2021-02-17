#include <array>
#include <stdint.h>
#include <memory>

#include "mmu.hpp"
class CPU
{
public:
    CPU();
    std::array<uint64_t, 32> regs;
    std::array<uint64_t, 32> cp0_regs;
    std::array<double, 32> floating_regs;
    std::unique_ptr<MMU> mmu;
    void emulate_cycle();

private:
    uint64_t pc;
    void simulate_pif();
    uint8_t get_opcode();
};