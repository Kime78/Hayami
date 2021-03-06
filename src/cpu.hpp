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
    void emulate_cycle(int32_t opcode);
    std::ofstream debug;
    uint64_t pc;
    uint64_t LO, HI;
    //bool operation_mode = 0; //32 bit or 64 bit

private:
    void simulate_pif();
};