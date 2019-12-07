# target board
if(NOT DEFINED TARGET_BOARD)
  set(TARGET_BOARD "wraith32")
  message("no TARGET_BOARD defined, using ${TARGET_BOARD}")
endif()

# target board directory
set(TARGET_DIR "${PROJECT_SOURCE_DIR}/src/target")

set(TARGET_SRC ${TARGET_DIR}/${TARGET_BOARD}/target.c)

# include target board CMakeLists
include(${TARGET_DIR}/${TARGET_BOARD}/target-mcu.cmake)

# target part
# TODO fail if not set
message("TARGET_MCU: ${TARGET_MCU}")

include(${BOILERPLATE}/src/target-mcu.cmake)

# target architecture
# TODO fail if not set
message("TARGET_CPU: ${TARGET_CPU}")

# include target board defintions
include_directories(${TARGET_DIR}/${TARGET_BOARD})

if(TARGET_MCU MATCHES "STM32.*")
    include_directories(${TARGET_DIR}/inc/stm32/common)
    if(TARGET_MCU MATCHES "STM32F0.*")
        include_directories(${TARGET_DIR}/inc/stm32/f0)
        set(ISR_SRC ${TARGET_DIR}/inc/stm32/f0/isr.c)
    elseif(TARGET_MCU MATCHES "STM32F3.*")
        include_directories(${TARGET_DIR}/inc/stm32/f3)
        set(ISR_SRC ${TARGET_DIR}/inc/stm32/f3/isr.c)
    elseif(TARGET_MCU MATCHES "STM32G4.*")
        include_directories(${TARGET_DIR}/inc/stm32/g4)
        set(ISR_SRC ${TARGET_DIR}/inc/stm32/g4/isr.c)
    else()
        message("unsupported TARGET_MCU: ${TARGET_MCU}")
    endif()
else()
    message("unsupported TARGET_MCU: ${TARGET_MCU}")
endif()
