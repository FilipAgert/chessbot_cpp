# paths
DSRC = ./src
DINC = ./include
DOBJ = ./bin
DEXE = ./exe
DTEST = ./tests

EXEN = main.exe
TEST_EXE = test.exe

# flags
FLAGS = -Wall -std=c++17 -O1 -I$(DINC)
LIBS = -lgtest -lgtest_main -pthread  # Google Test and pthread libs

# commands for compilation
CCL = g++ -o
CC = g++ $(FLAGS) -c -g

# objects
OBJECTS = $(DOBJ)/uci_interface.o $(DOBJ)/piece.o $(DOBJ)/board.o $(DOBJ)/game.o $(DOBJ)/notation_interface.o $(DOBJ)/board_state.o
MAIN_OBJ = $(DOBJ)/main.o
TEST_OBJECTS = $(DOBJ)/piece_test.o $(DOBJ)/board_test.o $(DOBJ)/interface_test.o $(DOBJ)/board_state_test.o $(DOBJ)/bitboard_test.o# Add object files for your tests (adjust the name as needed)

# Targets
main: $(DEXE)/$(EXEN)

# Link the main executable
$(DEXE)/$(EXEN): $(OBJECTS) $(MAIN_OBJ)
	$(CCL) $@ $(MAIN_OBJ) $(OBJECTS) $(LIBS)

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
