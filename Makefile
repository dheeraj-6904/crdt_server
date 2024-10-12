# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -I/usr/include/boost
LDFLAGS = -lboost_system -lboost_thread

# Executable name
TARGET = server

# Source files and object files
SRCS = Server.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
