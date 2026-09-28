ARM_CXX ?= arm-none-eabi-g++
NATIVE_CXX ?= c++
JSON_INCLUDE ?= $(if $(filter Darwin,$(shell uname -s)),/opt/homebrew/include,)
JSON_FLAGS = $(if $(JSON_INCLUDE),-isystem $(JSON_INCLUDE),)
COMMON = -std=c++17 -Wall -Wextra -Werror -I./distingNT_API/include -I./src
ARM_FLAGS = -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -mno-unaligned-access -Os -fPIC -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables

all: hardware
hardware: plugins/patch_helper.o
build/native_text_probe.o: tools/native_text_probe.cpp distingNT_API/include/distingnt/api.h Makefile
	mkdir -p build
	$(ARM_CXX) $(COMMON) $(ARM_FLAGS) -c $< -o $@

native-text-probe: build/native_text_probe.o
plugins/patch_helper.o: src/plugin.cpp src/patch_map.h src/patch_protocol.h distingNT_API/include/distingnt/api.h Makefile
	mkdir -p plugins
	$(ARM_CXX) $(COMMON) $(ARM_FLAGS) -c $< -o $@

build/tests: tests/test_plugin.cpp tests/json_adapter.h src/plugin.cpp src/patch_map.h src/patch_protocol.h
	mkdir -p build
	$(NATIVE_CXX) $(COMMON) $(JSON_FLAGS) -g -fsanitize=address,undefined tests/test_plugin.cpp src/plugin.cpp -o $@
test: build/tests
	./build/tests tests/fixtures/native-map.json

inspect: hardware
	python3 tools/inspect_object.py plugins/patch_helper.o
	arm-none-eabi-size -A plugins/patch_helper.o
	arm-none-eabi-nm -u -C plugins/patch_helper.o
build/arm_startup.o: tests/arm_startup.cpp distingNT_API/include/distingnt/api.h Makefile
	mkdir -p build
	$(ARM_CXX) $(COMMON) $(ARM_FLAGS) -c $< -o $@
arm-smoke: hardware build/arm_startup.o
	python3 tools/arm_startup_check.py plugins/patch_helper.o build/arm_startup.o
verify: test inspect arm-smoke
static-check:
	cppcheck --enable=warning,style,performance,portability --std=c++17 --suppress=missingIncludeSystem --suppress='uninitMemberVarNoCtor:distingNT_API/include/distingnt/api.h' --suppress='noExplicitConstructor:distingNT_API/include/distingnt/serialisation.h' --error-exitcode=1 -I src -I distingNT_API/include src
.PHONY: all hardware test inspect arm-smoke verify static-check native-text-probe
