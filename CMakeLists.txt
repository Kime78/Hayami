project(Hayami)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(Hayami
./src/cpu.cpp ./src/cpu.hpp ./src/CRC.h
./src/instructions.cpp ./src/instructions.hpp 
./src/mmu.cpp ./src/mmu.hpp
./src/main.cpp
./src/gpu.cpp
)
target_link_libraries(Hayami ${SDL2_LIBRARIES})
SET(CMAKE_CXX_FLAGS "-O0")