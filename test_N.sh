#!/bin/bash

SRC_DIR="/home/denis/Downloads/check"
DST_DIR="/home/denis/Downloads/copy"

FILES=("4KB.zip" "64KB.zip" "512KB.zip" "5MB.zip" "10MB.zip" "20MB.zip" "50MB.zip" "100MB.zip" "200MB.zip" "512MB.zip" "1GB.zip")
N_VALUES=(1 2 4 8 16 24 32)

PROGRAM="copy_aio"

echo "Компиляция..."
g++ -O2 -pthread -lrt LinuxMain.cpp -o $PROGRAM
if [ $? -ne 0 ]; then
    echo "Ошибка компиляции"
    exit 1
fi

mkdir -p "$DST_DIR"

echo
echo "=========================================="
echo "ТЕСТ КОПИРОВАНИЯ (AIO)"
echo "=========================================="
echo


printf "%-16s" "Файл"
for n in "${N_VALUES[@]}"; do
    printf "%-12s" "n=$n"
done
echo

for file in "${FILES[@]}"; do
    printf "%-12s" "$file"

    for n in "${N_VALUES[@]}"; do
        SRC_FILE="$SRC_DIR/$file"

        name="${file%.*}"
        ext="${file##*.}"
        DST_FILE="$DST_DIR/${name}_n${n}.${ext}"

        rm -f "$DST_FILE"

        TIME=$(./$PROGRAM "$SRC_FILE" "$DST_FILE" $n 2>/dev/null | grep "Done in" | awk '{print $3}')

        if [ -z "$TIME" ]; then
            TIME="ERR"
        fi

        printf "%-12s" "$TIME"
    done
    echo
done

echo
echo "=========================================="
echo "ПРОВЕРКА КОРРЕКТНОСТИ"
echo "=========================================="
echo

ALL_OK=1

for file in "${FILES[@]}"; do
    SRC_FILE="$SRC_DIR/$file"

    for n in "${N_VALUES[@]}"; do
        name="${file%.*}"
        ext="${file##*.}"
        DST_FILE="$DST_DIR/${name}_n${n}.${ext}"

        if cmp -s "$SRC_FILE" "$DST_FILE"; then
            echo "OK   $file (n=$n)"
        else
            echo "FAIL $file (n=$n)"
            ALL_OK=0
        fi
    done
done

echo
echo "=========================================="

if [ $ALL_OK -eq 1 ]; then
    echo "ВСЕ ФАЙЛЫ СОВПАДАЮТ ✅"
else
    echo "ЕСТЬ ОШИБКИ ❌"
fi
