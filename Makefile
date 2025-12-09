# paths
DSRC = ./src
DINC = ./include
DOBJ = ./build
DEXE = ./bin
DTEST = ./tests

EXE?=filipbot
TEST_EXE = testsuite
MAGIC_EXE = gen_magics

#compile in parallel
NPROCS:= $(SHELL grep -c 'processor' /proc/cpuinfo)
MAKEFLAGS+= -j$(NPROCS)

type?=dev
ifeq ($(type), release)
	FLAGS = -Wall -std=c++23 -O3
else
	FLAGS = -DDEBUG -g -Wall -std=c++23 -Ofast
endif


LIBS = -lgtest -lgtest_main -pthread  # Google Test and pthread libs


# commands for linking.
CCL = g++ -o
# Commands for compilation.
CC = g++ $(FLAGS) -MMD -MP -c

# objects
OBJECTS = $(DOBJ)/uci_interface.o $(DOBJ)/piece.o $(DOBJ)/board.o $(DOBJ)/game.o $(DOBJ)/notation_interface.o $(DOBJ)/bitboard.o $(DOBJ)/movegen.o $(DOBJ)/movegen_benchmark.o $(DOBJ)/time_manager.o $(DOBJ)/eval.o $(DOBJ)/moveorder.o $(DOBJ)/transposition.o
MAIN_OBJ = $(DOBJ)/main.o
MAGIC_OBJ = $(DOBJ)/gen_magic_nums.o
TEST_OBJECTS = $(DOBJ)/piece_test.o $(DOBJ)/board_test.o $(DOBJ)/interface_test.o $(DOBJ)/board_state_test.o $(DOBJ)/bitboard_test.o $(DOBJ)/movegen_test.o $(DOBJ)/time_manager_test.o $(DOBJ)/transposition_test.o

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

# Compile test object files (from ./test folder)
$(DOBJ)/%.o: $(DTEST)/%.cpp Makefile
	$(CC) -I$(DINC) -c $< -o $@


DEPS := $(OBJECTS:.o=.d)
-include $(DEPS)

# Compile source object files (from ./src folder)
$(DOBJ)/%.o: $(DSRC)/%.cpp Makefile
	$(CC) -I$(DINC) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(DOBJ)/*.o $(DEXE)/* $(DOBJ)/*.d

# Run the main executable
run:
	cd $(DEXE); \
	./$(EXE)

# Run all tests
test: $(DEXE)/$(TEST_EXE)
	./$(DEXE)/$(TEST_EXE)  # Run the test executable

magic: $(DEXE)/$(MAGIC_EXE)
	./$(DEXE)/$(MAGIC_EXE)

