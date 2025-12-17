import datetime
import json
import math
import os
import re
import statistics
import subprocess
import time
from math import nan

import numpy as np

# --- Configuration ---
SCOREBOARD_FILE = "bench/performance_scoreboard.json"
NUM_RUNS = 10  # Number of times to run the test PER FEN
BENCHMARK_DEPTHS = [5]  # Search depth for the benchmark
NUM_THREADS = 1  # Number of threads to use (pass to the executable)
AI_EXECUTABLE_PATH = "./bin/filipbot"
FEN_CONFIG_FILE = "bench/fen_benchmarks.txt"  # File containing FENs for benchmarking

# The script expects your AI to print a line to standard output that looks like:
# Nodes per second: 989111
OUTPUT_PATTERN = r"Nodes per second:\s*(\d+\.?\d*)"


def get_git_info():
    """Retrieves the current Git commit hash and message."""
    try:
        commit_hash = subprocess.run(
            ["git", "rev-parse", "HEAD"], capture_output=True, text=True, check=True
        ).stdout.strip()
        commit_msg = (
            subprocess.run(
                ["git", "log", "-1", "--pretty=%B"],
                capture_output=True,
                text=True,
                check=True,
            )
            .stdout.strip()
            .split("\n")[0]
        )  # Get the first line of the message
        return commit_hash, commit_msg
    except subprocess.CalledProcessError as e:
        print(
            f"Error: Could not retrieve Git info. Make sure you are in a Git repository."
        )
        print(e.stderr)
        exit(1)
    except FileNotFoundError:
        print(
            "Error: 'git' command not found. Ensure Git is installed and in your PATH."
        )
        exit(1)


def build_ai():
    """Runs 'make clean' and 'make type=release'."""
    if os.path.exists(AI_EXECUTABLE_PATH):
        print(
            "Executable found. Do you wish to rebuild to ensure correct commit hash (y/n)?"
        )
        inp = input()
        if "n" in inp:
            return

    print("--- Building AI in Release Mode ---")
    try:
        print("Running 'make clean'...")
        subprocess.run(["make", "clean"], check=True, capture_output=True, text=True)

        print("Stashing non-commited changes (if applicable)")
        stash_text = subprocess.run(
            ["git", "stash"], check=True, capture_output=True, text=True
        )

        print(f"Running 'make type=release' to generate {AI_EXECUTABLE_PATH}...")
        subprocess.run(
            ["make", "type=release"],
            check=True,
            capture_output=True,
            text=True,
        )

        if not os.path.exists(AI_EXECUTABLE_PATH):
            print(
                f"Error: Build successful, but executable not found at {AI_EXECUTABLE_PATH}. Check your Makefile output."
            )
            exit(1)

        print("Build successful.")
        if "No local changes to save" not in stash_text.stdout:
            print("Popping stash...")
            subprocess.run(
                ["git", "stash", "pop"], check=True, capture_output=True, text=True
            )

    except subprocess.CalledProcessError as e:
        print(f"Error during build process:\n{e.stderr}")
        exit(1)


def load_fen_configs():
    """Loads FEN strings from the configuration file."""
    if not os.path.exists(FEN_CONFIG_FILE):
        print(f"Error: FEN configuration file not found at {FEN_CONFIG_FILE}")
        exit(1)

    with open(FEN_CONFIG_FILE, "r") as f:
        # Filter out empty lines and tr    with open(FEN_CONFIG_FILE, 'r') as f:
        # Filter out empty lines and trim whitespace
        # Also filter out lines that start with '#' (comments)
        fens = [
            line.strip()
            for line in f
            if line.strip() and not line.strip().startswith("#")
        ]

    if not fens:
        print(f"Error: No FEN strings found in {FEN_CONFIG_FILE}.")
        exit(1)

    print(f"Loaded {len(fens)} FENs from {FEN_CONFIG_FILE}.")
    return fens


def run_perf_test(fens, depths, threads, num_runs):
    """
    Starts the AI process once and sends benchmark commands over stdin.
    Returns: {depth: {fen: [nps_run1, nps_run2, ...]}}
    """
    # Initialize the results structure
    raw_results = {d: {f: [] for f in fens} for d in depths}
    
    total_fen_configs = len(fens) * len(depths)
    total_data_points = num_runs * total_fen_configs
    
    print(f"\n--- Running Benchmark (Depths={depths}, Threads={threads}, Total Configs={total_fen_configs}, Total Runs={total_data_points}) ---")

    current_config_count = 0
    process = None
    
    try:
        # 1. Start the AI process persistently
        print(f"Starting AI process: {AI_EXECUTABLE_PATH}...")
        process = subprocess.Popen(
            [AI_EXECUTABLE_PATH],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, # Merge stdout and stderr for easier parsing
            text=True, # Use text mode for string I/O
            bufsize=1 # Line buffering
        )
        
        # Give the AI a moment to initialize
        time.sleep(0.5) 
        
        if process.poll() is not None:
             raise RuntimeError(f"AI executable {AI_EXECUTABLE_PATH} exited immediately upon start.")

        for depth in depths:
            time.sleep(0.5)
            for fen_idx, fen in enumerate(fens):
                current_config_count += 1
                
                for run_idx in range(1, num_runs + 1):
                    progress_label = f"Config {current_config_count}/{total_fen_configs} (D={depth}, FEN {fen_idx + 1}/{len(fens)}), Run {run_idx}/{num_runs}"
                    print(f"{progress_label}...", end='\r')
                    
                    # 2. Construct and send the command to stdin
                    command_line = f"bench {fen} {depth} {threads}\n"
                    process.stdin.write(command_line)
                    process.stdin.flush() # Ensure the command is sent immediately

                    # 3. Read output line-by-line until the result is found or timeout
                    nps = nan
                    start_time = time.time()
                    TIMEOUT = 120 # Timeout per single run
                    
                    while (time.time() - start_time) < TIMEOUT:
                        if process.poll() is not None:
                             raise RuntimeError(f"AI process exited unexpectedly during run.")

                        # Use process.stdout.readline() with a short timeout to prevent blocking indefinitely
                        # Note: In some environments, readline() doesn't support timeout, so we use select/poll 
                        # or rely on the process to output fast enough. For simple Popen, we'll try to read 
                        # and rely on the overall try-except block for process control.
                        line = process.stdout.readline().strip()
                        if not line:
                            # Avoid busy-waiting, give the process time to compute
                            time.sleep(0.01)
                            continue 
                        
                        # Search for the defined nodes/s pattern in the line
                        match = re.search(OUTPUT_PATTERN, line)
                        
                        if match:
                            nps = float(match.group(1))
                            raw_results[depth][fen].append(nps)
                            break # Move to the next run/config
                    
                    if nps is nan:
                        print(f"\nError: AI did not return performance data within {TIMEOUT}s for {progress_label}. Command: {command_line.strip()}")
                        # Optionally terminate and restart the process here if required, but for simplicity, we exit on critical error
                        exit(1)

        print(f"Benchmarking complete. Total results stored. Shutting down AI process.")
        return raw_results
    
    except Exception as e:
        print(f"\nAn error occurred during benchmarking: {e}")
        # Re-raise the exception after cleanup (optional, depends on desired exit behavior)
        exit(1)

    finally:
        # 4. Terminate the process if it's still running
        if process and process.poll() is None:
            # Send 'quit' command if the engine supports it
            try:
                process.stdin.write("quit\n")
                process.stdin.flush()
                process.wait(timeout=5)
            except:
                pass # Ignore errors on quit attempt

            if process.poll() is None:
                # Force kill if it didn't quit gracefully
                process.terminate()
                process.wait(timeout=5)
                

def calculate_detailed_stats(raw_results):
    """
    Calculates mean and stdev for every FEN/Depth combination, plus an overall mean/stdev.
    Returns: (detailed_stats, overall_mean_nps, overall_sample_stdev_nps)
    """
    detailed_stats = {}
    total_nps_for_overall_mean = []

    for depth, fen_results in raw_results.items():
        detailed_stats[depth] = {}
        for fen, results in fen_results.items():

            if not results:
                mean_val, stdev_val = nan, nan
            else:
                mean_val = statistics.mean(results)
                # Stdev requires at least 2 samples, otherwise it's 0.0
                stdev_val = statistics.stdev(results) if len(results) >= 2 else 0
                # Collect all results for the global average calculation
                total_nps_for_overall_mean.extend(results)

            detailed_stats[depth][fen] = {
                "avg_nps": int(mean_val),
                "sample_stdev_nps": int(stdev_val),

            }

    # Calculate the global mean and stdev across ALL runs, FENs, and depths
    if not total_nps_for_overall_mean:
        overall_mean, overall_stdev = nan, nan
    else:
        overall_mean = int(statistics.mean(total_nps_for_overall_mean))
        overall_stdev = int(
            statistics.stdev(total_nps_for_overall_mean)
            if len(total_nps_for_overall_mean) >= 2
            else 0.0
        )
        overall_n = len(total_nps_for_overall_mean)

    return detailed_stats, overall_mean, overall_stdev, overall_n


def load_scoreboard():
    """Loads the existing scoreboard dictionary from JSON."""
    if not os.path.exists(SCOREBOARD_FILE):
        return {}
    try:
        with open(SCOREBOARD_FILE, "r") as f:
            return json.load(f)
    except json.JSONDecodeError:
        print(
            f"Warning: Scoreboard file {SCOREBOARD_FILE} is corrupted. Starting a new one."
        )
        return {}
    except Exception as e:
        print(f"Warning: Could not read scoreboard file: {e}. Starting a new one.")
        return {}
def combine_sample_stats(n1, mean1, stdev1, n2, mean2, stdev2):
    """
    Calculates the merged mean and standard deviation for two samples.

    :param n1: Number of observations in sample 1
    :param mean1: Mean of sample 1
    :param stdev1: Standard deviation of sample 1
    :param n2: Number of observations in sample 2
    :param mean2: Mean of sample 2
    :param stdev2: Standard deviation of sample 2
    :return: A tuple (combined_mean, combined_stdev)
    """

    # --- 1. Combined Mean ---
    
    # Calculate the numerator (sum of all data points)
    sum_x1 = n1 * mean1
    sum_x2 = n2 * mean2
    
    # Calculate the total number of observations
    n_combined = n1 + n2
    
    # Calculate the combined mean (weighted average)
    combined_mean = (sum_x1 + sum_x2) / n_combined

    # --- 2. Combined Standard Deviation ---

    # 2a. Calculate the sum of squared differences (SSD) for both samples 
    # This combines the within-sample variance (from stdev) and the 
    # between-sample variance (from the difference to the combined mean).
    
    # Within-sample variance (variance of the first sample): (n1 - 1) * s1^2
    var1_within = (n1 - 1) * (stdev1 ** 2)
    # Between-sample variance (difference from sample 1 mean to combined mean)
    var1_between = n1 * ((mean1 - combined_mean) ** 2)
    
    # Within-sample variance (variance of the second sample): (n2 - 1) * s2^2
    var2_within = (n2 - 1) * (stdev2 ** 2)
    # Between-sample variance (difference from sample 2 mean to combined mean)
    var2_between = n2 * ((mean2 - combined_mean) ** 2)
    
    # Total Sum of Squared Differences (SSD)
    ssd = var1_within + var1_between + var2_within + var2_between

    # 2b. Calculate the Combined Variance 
    # Degrees of freedom for the combined sample is (n1 + n2 - 1)
    
    # Ensure denominator is not zero (requires at least 2 observations total)
    if n_combined <= 1:
        combined_stdev = 0.0 
    else:
        combined_variance = ssd / (n_combined - 1)
        
        # 2c. Calculate the Combined Standard Deviation
        combined_stdev = math.sqrt(combined_variance)

    return combined_mean, combined_stdev



def update_scoreboard(data, detailed_stats, overall_mean_nps, overall_sample_stdev_nps, overall_n, git_hash, git_msg):
    """Updates the scoreboard with the new performance data, using only the Git hash as the key.
    New (FEN, Depth) results are merged with existing ones. Collisions are resolved by 
    keeping the entry with the lowest relative error (highest stability).
    The overall NPS and STDEV for the hash are recalculated based on the merged data."""
    
    timestamp = datetime.datetime.now().isoformat()
    
    # Helper function to calculate the relative error for a single FEN/Depth result
    def calculate_fen_depth_rel_error(avg_nps, sample_stdev_nps):
        if avg_nps is nan or sample_stdev_nps is nan or avg_nps == 0:
            return nan
        return sample_stdev_nps / avg_nps
    
    # Calculate the relative error for the current run (lower is better)
    current_relative_error = calculate_fen_depth_rel_error(overall_mean_nps, overall_sample_stdev_nps)

    # 1. Generate the flattened stats map for the current run (Composite Key: FEN|Depth)
    current_fen_depth_stats = {}
    for depth_str, fen_results in detailed_stats.items():
        for fen, stats in fen_results.items():
            composite_key = f"{fen}|{depth_str}"
            
            # Core stats for this FEN/Depth configuration
            stats_core = {
                "avg_nps": stats["avg_nps"],
                "avg_std": int(stats["sample_stdev_nps"]/np.sqrt(NUM_RUNS)),
                "sample_stdev_nps": stats["sample_stdev_nps"],
                "threads": NUM_THREADS,
                "runs_per_fen": NUM_RUNS
            }
            
            # Add relative error to the temporary structure for easy comparison
            stats_core['rel_error'] = calculate_fen_depth_rel_error(stats["avg_nps"], stats["sample_stdev_nps"])
            
            current_fen_depth_stats[composite_key] = stats_core
            
    # 2. Build the core entry structure
    new_entry_core = {
        "date": timestamp,
        "commit_hash": git_hash,
        "commit_msg": git_msg,
        "overall_avg_nps": overall_mean_nps, 
        "overall_avg_std": int(overall_sample_stdev_nps/np.sqrt(overall_n)),
        "overall_sample_stdev_nps": overall_sample_stdev_nps, 
        "overall_n": overall_n,
        "relative_error": round(current_relative_error, 6) if not nan in [current_relative_error] else nan,
        "options": {
            "threads": NUM_THREADS,
            "runs_per_fen": NUM_RUNS,
            "fen_config_file": FEN_CONFIG_FILE
        }
    }

    # 3. Handle Merging and Collision Resolution
    if git_hash not in data:
        # Case 1: New Git Hash. Just add the full entry.
        final_entry = new_entry_core
        # Remove temporary rel_error from the final structure before saving
        for key in current_fen_depth_stats:
            current_fen_depth_stats[key].pop('rel_error', None)
        final_entry["fen_depth_stats"] = current_fen_depth_stats
        
        data[git_hash] = final_entry
        print(f"New entry created for Git hash {git_hash[:7]}.")
        latest_key = git_hash
    else:
        # Case 2: Existing Git Hash. Merge new stats with existing ones.
        existing_entry = data[git_hash]
        # This will hold the final, merged map
        merged_fen_depth_stats = existing_entry.get("fen_depth_stats", {})
        
        log_msgs = []
        
        for composite_key, new_stats in current_fen_depth_stats.items():
            new_rel_err = new_stats['rel_error']
            
            if composite_key not in merged_fen_depth_stats:
                # New (FEN, Depth) combination: Append it.
                new_stats.pop('rel_error', None) # Remove temp key
                merged_fen_depth_stats[composite_key] = new_stats
                
                log_msgs.append(f"Appended new configuration: {composite_key}.")
            else:
                # Collision: Compare and replace if new is better (lowest relative error).
                existing_stats = merged_fen_depth_stats[composite_key]
                old_avg = existing_stats.get('avg_nps')
                old_std = existing_stats.get('sample_stdev_nps')
                old_cnt = existing_stats.get('runs_per_fen')
                run_avg = new_stats.get('avg_nps')
                run_std = new_stats.get('sample_stdev_nps')
                run_cnt = new_stats.get('runs_per_fen')
                new_avg, new_std = combine_sample_stats(old_cnt, old_avg, old_std, run_cnt, run_avg, run_std)
                new_cnt = run_cnt + old_cnt
                
                new_stats.update({
                    "avg_nps":int(new_avg),
                    "avg_std": int(new_std/np.sqrt(new_cnt)),
                    "sample_stdev_nps":int(new_std),
                    "runs_per_fen":new_cnt,
                })
                log_msgs.append(f"Existing {composite_key}: Merged stats with current run.")

                new_stats.pop('rel_error', None) # Remove temp key
                merged_fen_depth_stats[composite_key] = new_stats
        
        # 4. Recalculate Overall Performance metrics from the MERGED fen_depth_stats
        
        running_avg = 0
        running_stdev = 0
        running_cnt = 0
        for stats in merged_fen_depth_stats.values():
            avg_nps = stats.get("avg_nps", nan) 
            std_nps = stats.get("sample_stdev_nps", nan)
            cnt = stats.get("runs_per_fen")
            running_avg, running_stdev = combine_sample_stats(running_cnt, running_avg, running_stdev, cnt, avg_nps, std_nps)
            running_cnt += cnt

        # 5. Update the existing entry with merged data and recalculated overall metrics
        existing_entry.update({
            "date": timestamp, # Update date to show last benchmark run
            "options": new_entry_core["options"], # Update options to reflect latest run config
            "fen_depth_stats": merged_fen_depth_stats,
            "overall_avg_nps": int(running_avg),
            "overall_avg_std": int(running_avg/np.sqrt(running_cnt)),
            "overall_sample_stdev_nps": int(running_stdev),
            "overall_n": running_cnt,
        })
        
        existing_entry.pop('relative_error', None)
        # 6. Report what happened
        print(f"Merged new run data into existing entry for Git hash {git_hash[:7]}.")
        print("Configuration merge results:")
        for msg in log_msgs:
            print(f"- {msg}")
        print(f"Recalculated Overall Mean NPS (across {len(merged_fen_depth_stats)} unique FEN/Depth configurations): {existing_entry['overall_avg_nps']:,}")

        latest_key = git_hash

    # 7. Save data
    with open(SCOREBOARD_FILE, 'w') as f:
        json.dump(data, f, indent=4)
    
    # Return the latest key
    return latest_key

def display_scoreboard(data, latest_key):
    """Prints a comparison between the latest run and the chronologically previous run, 
    plus detailed stats and a Top 5 list. Reads from the flattened fen_depth_stats."""
    print("\n" + "="*80)
    print("                 PERFORMANCE SCOREBOARD AND REGRESSION CHECK")
    print("="*80)

    all_entries = list(data.values())
    
    # 1. Sort chronologically to find the true "previous" run
    chronological_entries = sorted(all_entries, key=lambda x: x['date'])
    
    # 2. Find the latest entry
    latest_entry = data[latest_key]
    latest_nps = latest_entry.get('overall_avg_nps', nan)
    nps_label = "Nodes/s"
    
    # Find the latest entry's index in the chronological list
    latest_index = -1
    for i, entry in enumerate(chronological_entries):
        if entry == latest_entry:
            latest_index = i
            break
            
    # --- New logic for derived depths and stats table ---
    
    # Get the merged FEN/Depth statistics map
    fen_depth_stats = latest_entry.get('fen_depth_stats', {})
    
    # 1. Derive the list of recorded depths for the summary
    recorded_depths = sorted(list(set(int(k.rsplit('|', 1)[-1]) for k in fen_depth_stats.keys())))
    depths_str = ','.join(map(str, recorded_depths))
    
    # 3. Print Latest Run Summary
    latest_stdev = latest_entry.get('overall_sample_stdev_nps', nan)
    print(f"LATEST RUN (Hash {latest_entry['commit_hash'][:7]}, Total Configs: {len(fen_depth_stats)}): {latest_entry['date'].split('T')[0]} - {latest_entry['commit_msg']}")
    print(f"Hash: {latest_entry['commit_hash'][:7]}")
    print(f"Depths Recorded: {depths_str}")
    print(f"Options: Runs/FEN/Depth={latest_entry['options']['runs_per_fen']}, Threads={latest_entry['options']['threads']}")
    print(f"Result (Recalculated Overall Mean): {latest_nps:,} {nps_label} (±{latest_stdev})")

    # 4. Compare with the chronologically previous run (any hash/depth combination)
    prev_entry = None
    if latest_index > 0:
        # Iterate backwards to find the last unique entry regardless of hash/depth
        prev_entry = chronological_entries[latest_index - 1]

    if prev_entry:
        prev_nps = prev_entry.get('overall_avg_nps', nan)

        if not nan in [latest_nps, prev_nps] and prev_nps != 0:
            diff = latest_nps - prev_nps
            percentage_change = (diff / prev_nps) * 100

            if diff > 0:
                status = "IMPROVEMENT"
                color = "\033[92m" # Green
            elif diff < 0:
                status = "REGRESSION"
                color = "\033[91m" # Red
            else:
                status = "NO CHANGE"
                color = "\033[93m" # Yellow

            reset_color = "\033[0m"

            # Display info for the previous entry
            prev_fen_depth_stats_count = len(prev_entry.get('fen_depth_stats', {}))
            print("\n--- COMPARISON TO PREVIOUS RUN (Overall Performance) ---")
            print(f"Previous Run (Hash {prev_entry['commit_hash'][:7]}, Total Configs: {prev_fen_depth_stats_count}, Date: {prev_entry['date'].split('T')[0]}): {prev_nps:,} {nps_label}")
            print(f"Delta: {color}{diff:+,} {nps_label} ({percentage_change:+.2f}%){reset_color} - {status}")
        else:
             print("\n--- COMPARISON TO PREVIOUS RUN ---")
             print("Skipping comparison: Previous run data is invalid or zero.")
    
    # 5. Detailed Stats Table 
    
    # Group the flat stats back into a nested structure for display ease
    display_stats_by_depth = {}
    
    for key, stats in fen_depth_stats.items():
        # Split key by the last '|' to separate FEN from Depth
        fen_part, depth_part = key.rsplit('|', 1)
        depth = int(depth_part)
        
        if depth not in display_stats_by_depth:
            display_stats_by_depth[depth] = {}
            
        display_stats_by_depth[depth][fen_part] = stats

    print("\n" + "-"*80)
    print("                  DETAILED PERFORMANCE PER FEN & DEPTH")
    print("-"*80)

    # We load FENs again here to ensure the original FEN string is available for printing
    fens = []
    try:
        fens = load_fen_configs() 
    except:
        pass # Ignore errors here as main should have already handled them

    # Iterate through depths in sorted order
    for depth in sorted(display_stats_by_depth.keys()):
        print(f"\n--- Depth {depth} ---")
        fen_stats = display_stats_by_depth[depth]
        
        # Determine max FEN length for neat printing (use actual FENs from config)
        max_fen_len = min(60, max(len(fen) for fen in fens)) if fens else 0
        
        # Table header
        header = f"{'FEN':<{max_fen_len}} | {'AVG Nodes/s':>15}  {'Mean err' :>10} | {'STDEV':>10}"
        print(header)
        print("-" * (max_fen_len + 30))
        
        for fen in fens:
            stats = fen_stats.get(fen)
            # Shorten FEN for display if it's too long
            display_fen = fen[:max_fen_len]
            
            if stats:
                avg = stats['avg_nps']
                stdev = stats['sample_stdev_nps']
                avgstev = stats['avg_std']
                
                avg_str = f"{avg: }" if not nan in [avg] else "N/A"
                stdev_str = f"±{stdev: }" if not nan in [stdev] else "N/A"
                avgstdev_str = f"±{avgstev: }" if not nan in [avgstev] else "N/A"
                
                print(f"{display_fen:<{max_fen_len}} | {avg_str:>15}  {avgstdev_str:>10} | {stdev_str:>10}")
            else:
                # If the FEN was in the config but not tested for this depth, show missing
                print(f"{display_fen:<{max_fen_len}} | {'NOT RECORDED':>15}  {'':>10}| {'N/A':>10}") 

    # 6. Top 5 Scoreboard 
    print("\n" + "="*80)
    print("                      TOP 5 PERFORMANCE SCOREBOARD")
    print(" (Ranked by Recalculated Overall Avg Nodes/s across all recorded configs)")
    print("="*80)
    
    # Filter out invalid NPS entries and sort by NPS descending
    valid_entries = [e for e in all_entries if not nan in [e.get('overall_avg_nps', nan)]]
    best_nps_entries = sorted(valid_entries, key=lambda x: x['overall_avg_nps'], reverse=True)
    top_5 = best_nps_entries[:5]
    
    if top_5:
        # Changed column title from 'Total Depths' to 'Total Configs'
        header = f"{'Rank':<5} | {'Hash':<7} | {'Date':<10} | {'Total Configs':<13} | {'Avg Nodes/s':>15}  {'':>10} | {'STDEV':>10}"
        print(header)
        print("-" * 62)
        
        for rank, entry in enumerate(top_5, 1):
            nps = entry['overall_avg_nps']
            npserr = entry['overall_avg_std']
            stdev = entry['overall_sample_stdev_nps']
            date = entry['date']
            hash_str = entry['commit_hash'][:7]
            
            # Use the count of recorded FEN/Depth configs
            fen_depth_stats_count = len(entry.get('fen_depth_stats', {}))
            
            nps_str = f"{nps: }"
            stdev_str = f"±{stdev: }"
            npserr_str = f"±{npserr: }"
            
            print(f"{rank:<5} | {hash_str:<7} | {date[0:10]} | {fen_depth_stats_count:<13} | {nps_str:>15}  {npserr_str:>10} | {stdev_str:>10}")
    else:
        print("No valid performance data available for the Top 5 list.")
    
    print("="*80)


def main():
    """Main execution flow."""
    # 1. Get Git Info
    git_hash, git_msg = get_git_info()

    # 2. Build the AI executable
    build_ai()

    # 3. Load FENs
    fens = load_fen_configs()

    # 4. Run Performance Tests (Note: pass BENCHMARK_DEPTHS)
    results = run_perf_test(fens, BENCHMARK_DEPTHS, NUM_THREADS, NUM_RUNS)

    # 5. Calculate Detailed Stats (Still returns {depth: {fen: {stats}}})
    detailed_stats, overall_mean_nps, overall_sample_stdev_nps, overall_n = calculate_detailed_stats(results)
    
    if overall_mean_nps is nan:
        print("Fatal: No valid performance data was collected. Exiting.")
        return

    # 6. Update Scoreboard (Implements composite keying and collision resolution)
    scoreboard_data = load_scoreboard()
    latest_key = update_scoreboard(scoreboard_data, detailed_stats, overall_mean_nps, overall_sample_stdev_nps, overall_n, git_hash, git_msg)

    # 7. Display Scoreboard and Comparison
    display_scoreboard(scoreboard_data, latest_key)

    print(f"\nResults saved to {SCOREBOARD_FILE}")


if __name__ == "__main__":
    main()
