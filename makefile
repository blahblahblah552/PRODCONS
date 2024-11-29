# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -lpthread -std=c++20

# Target executable
TARGET = producer-Consumer

# For deleting the target
TARGET_DEL = producer-Consumer

# Source files
SRCS = main.cpp

# Object files
OBJS = $(SRCS:.cpp=.o) 

#clean up junk
JUNK = *.txt pcp.conf

#base command
COMMAND = $(shell ./$(TARGET) > outtext.txt)

# Default rule to build and run the executable
all: $(TARGET)

# Rule to link object files into the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to run the executable
run: $(TARGET)
	$(COMMAND)
	

# Clean rule to remove generated files
clean:
	rm $(TARGET_DEL) $(OBJS) $(JUNK)