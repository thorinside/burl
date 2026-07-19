CXX ?= c++
ARM_CXX ?= arm-none-eabi-g++
ARM_NM ?= arm-none-eabi-nm
ARM_SIZE ?= arm-none-eabi-size
NTPUSH ?= ntpush
API_DIR := distingNT_API
CPPFLAGS := -Iinclude
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror
SANITIZER_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
HARDWARE_FLAGS := -std=c++11 -Os -Wall -Wextra -Wpedantic -Werror \
	-fPIC -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions \
	-fno-unwind-tables -fno-asynchronous-unwind-tables \
	-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
HARDWARE_LDFLAGS := -Wl,--relocatable -nostdlib
PLUGIN_CPPFLAGS := $(CPPFLAGS) -I$(API_DIR)/include

BUILD_DIR := build
HARDWARE_BUILD_DIR := $(BUILD_DIR)/hardware
PLUGIN_OUTPUT := plugins/Burl.o
PLUGIN_SOURCES := src/plugin.cpp src/pattern_generator.cpp src/filter.cpp src/voice.cpp
PLUGIN_OBJECTS := $(patsubst %.cpp,$(HARDWARE_BUILD_DIR)/%.o,$(PLUGIN_SOURCES))
PLUGIN_DEPS := $(PLUGIN_OBJECTS:.o=.d)
PATTERN_TEST_BINARY := $(BUILD_DIR)/pattern_generator_test
PATTERN_TEST_SOURCES := src/pattern_generator.cpp tests/pattern_generator_test.cpp
DETERMINISM_TEST_BINARY := $(BUILD_DIR)/determinism_test
DETERMINISM_TEST_SOURCES := src/pattern_generator.cpp tests/determinism_test.cpp
FILTER_TEST_BINARY := $(BUILD_DIR)/filter_test
FILTER_TEST_SOURCES := src/filter.cpp tests/filter_test.cpp
VOICE_DETERMINISM_TEST_BINARY := $(BUILD_DIR)/voice_determinism_test
VOICE_DETERMINISM_TEST_SOURCES := src/pattern_generator.cpp src/filter.cpp src/voice.cpp tests/voice_determinism_test.cpp
VOICE_PWM_TEST_BINARY := $(BUILD_DIR)/voice_pwm_test
VOICE_PWM_TEST_SOURCES := src/pattern_generator.cpp src/filter.cpp src/voice.cpp tests/voice_pwm_test.cpp
VOICE_SOURCE_ROUTING_TEST_BINARY := $(BUILD_DIR)/voice_source_routing_test
VOICE_SOURCE_ROUTING_TEST_SOURCES := src/pattern_generator.cpp src/filter.cpp src/voice.cpp tests/voice_source_routing_test.cpp
VOICE_RESET_TEST_BINARY := $(BUILD_DIR)/voice_reset_test
VOICE_RESET_TEST_SOURCES := src/pattern_generator.cpp src/filter.cpp src/voice.cpp tests/voice_reset_test.cpp
VOICE_STRESS_TEST_BINARY := $(BUILD_DIR)/voice_stress_test
VOICE_STRESS_TEST_SOURCES := src/pattern_generator.cpp src/filter.cpp src/voice.cpp tests/voice_stress_test.cpp
VOICE_STRESS_SANITIZER_BINARY := $(BUILD_DIR)/voice_stress_test_sanitize
PLUGIN_TEST_BINARY := $(BUILD_DIR)/plugin_integration_test
PLUGIN_TEST_SOURCES := tests/plugin_integration_test.cpp $(PLUGIN_SOURCES)
RELEASE_BRANDING_TEST_BINARY := $(BUILD_DIR)/release_branding_test
RELEASE_BRANDING_TEST_SOURCES := tests/release_branding_test.cpp

.PHONY: all test stress-sanitize hardware check check-allocations size branding-check verify push clean

all: test hardware

test: $(PATTERN_TEST_BINARY) $(DETERMINISM_TEST_BINARY) $(FILTER_TEST_BINARY) $(VOICE_DETERMINISM_TEST_BINARY) $(VOICE_PWM_TEST_BINARY) $(VOICE_SOURCE_ROUTING_TEST_BINARY) $(VOICE_RESET_TEST_BINARY) $(VOICE_STRESS_TEST_BINARY) $(PLUGIN_TEST_BINARY)
	./$(PATTERN_TEST_BINARY)
	./$(DETERMINISM_TEST_BINARY)
	./$(FILTER_TEST_BINARY)
	./$(VOICE_DETERMINISM_TEST_BINARY)
	./$(VOICE_PWM_TEST_BINARY)
	./$(VOICE_SOURCE_ROUTING_TEST_BINARY)
	./$(VOICE_RESET_TEST_BINARY)
	./$(VOICE_STRESS_TEST_BINARY)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./$(PLUGIN_TEST_BINARY)

stress-sanitize: $(VOICE_STRESS_SANITIZER_BINARY)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./$(VOICE_STRESS_SANITIZER_BINARY)

$(PATTERN_TEST_BINARY): $(PATTERN_TEST_SOURCES) include/burl/pattern_generator.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PATTERN_TEST_SOURCES) -o $@

$(DETERMINISM_TEST_BINARY): $(DETERMINISM_TEST_SOURCES) include/burl/pattern_generator.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DETERMINISM_TEST_SOURCES) -o $@

$(FILTER_TEST_BINARY): $(FILTER_TEST_SOURCES) include/burl/filter.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(FILTER_TEST_SOURCES) -o $@

$(VOICE_DETERMINISM_TEST_BINARY): $(VOICE_DETERMINISM_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/filter.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_DETERMINISM_TEST_SOURCES) -o $@

$(VOICE_PWM_TEST_BINARY): $(VOICE_PWM_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/filter.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_PWM_TEST_SOURCES) -o $@

$(VOICE_SOURCE_ROUTING_TEST_BINARY): $(VOICE_SOURCE_ROUTING_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/filter.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_SOURCE_ROUTING_TEST_SOURCES) -o $@

$(VOICE_RESET_TEST_BINARY): $(VOICE_RESET_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/filter.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_RESET_TEST_SOURCES) -o $@

$(VOICE_STRESS_TEST_BINARY): $(VOICE_STRESS_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/filter.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_STRESS_TEST_SOURCES) -o $@

$(VOICE_STRESS_SANITIZER_BINARY): $(VOICE_STRESS_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/filter.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SANITIZER_FLAGS) $(VOICE_STRESS_TEST_SOURCES) -o $@

$(PLUGIN_TEST_BINARY): $(PLUGIN_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/filter.hpp include/burl/voice.hpp $(API_DIR)/include/distingnt/api.h | $(BUILD_DIR)
	$(CXX) $(PLUGIN_CPPFLAGS) $(CXXFLAGS) $(SANITIZER_FLAGS) $(PLUGIN_TEST_SOURCES) -o $@

$(RELEASE_BRANDING_TEST_BINARY): $(RELEASE_BRANDING_TEST_SOURCES) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(RELEASE_BRANDING_TEST_SOURCES) -o $@

$(HARDWARE_BUILD_DIR)/%.o: %.cpp include/burl/pattern_generator.hpp include/burl/filter.hpp include/burl/voice.hpp $(API_DIR)/include/distingnt/api.h
	@mkdir -p $(@D)
	$(ARM_CXX) $(PLUGIN_CPPFLAGS) $(HARDWARE_FLAGS) -MMD -MP -c -o $@ $<

$(PLUGIN_OUTPUT): $(PLUGIN_OBJECTS)
	@mkdir -p $(@D)
	$(ARM_CXX) $(HARDWARE_FLAGS) $(HARDWARE_LDFLAGS) -o $@ $^

hardware: $(PLUGIN_OUTPUT)

check: hardware
	@echo "Undefined symbols supplied by the disting NT host/runtime:"
	@$(ARM_NM) -u $(PLUGIN_OUTPUT)

check-allocations: hardware
	@symbols="$$($(ARM_NM) -u $(PLUGIN_OUTPUT) | awk '{ print $$NF }' | grep -E '^(malloc|calloc|realloc|free|_Zn[aw].*|_Zd[al].*)$$' || true)"; \
	if [ -n "$$symbols" ]; then \
		echo "Forbidden dynamic-allocation symbols:"; \
		echo "$$symbols"; \
		exit 1; \
	fi
	@echo "No dynamic-allocation symbols referenced by $(PLUGIN_OUTPUT)"

size: hardware
	$(ARM_SIZE) -A $(PLUGIN_OUTPUT)

branding-check: $(RELEASE_BRANDING_TEST_BINARY) hardware
	./$(RELEASE_BRANDING_TEST_BINARY)

verify: test stress-sanitize hardware check check-allocations size branding-check

push: hardware
	$(NTPUSH) $(PLUGIN_OUTPUT)

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) plugins

-include $(PLUGIN_DEPS)
