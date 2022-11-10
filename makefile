# Compiler flags
CXX := g++
CXXFLAGS := -W -Wall -Wextra -Werror -O2 -std=c++17

# Build target
TARGET := main
all: $(TARGET)

# Dependencies
DEPSDIR := .deps
DEPFILES := $(wildcard $(DEPSDIR)/*.d)
include $(DEPFILES)
DEPCXXFLAGS = -MD -MF $(DEPSDIR)/$(@F).d -MP

# Header files
INC := -Iinclude

# Source files
SRCDIR := src
SRCFILES := $(wildcard $(SRCDIR)/*.cc)

# Object files
OBJDIR := obj
OBJFILES := $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o, $(SRCFILES))

# Create directories if they don't exist
$(OBJFILES): | $(OBJDIR) $(DEPSDIR)

$(OBJDIR):
	mkdir -p $@

$(DEPSDIR):
	mkdir -p $@

# How to make object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CXX) $(CXXFLAGS) $(DEPCXXFLAGS) $(INC) -o $@ -c $<

# How to make the executable
$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(DEPCXXFLAGS) $(INC) -o $@ $^
