# ---------- paths ----------
DSRC  := src
DINC  := include
DTEST := tests
DEXE  := bin

EXE       ?= filipbot
TEST_EXE  := testsuite
MAGIC_EXE := gen_magics
TEXEL_EXE := tune

LIBRARIES = libraries
# eigen

EIGEN_DIR := $(LIBRARIES)/Eigen
EIGEN_URL := https://gitlab.com/libeigen/eigen/-/archive/5.0.1/eigen-5.0.1.tar.gz

ARGPARSE_DIR := $(LIBRARIES)/ArgParse
ARGPARSE_URL := https://raw.githubusercontent.com/p-ranav/argparse/refs/heads/master/include/argparse/argparse.hpp

# ---------- build type ----------
type ?= dev
PGO_DIR  := build/pgo
PROFRAW  := $(PGO_DIR)/default.profraw
PROFDATA := $(PGO_DIR)/default.profdata

# Optional PGO flags (set automatically by the 'pgo' target, or manually via pgo=generate / pgo=use)
pgo ?= none
ifeq ($(pgo),generate)
  PGO_FLAGS := -fprofile-generate=$(PROFRAW)
else ifeq ($(pgo),use)
  PGO_FLAGS := -fprofile-use=$(PROFDATA)
else ifeq ($(pgo),none)
  PGO_FLAGS :=
else
  $(error Invalid pgo '$(pgo)'. Must be none, generate, or use)
endif

ifeq ($(type),release)
  FLAGS := -Wall -std=c++23 -O3 -DNDEBUG -march=native -fopenmp $(PGO_FLAGS)
else ifeq ($(type),pgo)
  FLAGS := -Wall -std=c++23 -O3 -DNDEBUG -march=native -fopenmp -fprofile-generate=$(PROFRAW)
else ifeq ($(type),dev)
  FLAGS := -DDEBUG -g -Wall -std=c++23 -O2 -march=native -fsanitize=address
else ifeq ($(type),debug)
  FLAGS := -DDEBUG -g -Wall -std=c++23 -O0 -march=native -fsanitize=address
else ifeq ($(type),perft)
  FLAGS := -Wall -std=c++23 -O2 -g -DNDEBUG -march=native
else
  $(error Invalid type '$(type)'. Must be release, pgo, dev, debug or perft)
endif

# Separate object dir per build type, so switching type never mixes objects
ifeq ($(pgo),none)
  DOBJ := build/$(type)
else
  DOBJ := build/$(type)-$(pgo)
endif

# ---------- parallelism ----------
MAKEFLAGS += -j 2

# ---------- toolchain ----------
CXX      := clang++
PROFTOOL := $(shell \
    command -v llvm-profdata 2>/dev/null || \
    command -v llvm-profdata-$$($(CXX) -dumpversion | cut -d. -f1) 2>/dev/null || \
    ls -1 /usr/lib/llvm-*/bin/llvm-profdata /usr/bin/llvm-profdata-* /opt/homebrew/opt/llvm/bin/llvm-profdata 2>/dev/null | sort -V | tail -n1 \
)
INC_DIRS := $(shell find $(DINC) -type d)
CPPFLAGS := $(addprefix -I,$(INC_DIRS)) -MMD -MP
CXXFLAGS := $(FLAGS)
LDFLAGS  := $(FLAGS)
LIBS     := -lgtest -lgtest_main -pthread

# ---------- source discovery ----------
MAIN_SRC  := $(DSRC)/main.cpp
MAGIC_SRC := $(DSRC)/gen_magic_nums.cpp
TEXEL_SRC := $(shell find $(DSRC)/texel -name '*.cpp' 2>/dev/null)

ALL_SRC   := $(shell find $(DSRC) -name '*.cpp')
CORE_SRC  := $(filter-out $(MAIN_SRC) $(MAGIC_SRC) $(TEXEL_SRC),$(ALL_SRC))
TEST_SRC  := $(shell find $(DTEST) -name '*.cpp')

# src/a/b.cpp -> build/<type>/src/a/b.o
to_obj = $(patsubst %.cpp,$(DOBJ)/%.o,$(1))

CORE_OBJ  := $(call to_obj,$(CORE_SRC))
MAIN_OBJ  := $(call to_obj,$(MAIN_SRC))
MAGIC_OBJ := $(call to_obj,$(MAGIC_SRC))
TEXEL_OBJ := $(call to_obj,$(TEXEL_SRC))
TEST_OBJ  := $(call to_obj,$(TEST_SRC))

ALL_OBJ := $(CORE_OBJ) $(MAIN_OBJ) $(MAGIC_OBJ) $(TEXEL_OBJ) $(TEST_OBJ)

# add eigen include only to texel objects.
$(TEXEL_OBJ): CPPFLAGS += -I$(EIGEN_DIR) -I$(ARGPARSE_DIR) -fpermissive
$(TEXEL_OBJ): | $(EIGEN_DIR) $(ARGPARSE_DIR)

$(EIGEN_DIR):
	@echo "Downloading Eigen header-only library..."
	@mkdir -p libraries
	@if command -v curl >/dev/null; then \
		curl -L $(EIGEN_URL) -o libraries/eigen.tar.gz; \
	elif command -v wget >/dev/null; then \
		wget $(EIGEN_URL) -O libraries/eigen.tar.gz; \
	else \
		echo "Error: Neither curl nor wget found." >&2; exit 1; \
	fi
	@tar -xzf libraries/eigen.tar.gz -C libraries/
	@mv libraries/eigen-* $(EIGEN_DIR)
	@rm libraries/eigen.tar.gz
	@echo "Eigen downloaded successfully."

$(ARGPARSE_DIR):
	@echo "Downloading argparse library..."
	@mkdir -p $(ARGPARSE_DIR)/argparse
	@if command -v curl >/dev/null; then \
		curl -L $(ARGPARSE_URL) -o $(ARGPARSE_DIR)/argparse/argparse.hpp;\
	elif command -v wget >/dev/null; then \
		wget $(ARGPARSE_URL) -o $(ARGPARSE_DIR)/argparse/argparse.hpp;\
	else \
		echo "Error: Neither curl nor wget found" >&2; exit 1; \
	fi
	@echo "Argparse downloaded successfully."

# ---------- targets ----------
.PHONY: all test runtest magic tune run clean FORCE pgo

all: $(DEXE)/$(EXE)

# ---- link into build/<type>/ ----
$(DOBJ)/$(EXE): $(CORE_OBJ) $(MAIN_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(DOBJ)/$(MAGIC_EXE): $(CORE_OBJ) $(MAGIC_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(DOBJ)/$(TEXEL_EXE): $(CORE_OBJ) $(TEXEL_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(DOBJ)/$(TEST_EXE): $(CORE_OBJ) $(TEST_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

# ---- copy into bin/ (FORCE makes it copy every time) ----
$(DEXE)/%: $(DOBJ)/% FORCE
	@mkdir -p $(DEXE)
	@cmp -s $< $@ || cp -f $< $@

FORCE:

# One rule compiles everything (src, src/subdirs, tests, tests/subdirs)
$(DOBJ)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Header dependency tracking
-include $(ALL_OBJ:.o=.d)

# ---------- PGO release workflow ----------
pgo:
	@test -n "$(PROFTOOL)" || { echo "Error: llvm-profdata not found." >&2; exit 1; }
	@echo "=== [1/4] Building instrumented binary ==="
	@rm -rf $(PGO_DIR) build/release-use
	@$(MAKE) type=pgo all
	@echo "=== [2/4] Running self-play workload to collect profile ==="
	@printf "position startpos\nself 100\nquit\n" | LLVM_PROFILE_FILE="$(PGO_DIR)/default_%p.profraw" ./$(DEXE)/$(EXE) > /dev/null
	@echo "=== [3/4] Merging raw profile data ==="
	@$(PROFTOOL) merge -output=$(PROFDATA) $(PGO_DIR)/*.profraw
	@echo "=== [4/4] Recompiling optimized release binary ==="
	@$(MAKE) type=release pgo=use all
	@echo "=== PGO Release build complete: $(DEXE)/$(EXE) ==="

# ---------- convenience ----------
filter ?= *

test: $(DEXE)/$(TEST_EXE)
magic: $(DEXE)/$(MAGIC_EXE)
tune: $(DEXE)/$(TEXEL_EXE)

clean:
	rm -rf build $(DEXE)/$(EXE) $(DEXE)/$(TEST_EXE) $(DEXE)/$(MAGIC_EXE) $(DEXE)/$(TEXEL_EXE) libraries