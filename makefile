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

# ASAN=1 -- enable address sanitizer
ifeq ($(filter 1, $(ASAN)), 1)
CXXFLAGS += -g -fsanitize=address
endif

# UBSAN=1 -- enable undefined behavior sanitizer
ifeq ($(filter 1, $(UBSAN)), 1)
CXXFLAGS += -g -fsanitize=undefined
endif

# D=1 -- enable debugging mode: log outputs, assertions, etc
ifeq ($(filter 1, $(D)), 1)
CXXFLAGS += -g -ggdb3 -DDEBUGGING
endif

# Create directories if they don't exist
$(OBJFILES): | $(OBJDIR) $(DEPSDIR)

$(OBJDIR):
	@mkdir -p $@
	@echo "[MKDIR]  $@"

$(DEPSDIR):
	@mkdir -p $@
	@echo "[MKDIR]  $@"

# How to make object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	@$(CXX) $(CXXFLAGS) $(DEPCXXFLAGS) $(INC) -o $@ -c $<
	@echo "[CXX]    $@"

# How to make the executable
$(TARGET): $(OBJFILES)
	@$(CXX) $(CXXFLAGS) $(DEPCXXFLAGS) $(INC) -o $@ $^
	@echo "[LINK]   $@"

clean:
	@rm -rf $(OBJDIR) $(DEPSDIR) $(TARGET)
	@echo "[CLEAN]"

.PHONY: all clean
