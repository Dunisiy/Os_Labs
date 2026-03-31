#!/bin/bash

# Универсальные пути
SRC_DIR="$HOME/Downloads/check"
DST_DIR="$HOME/Downloads/copy"

# Создаем директории
mkdir -p "$SRC_DIR" "$DST_DIR"

# Список файлов
FILES=(
    "4KB:4096"
    "64KB:65536"
    "512KB:524288"
    "5MB:5242880"
    "10MB:10485760"
    "20MB:20971520"
    "50MB:52428800"
    "100MB:104857600"
    "200MB:209715200"
    "512MB:536870912"
    "1GB:1073741824"
)

echo "Создание ZIP-файлов в $SRC_DIR"

# Создаем файлы
for file_info in "${FILES[@]}"; do
    IFS=':' read -r name size <<< "$file_info"
    filename="$SRC_DIR/${name}.zip"
    
    echo -n "Создаю ${name}.zip... "
    
    # Создаем временный файл со случайными данными
    temp_file="/tmp/temp_$$.bin"
    dd if=/dev/urandom of="$temp_file" bs=1M count=$((size / 1048576)) 2>/dev/null
    remainder=$((size % 1048576))
    [ $remainder -gt 0 ] && dd if=/dev/urandom of="$temp_file" bs=1 count=$remainder seek=$((size - remainder)) 2>/dev/null
    
    # Создаем ZIP без сжатия
    zip -0 -j "$filename" "$temp_file" >/dev/null 2>&1
    
    # Удаляем временный файл
    rm -f "$temp_file"
    
    echo "готово ($(du -h "$filename" | cut -f1))"
done

echo ""
echo "Готово! Файлы в $SRC_DIR:"
ls -lh "$SRC_DIR"/*.zip
echo ""
echo "Общий размер: $(du -sh "$SRC_DIR" | cut -f1)"
