# paths
DSRC = ./src
DINC = ./include
DOBJ = ./build
DEXE = ./bin
DTEST = ./tests

EXE?=filipbot
TEST_EXE = testsuite
MAGIC_EXE = gen_magics

# compile in parallel
NPROCS := $(shell grep -c 'processor' /proc/cpuinfo)
MAKEFLAGS += -j4

type ?= dev
ifeq ($(type), release)
    FLAGS = -Wall -std=c++23 -O3 -DNDEBUG -march=native
else ifeq ($(type), dev)
    FLAGS = -DDEBUG -g -Wall -std=c++23 -Ofast -march=native -fsanitize=address
else ifeq ($(type), debug)
    FLAGS = -DDEBUG -g -Wall -std=c++23 -O0 -march=native -fsanitize=address
else ifeq ($(type), perft)
    FLAGS = -Wall -std=c++23 -O2 -g -DNDEBUG -march=native
else
    $(error Invalid value for type: '$(type)'. Must be 'release', 'dev' or 'perft'.)
endif

LIBS = -lgtest -lgtest_main -pthread  # Google Test and pthread libs

# Commands for linking.
CCL = g++ $(FLAGS) -o
# Commands for compilation (removed redundant -c here)
CC = g++ $(FLAGS) -MMD -MP -c

# objects
OBJECTS = $(DOBJ)/uci_interface.o $(DOBJ)/piece.o $(DOBJ)/board.o $(DOBJ)/game.o $(DOBJ)/notation_interface.o $(DOBJ)/bitboard.o $(DOBJ)/movegen.o $(DOBJ)/movegen_benchmark.o $(DOBJ)/time_manager.o $(DOBJ)/eval.o $(DOBJ)/moveorder.o $(DOBJ)/tables.o $(DOBJ)/config_parser.o $(DOBJ)/history.o
MAIN_OBJ = $(DOBJ)/main.o
MAGIC_OBJ = $(DOBJ)/gen_magic_nums.o
TEST_OBJECTS = $(DOBJ)/piece_test.o $(DOBJ)/board_test.o $(DOBJ)/interface_test.o $(DOBJ)/board_state_test.o $(DOBJ)/bitboard_test.o $(DOBJ)/movegen_test.o $(DOBJ)/time_manager_test.o $(DOBJ)/tables_test.o $(DOBJ)/config_parser_test.o

# Target
all: $(DEXE)/$(EXE)

# Link the main executable
$(DEXE)/$(EXE): $(OBJECTS) $(MAIN_OBJ) 
	$(CCL) $@ $(MAIN_OBJ) $(OBJECTS) $(LIBS)

$(DEXE)/$(MAGIC_EXE): $(OBJECTS) $(MAGIC_OBJ) 
	$(CCL) $@ $(MAGIC_OBJ) $(OBJECTS) $(LIBS)

# Test executable target
$(DEXE)/$(TEST_EXE): $(TEST_OBJECTS) $(OBJECTS)
	$(CCL) $@ $(TEST_OBJECTS) $(OBJECTS) $(LIBS)

# Compile test object files (from ./tests folder)
$(DOBJ)/%.o: $(DTEST)/%.cpp Makefile
	$(CC) -I$(DINC) $< -o $@

# Compile source object files (from ./src folder - includes main.cpp and gen_magic_nums.cpp)
$(DOBJ)/%.o: $(DSRC)/%.cpp Makefile 
	$(CC) -I$(DINC) $< -o $@

# Include dependency files for ALL objects
DEPS := $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(MAIN_OBJ:.o=.d) $(MAGIC_OBJ:.o=.d)
-include $(DEPS)

build:
	mkdir -p $(DOBJ)

bin:
	mkdir -p $(DEXE)

# Clean up build artifacts safely
clean:
	rm -rf $(DOBJ)/* $(DEXE)/$(EXE) $(DEXE)/$(TEST_EXE) $(DEXE)/$(MAGIC_EXE)

# Run the main executable
run:
	cd $(DEXE); \
	./$(EXE)

# Compile tests
test: $(DEXE)/$(TEST_EXE)

filter ?= "*"
# runtests
runtest: $(DEXE)/$(TEST_EXE)
	./$(DEXE)/$(TEST_EXE) --gtest_filter="$(filter)"

magic: $(DEXE)/$(MAGIC_EXE)
	./$(DEXE)/$(MAGIC_EXE)