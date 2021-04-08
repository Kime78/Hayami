#include <array>
#include <stdint.h>
#include <memory>
#include <fstream>

#include "mmu.hpp"
class CPU
{
public:
    CPU();
    std::array<uint64_t, 32> regs;
    std::array<uint64_t, 32> cp0_regs;
    std::array<double, 32> floating_regs;
    std::unique_ptr<MMU> mmu;
    void emulate_cycle(uint32_t opcode);
    uint64_t pc;
    bool operation_mode = 0; //32 bit or 64 bit

private:
    void simulate_pif();
};