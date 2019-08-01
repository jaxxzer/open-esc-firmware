cmake_minimum_required(VERSION 3.14)
project(gsc-firmware)
set(COMPILER_FLAGS "-fdata-sections -ffunction-sections")
set(LINKER_FLAGS "-specs=nano.specs -specs=nosys.specs --static -ggdb3 -Wl,--gc-sections -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group -T ${CMAKE_SOURCE_DIR}/lib/stm32-lib/src/link/stm32-mem.ld -T ${CMAKE_SOURCE_DIR}/lib/stm32-lib/src/link/stm32.ld")

# target board
set(TARGET_BOARD "wraith32")

# target board directory
set(TARGET_DIR "target/${TARGET_BOARD}")

# include target board variables
include(${TARGET_DIR}/CMakeLists.txt)

# target part
# TODO fail if not set
message("target mcu: ${TARGET_MCU}")

# target architecture
# todo fail if not set
message("arch flags: ${ARCH_FLAGS}")

include_directories(lib/libopencm3/include)
#include_directories(lib/stm32-lib/driver/stm32f0/cmsis/core)
#include_directories(lib/stm32-lib/driver/stm32f0/cmsis/device)
#include_directories(lib/stm32-lib/driver/stm32f0/spl/inc)

add_definitions(-DSTM32F0 -DSTM32F072)

file(GLOB STM32_F0_SRC 
lib/libopencm3/lib/stm32/common/gpio_common_all.c
lib/libopencm3/lib/stm32/common/gpio_common_f0234.c
lib/libopencm3/lib/stm32/common/rcc_common_all.c
lib/libopencm3/lib/stm32/f0/*.c)

set(CMAKE_C_FLAGS "${COMPILER_FLAGS} ${ARCH_FLAGS}")
message("cxx flags: ${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS "${COMPILER_FLAGS} ${ARCH_FLAGS}")
message("cxx flags: ${CMAKE_CXX_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS} ${ARCH_FLAGS}")
message("ld flags: ${CMAKE_EXE_LINKER_FLAGS}")

# it's the only way
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
enable_language(ASM)
#add_executable(cm3-example-blink src/example/cm3/example-blink.cpp ${STM32_F0_SRC} lib/stm32-lib/driver/stm32f0/cmsis/device/system_stm32f0xx.c lib/stm32-lib/src/startup/startup_stm32f0.s)
add_executable(cm3-example-blink src/example/cm3/example-blink.cpp ${STM32_F0_SRC}  lib/stm32-lib/src/startup/startup_stm32f0.s)

# TODO how to make clean?
add_custom_command(TARGET cm3-example-blink POST_BUILD
    COMMAND mkdir -p bin COMMAND cp cm3-example-blink bin/debug.elf
)
