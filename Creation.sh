#!/bin/bash

# Скрипт для проверки созданных файлов
SRC_DIR="$HOME/Downloads/check"

echo "=== Проверка ZIP-файлов в $SRC_DIR ==="
echo ""

# Ожидаемые размеры
declare -A EXPECTED_SIZES=(
    ["4KB.zip"]=4096
    ["64KB.zip"]=65536
    ["512KB.zip"]=524288
    ["5MB.zip"]=5242880
    ["10MB.zip"]=10485760
    ["20MB.zip"]=20971520
    ["50MB.zip"]=52428800
    ["100MB.zip"]=104857600
    ["200MB.zip"]=209715200
    ["512MB.zip"]=536870912
    ["1GB.zip"]=1073741824
)

for file in "${!EXPECTED_SIZES[@]}"; do
    filepath="$SRC_DIR/$file"
    expected="${EXPECTED_SIZES[$file]}"
    
    if [ -f "$filepath" ]; then
        # Получаем размер файла в байтах
        if [[ "$OSTYPE" == "darwin"* ]]; then
            actual=$(stat -f%z "$filepath")
        else
            actual=$(stat -c%s "$filepath")
        fi
        
        actual_display=$(du -h "$filepath" | cut -f1)
        expected_display=$(numfmt --to=iec "$expected" 2>/dev/null || echo "$expected")
        
        if [ "$actual" -eq "$expected" ]; then
            echo "✓ $file: $actual_display (OK)"
        else
            echo "✗ $file: $actual_display (ожидалось: $expected_display)"
        fi
    else
        echo "✗ $file: не найден"
    fi
done