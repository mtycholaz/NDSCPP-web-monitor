# Compiler settings
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -O2
INCLUDES = -I.
LDFLAGS =

# Libraries needed
LIBS = -lgtest -lgtest_main -lcpr -pthread 

# Binary name
TARGET = tests

# Source files
SOURCES = tests.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Detect platform
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    # macOS-specific settings using Homebrew
    INCLUDES += -I$(shell brew --prefix)/include/
    LDFLAGS += -L$(shell brew --prefix)/lib/
    
    # Add common Homebrew installation paths for required libraries
    INCLUDES += -I$(shell brew --prefix googletest)/include/
    INCLUDES += -I$(shell brew --prefix nlohmann-json)/include/
    INCLUDES += -I$(shell brew --prefix cpr)/include/
    
    LDFLAGS += -L$(shell brew --prefix googletest)/lib/
    LDFLAGS += -L$(shell brew --prefix cpr)/lib/
endif

# Default target
all: $(TARGET)

# Link the target binary
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET) $(LIBS)

# Compile source files
%.o: %.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -f $(OBJECTS) $(TARGET)

# Run the tests
test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET)

# Install dependencies on macOS
install-deps-mac:
	@echo "Installing dependencies via Homebrew..."
	@brew install googletest
	@brew install cpr
	@brew install nlohmann-json

.PHONY: all clean test install-deps-mac