STM32DIR = lib/stm32-lib
BOARD = gsc
TARGET_DIR = target/$(BOARD)
OPTIMIZE = -O0

BUILD_DIR ?= build
SUPPORT_DIR = lib/support
INCLUDES += -I $(SUPPORT_DIR)
CXX_SRC += $(wildcard $(SUPPORT_DIR)/*.cpp)
CXX_SRC += $(wildcard src/*.cpp)

include $(STM32DIR)/Makefile

main: $(TARGET_OBJS) $(OBJ_DIR)/src/main/main.opp
	@echo "linking $@"
	@mkdir -p $(BIN_DIR)
ifneq (,$(FLASH_OVERRIDE))
	@python $(STM32DIR)/src/link/generate-ldscript.py -p $(TARGET_MCU) -o$(FLASH_OVERRIDE) > $(STM32DIR)/src/link/stm32-mem.ld
else
	@python $(STM32DIR)/src/link/generate-ldscript.py -p $(TARGET_MCU) > $(STM32DIR)/src/link/stm32-mem.ld
endif
	@arm-none-eabi-g++ -o $(BIN_DIR)/$@.elf $^ -T $(STM32DIR)/src/link/stm32-mem.ld -T $(LD_SRC) $(LD_FLAGS)
	arm-none-eabi-size $(BIN_DIR)/$@.elf
	@cp $(BIN_DIR)/$@.elf $(BIN_DIR)/debug.elf
	@arm-none-eabi-objcopy -O ihex $(BIN_DIR)/$@.elf $(BIN_DIR)/$@.hex
	@cp $(BIN_DIR)/$@.hex $(BIN_DIR)/debug.hex
	@arm-none-eabi-objcopy -O binary $(BIN_DIR)/$@.elf $(BIN_DIR)/$@.bin
