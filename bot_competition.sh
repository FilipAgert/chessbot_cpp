

DEFAULT_ENGINE_1="filipbot"
DEFAULT_ENGINE_2="filipbot_base"
ENGINE_PATH="bin"

# Check the number of arguments provided
if [ "$#" -lt 2 ]; then
    echo "Warning: Expected 2 arguments (engine1 and engine2). Using defaults for missing arguments."

    # If $1 is missing, set it to the default
    if [ -z "$1" ]; then
        ENGINE1_CMD=$DEFAULT_ENGINE_1
        ENGINE1_NAME=$DEFAULT_ENGINE_1
    else
        ENGINE1_CMD=$1
        ENGINE1_NAME=$1
    fi

    # If $2 is missing, set it to the default
    if [ -z "$2" ]; then
        ENGINE2_CMD=$DEFAULT_ENGINE_2
        ENGINE2_NAME=$DEFAULT_ENGINE_2
    else
        ENGINE2_CMD=$2
        ENGINE2_NAME=$2
    fi
else
    # Both arguments are provided, use them directly
    ENGINE1_CMD=$1
    ENGINE1_NAME=$1
    ENGINE2_CMD=$2
    ENGINE2_NAME=$2
fi
ENGINE1_CMD="$ENGINE_PATH/$ENGINE1_CMD"
ENGINE2_CMD="$ENGINE_PATH/$ENGINE2_CMD"

# Execute fastchess with the determined engine commands and names
echo "Running SPRT: Engine 1 ($ENGINE1_NAME) vs Engine 2 ($ENGINE2_NAME)"
fastchess -engine cmd=$ENGINE1_CMD name=$ENGINE1_NAME -engine cmd=$ENGINE2_CMD name=$ENGINE2_NAME -each tc=10+0.1 -rounds 5000 -repeat -concurrency 4 -openings file=opening_books/8moves_v3.pgn format=pgn order=random -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 -log file="out.log"
