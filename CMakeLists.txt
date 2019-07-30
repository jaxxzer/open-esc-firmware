cmake_minimum_required(VERSION 3.14)

set(COMPILER_FLAGS "-fdata-sections -ffunction-sections")
set(LINKER_FLAGS "-specs=nano.specs -specs=nosys.specs --static -ggdb3 -Wl,--gc-sections -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group")

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

add_definitions(-DSTM32F0)

file(GLOB STM32_F0_SRC 
lib/libopencm3/lib/stm32/common/gpio_common_all.c
lib/libopencm3/lib/stm32/common/gpio_common_f0234.c
lib/libopencm3/lib/stm32/f0/*.c)

set(CMAKE_CXX_FLAGS "${COMPILER_FLAGS}")
message("cxx flags: ${CMAKE_CXX_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS} ${ARCH_FLAGS}")
message("ld flags: ${CMAKE_EXE_LINKER_FLAGS}")

# it's the only way
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")

add_executable(cm3-example-blink src/example/cm3/example-blink.cpp ${STM32_F0_SRC} lib/stm32-lib/src/startup/startup_stm32f0.s)
