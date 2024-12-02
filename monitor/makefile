# Compiler settings
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -O2
INCLUDES = -I.

# Libraries needed
LIBS = -lncurses -lcurl

# Binary name
TARGET = ledmon

# Source files
SOURCES = main.cpp content.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Link the target binary
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)

# Compile source files
%.o: %.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
