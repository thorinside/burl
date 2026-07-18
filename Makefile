CXX ?= c++
CPPFLAGS := -Iinclude
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror

BUILD_DIR := build
TEST_BINARY := $(BUILD_DIR)/pattern_generator_test
TEST_SOURCES := src/pattern_generator.cpp tests/pattern_generator_test.cpp

.PHONY: all test clean

all: test

test: $(TEST_BINARY)
	./$(TEST_BINARY)

$(TEST_BINARY): $(TEST_SOURCES) include/burl/pattern_generator.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TEST_SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
