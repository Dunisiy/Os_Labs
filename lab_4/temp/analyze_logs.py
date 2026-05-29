import os
import re
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

# === CONFIG ===
LOG_DIR = r"C:\Users\denis\source\repos\GIT_ETU_4314\OS\lab\Kuznetsov_DS\lab_4\temp"
NUM_PAGES = 9

# Regex исправлен:
# \s* обрабатывает лишние пробелы вокруг разделителей |
# ([A-Z_]+) захватывает состояние, strip() уберет остаточные пробелы
pattern = re.compile(
    r"(\d+)\s*\|\s*(READER|WRITER)_(\d+)\s*\|\s*([A-Z_]+)\s*(?:\|\s*PAGE\s*(\d+))?"
)

records = []

# =========================================================
# LOAD LOGS
# =========================================================
print("Loading logs...")
for file in os.listdir(LOG_DIR):
    if not file.endswith(".log"):
        continue
    
    path = os.path.join(LOG_DIR, file)
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = pattern.search(line)
            if not m:
                continue
            
            # Очищаем состояние от пробелов
            state = m.group(4).strip()
            
            records.append({
                "time": int(m.group(1)),
                "type": m.group(2),
                "pid": m.group(3),
                "proc": f"{m.group(2)}_{m.group(3)}",
                "state": state,
                "page": int(m.group(5)) if m.group(5) else None
            })

df = pd.DataFrame(records)
if df.empty:
    print("No logs found. Check LOG_DIR.")
    exit()

# Нормализация времени относительно начала (минимального времени в логах)
min_time = df["time"].min()
df["time"] -= min_time

# Сортировка для корректного расчета интервалов
df = df.sort_values(by=["proc", "time"])

# =========================================================
# COLORS
# =========================================================
COLORS = {
    "WAIT_WRITE": "#ffb347", # Orange
    "WRITING": "#ff4d4d",   # Red
    "WAIT_READ": "#87cefa", # Light Blue
    "READING": "#1e90ff",   # Blue
    "LAST_READER": "#9370db",
    "RELEASE": "#32cd32",
    "FINISHED": "#555555",
    "IDLE": "#eeeeee"       # Light Grey
}

processes = sorted(df["proc"].unique())
proc_y = {p: i for i, p in enumerate(processes)}

# =========================================================
# 1. PROCESS TIMELINE
# =========================================================
print("Plotting Timeline...")
plt.figure(figsize=(20, 8))

for proc in processes:
    p_df = df[df["proc"] == proc]
    rows = p_df.to_dict("records")
    
    # Добавляем фиктивную последнюю запись для замыкания последнего интервала
    # (если процесс не завершился корректно, он просто закончится на последнем событии)
    
    for i in range(len(rows) - 1):
        curr = rows[i]
        nxt = rows[i + 1]
        
        start = curr["time"]
        end = nxt["time"]
        state = curr["state"]
        duration = end - start
        
        # Если событие "DONE" или "RELEASE" — это маркер окончания, 
        # его длительность пренебрежимо мала, поэтому не рисуем линию.
        if state in ["WRITE_DONE", "RELEASE", "LAST_READER", "FINISHED", "START"]:
            continue
        
        # Определяем цвет
        color = COLORS.get(state, COLORS["IDLE"])
        
        # Если состояние не "WAIT" и интервал слишком маленький (< 50мс), считаем это шумом/оверфловом
        if "WAIT" not in state and duration < 50:
            continue
            
        # Если состояние "WAIT" — это Idle
        if "WAIT" in state:
            color = COLORS["IDLE"]

        plt.hlines(
            y=proc_y[proc],
            xmin=start,
            xmax=end,
            linewidth=10, # Толщина линии
            color=color
        )

# Легенда
legend_patches = [
    Patch(color=COLORS["WRITING"], label="Writing"),
    Patch(color=COLORS["WAIT_WRITE"], label="Writer Wait"),
    Patch(color=COLORS["READING"], label="Reading"),
    Patch(color=COLORS["WAIT_READ"], label="Reader Wait"),
    Patch(color=COLORS["IDLE"], label="Idle/Transition")
]
plt.legend(handles=legend_patches, loc="upper right")
plt.yticks(range(len(processes)), processes)
plt.title("Process Timeline")
plt.xlabel("Time (ms)")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()

# =========================================================
# 2. PAGE OCCUPANCY (Gantt Chart for Pages)
# =========================================================
print("Plotting Page Occupancy...")
plt.figure(figsize=(20, 6))

# Рисуем писателей (Красные полоски)
for proc in processes:
    p_df = df[df["proc"] == proc]
    rows = p_df.to_dict("records")
    
    for i in range(len(rows) - 1):
        curr = rows[i]
        nxt = rows[i + 1]
        
        if curr["state"] == "WRITING":
            start = curr["time"]
            end = nxt["time"] # Запись заканчивается, когда приходит лог WRITE_DONE
            page = curr["page"]
            
            if page is not None:
                plt.hlines(y=page, xmin=start, xmax=end, linewidth=8, color="#ff4d4d", zorder=2)

# Рисуем читателей (Синие полоски)
# ВАЖНО: Мы рисуем их ПОВЕРХ писателей, чтобы видеть нарушения синхронизации (наложения)
for proc in processes:
    p_df = df[df["proc"] == proc]
    rows = p_df.to_dict("records")
    
    for i in range(len(rows) - 1):
        curr = rows[i]
        nxt = rows[i + 1]
        
        if curr["state"] == "READING":
            start = curr["time"]
            end = nxt["time"] # Чтение заканчивается на RELEASE/LAST_READER
            page = curr["page"]
            
            if page is not None:
                # Чуть тоньше, чтобы красное было видно, если синхронизация нарушена
                plt.hlines(y=page, xmin=start, xmax=end, linewidth=4, color="#1e90ff", zorder=3)

legend_patches = [
    Patch(color="#ff4d4d", label="Writer Active"),
    Patch(color="#1e90ff", label="Reader Active")
]
plt.legend(handles=legend_patches, loc="upper right")
plt.title("Shared Memory Page Occupancy (Overlap = Bug)")
plt.xlabel("Time (ms)")
plt.ylabel("Page Number")
plt.yticks(range(NUM_PAGES))
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()

# =========================================================
# 3. CONCURRENCY HEATMAP
# =========================================================
print("Plotting Heatmap...")
TIME_STEP = 50 # Шаг дискретизации (мс)

max_time = int(df["time"].max())
num_bins = int(max_time / TIME_STEP) + 1

# Массив [Страница, Время]
heatmap = np.zeros((NUM_PAGES, num_bins))

for proc in processes:
    p_df = df[df["proc"] == proc]
    rows = p_df.to_dict("records")
    
    for i in range(len(rows) - 1):
        curr = rows[i]
        nxt = rows[i + 1]
        
        # Нас интересуют только активные действия
        if curr["state"] in ["WRITING", "READING"]:
            if curr["page"] is None:
                continue
                
            start = curr["time"]
            end = nxt["time"]
            page = int(curr["page"])
            
            # Рассчитываем индексы бинов
            start_bin = int(start / TIME_STEP)
            end_bin = int(end / TIME_STEP)
            
            # Заполняем диапазон
            # +1 чтобы включить последний интервал
            if start_bin < num_bins:
                # Ограничиваем верхний индекс размером массива
                limit_end = min(end_bin + 1, num_bins)
                heatmap[page, start_bin:limit_end] += 1

plt.figure(figsize=(20, 6))
plt.imshow(heatmap, aspect='auto', origin='lower', cmap='inferno') # inferno хорош для интенсивности

plt.title("Concurrency Heatmap (Intensity = Number of Accesses)")
plt.xlabel("Time (bins of 50ms)")
plt.ylabel("Page Number")
plt.yticks(range(NUM_PAGES))
plt.colorbar(label="Active Processes on Page")
plt.tight_layout()
plt.show()