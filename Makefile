# paths
DSRC = ./src
DINC = ./include
DOBJ = ./build
DEXE = ./bin
DTEST = ./tests

EXEN = filipbot
TEST_EXE = testsuite
MAGIC_EXE = gen_magics

#compile in parallel
NPROCS:= $(SHELL grep -c 'processor' /proc/cpuinfo)
MAKEFLAGS+= -j$(NPROCS)

type?=dev
ifeq ($(type), release)
	FLAGS = -Wall -std=c++23 -O3 -MMD -MP
else
	FLAGS = -DDEBUG -g -Wall -std=c++23 -Ofast   -MMD -MP
endif

DEPS := $(OBJS:.o=.d)
-include $(DEPS)

LIBS = -lgtest -lgtest_main -pthread  # Google Test and pthread libs


# commands for compilation
CCL = g++ -o
CC = g++ $(FLAGS) -c -g

# objects
OBJECTS = $(DOBJ)/uci_interface.o $(DOBJ)/piece.o $(DOBJ)/board.o $(DOBJ)/game.o $(DOBJ)/notation_interface.o $(DOBJ)/bitboard.o $(DOBJ)/movegen.o $(DOBJ)/movegen_benchmark.o $(DOBJ)/time_manager.o $(DOBJ)/eval.o
MAIN_OBJ = $(DOBJ)/main.o
MAGIC_OBJ = $(DOBJ)/gen_magic_nums.o
TEST_OBJECTS = $(DOBJ)/piece_test.o $(DOBJ)/board_test.o $(DOBJ)/interface_test.o $(DOBJ)/board_state_test.o $(DOBJ)/bitboard_test.o $(DOBJ)/movegen_test.o $(DOBJ)/time_manager_test.o# Add object files for your tests (adjust the name as needed)

# Targets
main: $(DEXE)/$(EXEN)

# Link the main executable
$(DEXE)/$(EXEN): $(OBJECTS) $(MAIN_OBJ)
	$(CCL) $@ $(MAIN_OBJ) $(OBJECTS) $(LIBS)

$(DEXE)/$(MAGIC_EXE): $(OBJECTS) $(MAGIC_OBJ)
	$(CCL) $@ $(MAGIC_OBJ) $(OBJECTS) $(LIBS)
# Test executable target
$(DEXE)/$(TEST_EXE): $(TEST_OBJECTS) $(OBJECTS)
	$(CCL) $@ $(TEST_OBJECTS) $(OBJECTS) $(LIBS)

# Compile test object files (from ./test folder)
$(DOBJ)/%.o: $(DTEST)/%.cpp
	$(CC) -I$(DINC) -I$(GTEST_INCLUDE_DIR) -c $< -o $@

# Compile source object files (from ./src folder)
$(DOBJ)/%.o: $(DSRC)/%.cpp
	$(CC) -I$(DINC) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(DOBJ)/*.o $(DEXE)/*.exe

# Run the main executable
run:
	cd $(DEXE); \
	./$(EXEN)

# Run all tests
test: $(DEXE)/$(TEST_EXE)
	./$(DEXE)/$(TEST_EXE)  # Run the test executable

magic: $(DEXE)/$(MAGIC_EXE)
	./$(DEXE)/$(MAGIC_EXE)
