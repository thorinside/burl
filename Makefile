CXX ?= c++
CPPFLAGS := -Iinclude
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror

BUILD_DIR := build
PATTERN_TEST_BINARY := $(BUILD_DIR)/pattern_generator_test
PATTERN_TEST_SOURCES := src/pattern_generator.cpp tests/pattern_generator_test.cpp
DETERMINISM_TEST_BINARY := $(BUILD_DIR)/determinism_test
DETERMINISM_TEST_SOURCES := src/pattern_generator.cpp tests/determinism_test.cpp
VOICE_DETERMINISM_TEST_BINARY := $(BUILD_DIR)/voice_determinism_test
VOICE_DETERMINISM_TEST_SOURCES := src/pattern_generator.cpp src/voice.cpp tests/voice_determinism_test.cpp

.PHONY: all test clean

all: test

test: $(PATTERN_TEST_BINARY) $(DETERMINISM_TEST_BINARY) $(VOICE_DETERMINISM_TEST_BINARY)
	./$(PATTERN_TEST_BINARY)
	./$(DETERMINISM_TEST_BINARY)
	./$(VOICE_DETERMINISM_TEST_BINARY)

$(PATTERN_TEST_BINARY): $(PATTERN_TEST_SOURCES) include/burl/pattern_generator.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PATTERN_TEST_SOURCES) -o $@

$(DETERMINISM_TEST_BINARY): $(DETERMINISM_TEST_SOURCES) include/burl/pattern_generator.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DETERMINISM_TEST_SOURCES) -o $@

$(VOICE_DETERMINISM_TEST_BINARY): $(VOICE_DETERMINISM_TEST_SOURCES) include/burl/pattern_generator.hpp include/burl/voice.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(VOICE_DETERMINISM_TEST_SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
