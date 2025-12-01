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

# --- ENGINE UTILITIES (Same as before) ---

def start_engine(path):
    """Starts an engine process."""
    try:
        process = subprocess.Popen(
            path,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1
        )
        return process
    except FileNotFoundError:
        print(f"Error: Could not find executable at {path}")
        sys.exit(1)

def send_command(process, command):
    """Sends a command to the engine."""
    process.stdin.write(command + "\n")
    process.stdin.flush()

def get_perft_results(process, engine_name):
    """Reads output until the total node count is found."""
    results = {}
    total_nodes = 0
    
    # Send a dummy command to clear buffers (or wait for "readyok")
    send_command(process, "isready")
    while True:
        line = process.stdout.readline().strip()
        if line == "readyok": break
        
    # Now execute the perft reading logic
    while True:
        line = process.stdout.readline()
        if not line: break
            
        line = line.strip()

        if ":" in line and "Nodes searched" not in line and "Error" not in line:
            parts = line.split(":")
            if len(parts) == 2:
                move = parts[0].strip()
                try:
                    count = int(parts[1].strip())
                    results[move] = count
                except ValueError:
                    pass
        
        if "Nodes searched" in line or "Total" in line:
            try:
                total_nodes = int(line.split()[-1])
            except ValueError:
                total_nodes = 0
            break
            
    return results, total_nodes

# --- NEW RECURSIVE BISECTION LOGIC ---

def bisect_error_chain(fen, depth, move_chain, sf_proc, fa_proc):
    """
    Recursively searches down the move chain until the exact depth-1 error is found.
    """
    
    if depth <= 0:
        print("\n\n🔥 **ERROR PINPOINTED** 🔥")
        print(f"The discrepancy occurs *at* the FEN reached by the moves: {move_chain}")
        print("This means the error is in the move generation or legality checking for the next ply.")
        return

    # 1. Reset position to initial FEN
    position_cmd = f"position fen {fen} moves {' '.join(move_chain)}"
    
    send_command(sf_proc, position_cmd)
    send_command(fa_proc, position_cmd)

    # 2. Run perft at the new (decremented) depth
    perft_command = f"go perft {depth}"
    
    # Running perft on the new position (depth - 1 from the original call)
    send_command(sf_proc, perft_command)
    send_command(fa_proc, perft_command)
    
    # 3. Read results
    sf_moves, sf_total = get_perft_results(sf_proc, "Stockfish")
    fa_moves, fa_total = get_perft_results(fa_proc, "Custom")

    print("\n" + "---" * (depth + 1))
    print(f"DEPTH {depth}: {' '.join(move_chain) or 'Initial FEN'}")
    print(f"SF Total: {sf_total}, FA Total: {fa_total}")
    
    if sf_total == fa_total:
        print("Error: Totals matched unexpectedly in recursive call. Logic error or already fixed.")
        return

    # 4. Find the first discrepancy (the first move in the list that diverges)
    all_moves = set(sf_moves.keys()) | set(fa_moves.keys())
    
    # Sort for deterministic behavior
    for move in sorted(all_moves):
        sf_count = sf_moves.get(move, 0)
        fa_count = fa_moves.get(move, 0)
        
        if sf_count != fa_count:
            # 5. Take the first discrepant move and recurse
            next_move_chain = move_chain + [move]
            
            print(f"Divergence detected on move: {move}")
            print(f"SF Count: {sf_count}, FA Count: {fa_count}. Recursing...")
            
            # Recursive call: depth decreases by 1
            bisect_error_chain(fen, depth - 1, next_move_chain, sf_proc, fa_proc)
            return # Stop after the first divergence is processed

# --- MAIN EXECUTION LOGIC (Updated to call bisect_error_chain) ---

def get_user_fen():
    """Prompts the user to select a FEN or enter a custom one."""
    # (Function body omitted for brevity, assumed to work from previous step)
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

def main():
    print("--- CHESS ENGINE PERFT BISECTOR ---")
    
    fen = get_user_fen()
    
    while True:
        depth_input = input("Please enter the maximum search depth: ").strip()
        try:
            initial_depth = int(depth_input)
            if initial_depth < 1:
                print("Depth must be at least 1.")
                continue
            break
        except ValueError:
            print("Invalid depth. Please enter an integer.")

    print(f"\nStarting {EXECUTABLE_SF}...")
    sf_proc = start_engine(EXECUTABLE_SF)
    print(f"Starting {EXECUTABLE_FA}...")
    fa_proc = start_engine(EXECUTABLE_FA)

    # UCI Setup
    send_command(sf_proc, "uci")
    send_command(fa_proc, "uci")
    time.sleep(0.1) 
    send_command(sf_proc, "isready")
    send_command(fa_proc, "isready")
    sf_proc.stdout.readline()
    fa_proc.stdout.readline()


    # Initial Run to find the first error
    position_cmd = f"position fen {fen}"
    perft_command = f"go perft {initial_depth}"
    
    send_command(sf_proc, position_cmd)
    send_command(fa_proc, position_cmd)

    send_command(sf_proc, perft_command)
    send_command(fa_proc, perft_command)
    
    sf_moves, sf_total = get_perft_results(sf_proc, "Stockfish")
    fa_moves, fa_total = get_perft_results(fa_proc, "Custom")

    print("\n" + "="*40)
    print(f"INITIAL RUN (Depth {initial_depth})")
    print(f"Stockfish Total: {sf_total}")
    print(f"Custom Total:    {fa_total}")
    print("="*40)

    if sf_total == fa_total:
        print("✅ SUCCESS: Node counts match perfectly! No bisection needed.")
    else:
        print("❌ FAILURE: Discrepancy detected. Starting automated bisection...")
        
        # Find the first move that diverges
        first_divergence_move = None
        all_moves = set(sf_moves.keys()) | set(fa_moves.keys())
        
        for move in sorted(all_moves):
            if sf_moves.get(move, 0) != fa_moves.get(move, 0):
                first_divergence_move = move
                break

        if first_divergence_move:
            print(f"\nFirst error branch: {first_divergence_move}")
            # Start the recursion process
            bisect_error_chain(fen, initial_depth - 1, [first_divergence_move], sf_proc, fa_proc)
        else:
            print("ERROR: Totals differ, but no move count discrepancy found. Check parsing.")


    # Clean up processes
    sf_proc.terminate()
    fa_proc.terminate()

if __name__ == "__main__":
    main()
