# Compiler flags
CXX := g++
CXXFLAGS := -W -Wall -Wextra -Werror -Wshadow -std=c++17

# Build target
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

# Experiment source files
EXPSRCDIR := exp
EXPSRCFILES := $(wildcard $(EXPSRCDIR)/*.cc)

# Object files
OBJDIR := obj
OBJFILES := $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o, $(SRCFILES))

# Experiment object files
EXPOBJDIR := obj
EXPOBJFILES := $(patsubst $(EXPSRCDIR)/%.cc, $(EXPOBJDIR)/%.o, $(EXPSRCFILES))

# Executables
TARGET := $(patsubst $(EXPSRCDIR)/%.cc, %, $(EXPSRCFILES))
all: $(TARGET)

# ASAN=1 -- enable address sanitizer
ifeq ($(filter 1, $(ASAN)), 1)
CXXFLAGS += -g -fsanitize=address
endif

# UBSAN=1 -- enable undefined behavior sanitizer
ifeq ($(filter 1, $(UBSAN)), 1)
CXXFLAGS += -g -fsanitize=undefined
endif

# D=1 -- enable debugging mode: debugging simbols, assertions, etc
ifeq ($(filter 1, $(D)), 1)
CXXFLAGS += -g -ggdb3 -DDEBUGGING -O0
else
CXXFLAGS += -O2
endif

# V=1 -- enable verbose mode
ifeq ($(filter 1, $(V)), 1)
CXXFLAGS += -DVERBOSE
endif

# Create directories if they don't exist
$(OBJFILES): | $(OBJDIR) $(DEPSDIR)
$(EXPOBJFILES): | $(OBJDIR) $(DEPSDIR)

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

# How to make experiment object files
$(EXPOBJDIR)/%.o: $(EXPSRCDIR)/%.cc
	@$(CXX) $(CXXFLAGS) $(DEPCXXFLAGS) $(INC) -o $@ -c $<
	@echo "[CXX]    $@"

# How to make experiment executables
exp-%: $(EXPOBJDIR)/exp-%.o $(OBJFILES)
	@$(CXX) $(CXXFLAGS) $(DEPCXXFLAGS) $(INC) -o $@ $^
	@echo "[LINK]   $@"

clean:
	@rm -rf $(OBJDIR) $(DEPSDIR) $(TARGET) $(EXPOBJDIR)
	@echo "[CLEAN]"

.PHONY: all clean
