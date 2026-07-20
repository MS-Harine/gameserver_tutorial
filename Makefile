CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -g

# Automatically find all directories (excluding hidden directories like .git) to add to include paths
INC_DIRS := $(shell find . -type d -not -path '*/.*')
INC_FLAGS := $(addprefix -I, $(INC_DIRS))

# Automatically find all .cpp source files in the project recursively
SRCS := $(shell find . -name "*.cpp" -not -path '*/.*')

# Object directory and object files
OBJDIR = obj
OBJS := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRCS))

# Output directory and target executable
TARGET_DIR = bin
TARGET = $(TARGET_DIR)/gameserver

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(TARGET_DIR)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -o $(TARGET) $(OBJS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET_DIR)

.PHONY: all clean
