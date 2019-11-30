cmake_minimum_required(VERSION 3.12)
project(stm32-boilerplate-example)

set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")

set(BOILERPLATE ${PROJECT_SOURCE_DIR}/lib/stm32-boilerplate)
set(LIBOPENCM3 ${BOILERPLATE}/lib/libopencm3)
set(LIBOPENCM3_SRC ${LIBOPENCM3}/lib/stm32)

set(COMPILER_CXX_FLAGS "-fno-use-cxa-atexit")
set(COMPILER_FLAGS "-fdata-sections -ffunction-sections -O0 -DNDEBUG -Werror")
set(LINKER_FLAGS "-nostartfiles -specs=nano.specs -specs=nosys.specs --static -ggdb3 -Wl,--gc-sections -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group -Wl,-Map=output.map -T ${BOILERPLATE}/src/link/stm32-mem.ld -L ${BOILERPLATE}/lib/libopencm3/lib")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(src/target/target.cmake)

include_directories(${BOILERPLATE}/lib/libopencm3/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/ping-cpp/src/message)

set(CMAKE_C_FLAGS "${COMPILER_FLAGS} ${ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS "${COMPILER_CXX_FLAGS} ${COMPILER_FLAGS} ${ARCH_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS} ${ARCH_FLAGS}")

include(${BOILERPLATE}/src/target/f/f.cmake)

set(MAIN_SRC
  src/main.cpp
  ${ADC_SRC}
  ${COMP_SRC}
  ${DMA_SRC}
  ${GPIO_SRC}
  ${ISR_SRC}
  ${RCC_SRC}
  ${TARGET_SRC}
  ${TIMER_SRC}
  ${USART_SRC}
  ${COMPARATOR_SRC}
)

include_directories(
  ${PROJECT_SOURCE_DIR}/src/hal/inc
  ${PROJECT_SOURCE_DIR}/src/main/inc
)

set(PINGCPP_SRC ${PROJECT_SOURCE_DIR}/lib/ping-cpp/src)
set(PINGPROTOCOL_SRC ${PROJECT_SOURCE_DIR}/lib/ping-cpp/lib/ping-protocol/src)

set(COMMON_SRC
  ${BOILERPLATE}/lib/libopencm3/lib/cm3/nvic.c
  ${BOILERPLATE}/lib/libopencm3/lib/cm3/vector.c
  ${BOILERPLATE}/lib/libopencm3/lib/stm32/common/exti_common_all.c
  ${BOILERPLATE}/lib/libopencm3/lib/stm32/common/iwdg_common_all.c
  ${PROJECT_SOURCE_DIR}/src/hal/src/stm32/common/adc.c
  ${PROJECT_SOURCE_DIR}/src/hal/src/stm32/common/bridge.c
  ${PROJECT_SOURCE_DIR}/src/hal/src/stm32/common/global.c
  ${PROJECT_SOURCE_DIR}/src/hal/src/stm32/common/pwm-input.c
  ${PROJECT_SOURCE_DIR}/src/hal/src/stm32/common/system.c
  ${PROJECT_SOURCE_DIR}/src/hal/src/stm32/common/usart.c
  ${PROJECT_SOURCE_DIR}/src/hal/src/stm32/common/watchdog.c
  ${PROJECT_SOURCE_DIR}/src/main/src/audio.cpp
  ${PROJECT_SOURCE_DIR}/src/main/src/bemf.c
  ${PROJECT_SOURCE_DIR}/src/main/src/commutation.c
  ${PROJECT_SOURCE_DIR}/src/main/src/comparator-isr.c
  ${PROJECT_SOURCE_DIR}/src/main/src/console.c
  ${PROJECT_SOURCE_DIR}/src/main/src/debug-pins.c
  ${PROJECT_SOURCE_DIR}/src/main/src/io.cpp
  ${PROJECT_SOURCE_DIR}/src/main/src/led-gpio.c
  ${PROJECT_SOURCE_DIR}/src/main/src/overcurrent-watchdog.c
  ${PINGCPP_SRC}/message/ping-message-common.h
  ${PINGCPP_SRC}/message/ping-message-openesc.h
)

set(EXE_NAME open-esc-${TARGET_BOARD})
add_executable(${EXE_NAME} ${MAIN_SRC} ${COMMON_SRC})

# TODO how to make clean?
add_custom_command(TARGET ${EXE_NAME} POST_BUILD
  COMMAND arm-none-eabi-objcopy -O ihex ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME}.hex
  COMMAND arm-none-eabi-size ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME}
  COMMAND cp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXE_NAME} debug.elf
)

add_custom_command(TARGET ${EXE_NAME} PRE_LINK
  COMMAND python ${BOILERPLATE}/src/link/generate-ldscript.py -p ${TARGET_MCU} > ${BOILERPLATE}/src/link/stm32-mem.ld
)

# 'make flash' to upload to board after compiling
add_custom_target(flash
  # try stlink v2 first, then stlink v2-1 if that fails
  openocd -f interface/stlink-v2.cfg -f ${OPENOCD_TARGET} -c "program debug.elf verify reset exit" ||
  openocd -f interface/stlink-v2-1.cfg -f ${OPENOCD_TARGET} -c "program debug.elf verify reset exit"
  DEPENDS ${EXE_NAME}
)


add_custom_command(OUTPUT ${PINGCPP_SRC}/message/ping-message-common.h
  # generate ping-cpp message files
  COMMAND ${PINGCPP_SRC}/generate/generate-message.py --output-dir ${PINGCPP_SRC}/message
  COMMENT "generating ping-cpp messages"
)

add_custom_command(OUTPUT ${PINGCPP_SRC}/message/ping-message-openesc.h
  # generate ping-cpp message files
  COMMAND ${PINGPROTOCOL_SRC}/generator.py --definition ${PROJECT_SOURCE_DIR}/src/util/openesc.json --template ${PINGCPP_SRC}/generate/templates/ping-message-.h.in > ${PINGCPP_SRC}/message/ping-message-openesc.h
  COMMENT "generating openesc messages"
)
