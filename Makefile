# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -g

# Target executable
TARGET = main

# Source files
SRCS = bitboard_gen.cpp engine_setup.cpp evaluation.cpp main.cpp search.cpp

# Object files (from .cpp files)
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to build each .o file from its .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove all generated files
clean:
	rm -f $(OBJS) $(TARGET)
