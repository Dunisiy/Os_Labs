#!/bin/bash

SRC="/home/denis/Downloads/check/512MB.zip"
DST_DIR="/home/denis/Downloads/copy"
PROGRAM="./copy_aio"

RUNS=5

N_VALUES=(1 2 4 8 16 32)
BLOCK_VALUES=(4 16 64 256 1024 4096 16384 65536) # KB

mkdir -p "$DST_DIR"

########################################
# ФУНКЦИЯ: запуск с выводом всех замеров
########################################
run_test() {
    local src=$1
    local dst=$2
    local n=$3
    local block=$4

    local sum=0
    local times=()

    for ((i=1;i<=RUNS;i++)); do
        rm -f "$dst"

        OUT=$($PROGRAM "$src" "$dst" $n $block 2>&1)
        TIME=$(echo "$OUT" | awk '/Done in/ {print $3}')

        if [ -z "$TIME" ]; then
            echo "ERR"
            return
        fi

        times+=("$TIME")
        sum=$(awk "BEGIN {print $sum + $TIME}")
    done

    avg=$(awk "BEGIN {printf \"%.6f\", $sum / $RUNS}")

    # вывод строки
    printf "%-8s" "$1"  # будет заменено снаружи
    for t in "${times[@]}"; do
        printf "%-10s" "$t"
    done
    printf "%-10s" "$avg"
    echo
}

########################################
# ТЕСТ 1: влияние n
########################################
echo "=========================================="
echo ">>> Влияние количества операций (block = 64KB)"
echo "=========================================="

printf "%-8s %-10s %-10s %-10s %-10s %-10s %-10s\n" "n" "run1" "run2" "run3" "run4" "run5" "avg"

for n in "${N_VALUES[@]}"; do
    DST="$DST_DIR/test_n${n}.zip"

    sum=0
    times=()

    for ((i=1;i<=RUNS;i++)); do
        rm -f "$DST"

        OUT=$($PROGRAM "$SRC" "$DST" $n 64 2>&1)
        TIME=$(echo "$OUT" | awk '/Done in/ {print $3}')

        if [ -z "$TIME" ]; then
            TIME="ERR"
        fi

        times+=("$TIME")

        if [ "$TIME" != "ERR" ]; then
            sum=$(awk "BEGIN {print $sum + $TIME}")
        fi
    done

    avg=$(awk "BEGIN {printf \"%.6f\", $sum / $RUNS}")

    printf "%-8s" "$n"
    for t in "${times[@]}"; do
        printf "%-10s" "$t"
    done
    printf "%-10s\n" "$avg"
done

echo
echo "=========================================="
echo ">>> Влияние размера блока (n = 8)"
echo "=========================================="

printf "%-10s %-10s %-10s %-10s %-10s %-10s %-10s\n" "blockKB" "run1" "run2" "run3" "run4" "run5" "avg"

for b in "${BLOCK_VALUES[@]}"; do
    DST="$DST_DIR/test_b${b}.zip"

    sum=0
    times=()

    for ((i=1;i<=RUNS;i++)); do
        rm -f "$DST"

        OUT=$($PROGRAM "$SRC" "$DST" 8 $b 2>&1)
        TIME=$(echo "$OUT" | awk '/Done in/ {print $3}')

        if [ -z "$TIME" ]; then
            TIME="ERR"
        fi

        times+=("$TIME")

        if [ "$TIME" != "ERR" ]; then
            sum=$(awk "BEGIN {print $sum + $TIME}")
        fi
    done

    avg=$(awk "BEGIN {printf \"%.6f\", $sum / $RUNS}")

    printf "%-10s" "$b"
    for t in "${times[@]}"; do
        printf "%-10s" "$t"
    done
    printf "%-10s\n" "$avg"
done

echo
echo "=========================================="
echo "ГОТОВО"
echo
