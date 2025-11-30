import subprocess
import sys
import time

# --- CONFIGURATION ---
EXECUTABLE_SF = "stockfish"
# Ensure the path style matches your OS (forward slashes usually work in python on Win too)
EXECUTABLE_FA = "exe/main.exe" 
#--- STANDARD FEN LIBRARY ---
# These FENs are commonly used for perft testing, often called 'Perft Suite' positions.
STANDARD_FENS = {
    1: ("Startpos (Initial Board)", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
    2: ("Kiwipete (Rook, Bishop, Queen moves)", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 1 1")
}
def start_engine(path):
    """Starts an engine process."""
    try:
        process = subprocess.Popen(
            path,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1 # Line buffered
        )
        return process
    except FileNotFoundError:
        print(f"Error: Could not find executable at {path}")
        sys.exit(1)

def send_command(process, command):
    """Sends a command to the engine."""
    # print(f"-> {command}") # Uncomment for debug
    process.stdin.write(command + "\n")
    process.stdin.flush()

def get_user_fen():
    """Prompts the user to select a FEN or enter a custom one."""
    print("\n--- STANDARD FENS ---")
    for key, (name, fen) in STANDARD_FENS.items():
        print(f" {key}. {name}: {fen}")
    
    print("\n[C]. Custom FEN")
    
    while True:
        choice = input(f"Enter selection (1-{len(STANDARD_FENS)} or C): ").strip().lower()
        if choice in ['1', '2', '3', '4', '5', '6']:
            key = int(choice)
            return STANDARD_FENS[key][1]
        elif choice == 'c':
            fen = input("Please paste your custom FEN: ").strip()
            if fen:
                return fen
            else:
                print("Custom FEN cannot be empty. Please try again.")
        else:
            print("Invalid choice. Please select a number (1-6) or 'C' for custom.")

def get_perft_results(process, engine_name):
    """
    Reads output until the total node count is found.
    Returns a dictionary: {'e2e4': 20, 'a2a3': 15, ...} and the total count.
    """
    results = {}
    total_nodes = 0
    
    while True:
        line = process.stdout.readline()
        if not line:
            break
            
        line = line.strip()
        # print(f"[{engine_name}] {line}") # Uncomment to see raw output

        # Standard UCI perft output usually looks like: "e2e4: 12345"
        if ":" in line and "Nodes searched" not in line and "Error" not in line:
            parts = line.split(":")
            if len(parts) == 2:
                move = parts[0].strip()
                try:
                    count = int(parts[1].strip())
                    results[move] = count
                except ValueError:
                    pass # Not a perft line
        
        # Check for end of perft (Stockfish uses "Nodes searched: X")
        # Your engine might need adjustment here if the total line is different
        if "Nodes searched" in line or "Total" in line:
            # Try to grab the last number in the line
            try:
                total_nodes = int(line.split()[-1])
            except ValueError:
                total_nodes = 0
            break
            
    return results, total_nodes

def main():
    print("--- CHESS ENGINE PERFT BISECTOR ---")
    
    fen = get_user_fen()
    
    print("Please enter the maximum search depth:")
    depth = input().strip()

    # 1. Start Engines
    print(f"\nStarting {EXECUTABLE_SF}...")
    sf_proc = start_engine(EXECUTABLE_SF)
    
    print(f"Starting {EXECUTABLE_FA}...")
    fa_proc = start_engine(EXECUTABLE_FA)

    # Initialize UCI mode (optional but good practice)
    send_command(sf_proc, "uci")
    send_command(fa_proc, "uci")
    
    # Wait a tiny bit for startup
    time.sleep(0.5)

    # 2. Run the bot_cmd (Set Position)
    position_cmd = f"position fen {fen}"
    send_command(sf_proc, position_cmd)
    send_command(fa_proc, position_cmd)

    # 3. Run the perft_cmd
    perft_command = f"go perft {depth}"
    print(f"\nRunning: {perft_command}")
    
    send_command(sf_proc, perft_command)
    send_command(fa_proc, perft_command)

    # 4. & 5. Read outputs
    print("Parsing Stockfish output...")
    sf_moves, sf_total = get_perft_results(sf_proc, "Stockfish")
    
    print("Parsing Custom Engine output...")
    fa_moves, fa_total = get_perft_results(fa_proc, "Custom")

    # Clean up processes
    sf_proc.terminate()
    fa_proc.terminate()

    # --- COMPARISON ---
    print("\n" + "="*40)
    print(f"RESULTS COMPARISON (Depth {depth})")
    print(f"Stockfish Total: {sf_total}")
    print(f"Custom Total:    {fa_total}")
    print("="*40)

    if sf_total == fa_total:
        print("✅ SUCCESS: Node counts match perfectly!")
    else:
        print("❌ FAILURE: Discrepancy detected.\n")
        
        # Find moves in SF but not in FA
        all_moves = set(sf_moves.keys()) | set(fa_moves.keys())
        
        found_diff = False
        for move in sorted(all_moves):
            sf_count = sf_moves.get(move, 0)
            fa_count = fa_moves.get(move, 0)
            
            if sf_count != fa_count:
                found_diff = True
                diff = fa_count - sf_count
                print(f"Move {move}:")
                print(f"   Stockfish: {sf_count}")
                print(f"   Custom:    {fa_count}")
                print(f"   Diff:      {diff:+d}")
                
        if not found_diff and sf_total != fa_total:
            print("Individual moves match, but totals differ? Check parsing logic.")

if __name__ == "__main__":
    main()
