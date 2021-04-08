#include "cpu.hpp"

extern void cop_handler(CPU &cpu, uint32_t opcode);
extern void lui(CPU &cpu, uint32_t opcode);
extern void addiu(CPU &cpu, uint32_t opcode);
extern void lw(CPU &cpu, uint32_t opcode);
extern void bne(CPU &cpu, uint32_t opcode);
extern void sw(CPU &cpu, uint32_t opcode);
extern void ori(CPU &cpu, uint32_t opcode);
extern void addi(CPU &cpu, uint32_t opcode);
extern void special_handler(CPU &cpu, uint32_t opcode);
extern void beq(CPU &cpu, uint32_t opcode);