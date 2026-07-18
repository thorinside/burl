CXX ?= c++
CPPFLAGS := -Iinclude
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror
SANITIZER_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

BUILD_DIR := build
PATTERN_TEST_BINARY := $(BUILD_DIR)/pattern_generator_test
PATTERN_TEST_SOURCES := src/pattern_generator.cpp tests/pattern_generator_test.cpp
DETERMINISM_TEST_BINARY := $(BUILD_DIR)/determinism_test
DETERMINISM_TEST_SOURCES := src/pattern_generator.cpp tests/determinism_test.cpp
VOICE_DETERMINISM_TEST_BINARY := $(BUILD_DIR)/voice_determinism_test
VOICE_DETERMINISM_TEST_SOURCES := src/pattern_generator.cpp src/voice.cpp tests/voice_determinism_test.cpp
VOICE_PWM_TEST_BINARY := $(BUILD_DIR)/voice_pwm_test
VOICE_PWM_TEST_SOURCES := src/pattern_generator.cpp src/voice.cpp tests/voice_pwm_test.cpp
VOICE_SOURCE_ROUTING_TEST_BINARY := $(BUILD_DIR)/voice_source_routing_test
VOICE_SOURCE_ROUTING_TEST_SOURCES := src/pattern_generator.cpp src/voice.cpp tests/voice_source_routing_test.cpp
VOICE_RESET_TEST_BINARY := $(BUILD_DIR)/voice_reset_test
VOICE_RESET_TEST_SOURCES := src/pattern_generator.cpp src/voice.cpp tests/voice_reset_test.cpp
VOICE_STRESS_TEST_BINARY := $(BUILD_DIR)/voice_stress_test
VOICE_STRESS_TEST_SOURCES := src/pattern_generator.cpp src/voice.cpp tests/voice_stress_test.cpp
VOICE_STRESS_SANITIZER_BINARY := $(BUILD_DIR)/voice_stress_test_sanitize

.PHONY: all test stress-sanitize clean

all: test

test: $(PATTERN_TEST_BINARY) $(DETERMINISM_TEST_BINARY) $(VOICE_DETERMINISM_TEST_BINARY) $(VOICE_PWM_TEST_BINARY) $(VOICE_SOURCE_ROUTING_TEST_BINARY) $(VOICE_RESET_TEST_BINARY) $(VOICE_STRESS_TEST_BINARY)
	./$(PATTERN_TEST_BINARY)
	./$(DETERMINISM_TEST_BINARY)
	./$(VOICE_DETERMINISM_TEST_BINARY)
	./$(VOICE_PWM_TEST_BINARY)
	./$(VOICE_SOURCE_ROUTING_TEST_BINARY)
	./$(VOICE_RESET_TEST_BINARY)
	./$(VOICE_STRESS_TEST_BINARY)

stress-sanitize: $(VOICE_STRESS_SANITIZER_BINARY)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./$(VOICE_STRESS_SANITIZER_BINARY)

$(PATTERN_TEST_BINARY): $(PATTERN_TEST_SOURCES) include/burl/pattern_generator.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PATTERN_TEST_SOURCES) -o $@

$(DETERMINISM_TEST_BINARY): $(DETERMINISM_TEST_SOURCES) include/burl/pattern_generator.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DETERMINISM_TEST_SOURCES) -o $@

$(VOICE_DETERMINISM_TEST_BINARY): $(VOICE_DETERMINISM_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_DETERMINISM_TEST_SOURCES) -o $@

$(VOICE_PWM_TEST_BINARY): $(VOICE_PWM_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_PWM_TEST_SOURCES) -o $@

$(VOICE_SOURCE_ROUTING_TEST_BINARY): $(VOICE_SOURCE_ROUTING_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_SOURCE_ROUTING_TEST_SOURCES) -o $@

$(VOICE_RESET_TEST_BINARY): $(VOICE_RESET_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_RESET_TEST_SOURCES) -o $@

$(VOICE_STRESS_TEST_BINARY): $(VOICE_STRESS_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_STRESS_TEST_SOURCES) -o $@

$(VOICE_STRESS_SANITIZER_BINARY): $(VOICE_STRESS_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SANITIZER_FLAGS) $(VOICE_STRESS_TEST_SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
