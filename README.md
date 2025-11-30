# Chessbot in C++
This README provides basic information on compiling, running.

The chessbot aims to communicate with the UCI interface.

---

## Compiling
Running the command
```bash
make
```
Will output the executeable in 
```app/main.exe```
which can be ran with the command
```bash
app/main.exe
```
## UCI interface
After launching the executeable, the program will output
```bash
Welcome to the UCI interface!
```
if its running correctly.

Setting up a position can be done in one of two ways, as seen below.
### From FEN
From a FEN string, it can be set up by launching the command
```bash
position fen <fen> moves <moves>
```
where ```moves <moves>``` is optional but allows for doing moves after the FEN string. The moves must be 4 or 5 characters, in full chess notation. All moves are 4 length except promotions which are 5. Examples:
- ```a2a4``` move piece from a2 to a4.
- ```a7a8Q``` promote pawn from a7 to a8.


### From start position
To generate a board from the start position put the command
```bash
position startpos moves <moves>
```
where the syntax for moves is the same as above.

### PERFT
Move generation / PERFT can be performed after setting up the position by typing the command
```bash
go perft <depth>
```
into the console, where <depth> is number of ply (half moves) to generate moves for. The program will then output the number of possible moves, along which the number of moves in this branch found at the specified depth.
The total number of moves at this depth will also be displayed. For example
```bash
go perft 2
```
will output
```bash
a2a4: 15
b7b8: 3

Nodes searched: 18
```
  
If the move ```a2a4``` leads to 15 countermoves by the other color and ```b7b8``` leads to 3.
### Other commands
- board: Will print the board human readable to console.

## Testing
To run the unit tests, type
```bash
make test
```
which runs the full test suite. For this to work, GTEST needs to be installed on your computer.

### Bisect
```bisect.py``` Compares this program with another chessbot for validating move generation. User can input a FEN, or select one of the premade ones, and compare number of moves with the other chess bot.
If there are discrepancies, the program finds the moves with discrepancies to easily find the source of error.
This requires stockfish to be installed on your computer, which should be ran with
```bash
stockfish
```
If stockfish is not installed, you can modify ```bisect.py``` to replace stockfish with the chess bot of your choice.


## Clanker use
The bisect.py and most of the unit tests were generated mainly with clankers. The src code is not.
