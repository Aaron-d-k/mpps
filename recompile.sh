#!/bin/bash

# Thanks to Sokwe for the initial version of this code (https://conwaylife.com/forums/viewtopic.php?p=196276#p196276)


usage=$(cat <<EOF
Usage: ./recompile.sh -w width -p period [-r RULESTRING] [-m MAX TREE_SIZE] [-v VERBOSITY] [-o] [-a]
If no rule is provided, mpps will be compiled without parallel evolution.
    -w sets the search width
    -p sets the max period
    -o enables finding only one ship per rule
    -m helps limit memory usage
    -v makes the output of mpps less or more verbose. can range from -1 to 2
    -a enables assymetric searches
EOF
)


compiler="g++"

MAX_TREE_SIZE=20000000
VERBOSITY=2
MAX_ITER="INFTY"
MORE_OPTIONS=""
SYMMETRY="-DEVEN"

while getopts "w:p:r:m:v:oah" o; do
    case "${o}" in
        w)
            WIDTH=${OPTARG}
            ;;
        p)
            MAXPERIOD=${OPTARG}
            ;;
        r)
            rulearg=${OPTARG}
            ;;
        m)
            MAX_TREE_SIZE=${OPTARG}
            ;;
        v)
            VERBOSITY=${OPTARG}
            ;;
        o)
            MORE_OPTIONS="-DONE_SHIP_PER_RULE"
            ;;
        a)
            SYMMETRY=" "
            ;;
        h)
            printf "$usage\n"
            exit 0
            ;;
        
        *)
            printf "$usage\n"
            exit 1
            ;;
    esac
done

set -e

if ((${#rulearg} == 0)); then
echo "Rule unspecified; compiling mpps without parallel evolution..."

echo "Options used: DM_MAX_PERIOD=$MAXPERIOD DM_W=$WIDTH DM_MAX_TREE_SIZE=$MAX_TREE_SIZE DM_VERBOSITY=$VERBOSITY $MORE_OPTIONS $SYMMETRY -DMACRO_OPTIONS"
$compiler mpps.cpp -O3 -march=native -Wall -Wextra -std=c++20 -o mpps -DM_MAX_PERIOD=$MAXPERIOD -DM_W=$WIDTH -DM_MAX_TREE_SIZE=$MAX_TREE_SIZE -DM_VERBOSITY=$VERBOSITY $MORE_OPTIONS $SYMMETRY -DMACRO_OPTIONS

echo ""
echo "**** Build process completed successfully ****"
exit 0
fi


if [ ! -f "genrc" ]; then
echo "Compiling genrulecode..."
$compiler genrulecode.cpp -O3 -march=native -Wall -Wextra -std=c++20 -o genrc
fi

# Run genrc.a to make sure it doesn't crash due to bad input
./genrc $rulearg >/dev/null

echo "Generating code..."
./genrc $rulearg > pevo.h

echo "Compiling mpps for rule $rulearg..."

echo "Options used: DPARALLEL_EVOLVE DM_MAX_PERIOD=$MAXPERIOD DM_W=$WIDTH DM_MAX_TREE_SIZE=$MAX_TREE_SIZE DM_VERBOSITY=$VERBOSITY $MORE_OPTIONS $SYMMETRY DMACRO_OPTIONS"
$compiler mpps.cpp -O3 -march=native -Wall -Wextra -std=c++20 -o mpps -DPARALLEL_EVOLVE -DM_MAX_PERIOD=$MAXPERIOD -DM_W=$WIDTH -DM_MAX_TREE_SIZE=$MAX_TREE_SIZE -DM_VERBOSITY=$VERBOSITY $MORE_OPTIONS $SYMMETRY -DMACRO_OPTIONS

echo ""
echo "**** Build process completed successfully ****"
