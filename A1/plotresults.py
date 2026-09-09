import pandas as pd
import matplotlib.pyplot as plt

# 1. 读取 CSV 数据
# 假设 results.csv 的列名为: T, N, Time_sec
df = pd.read_csv("results.csv")

# 2. 数据透视：以线程数 T 为行索引，数据量 N 为列索引，运行时间为数值
pivot_df = df.pivot(index="T", columns="N", values="Time_sec")

# 3. 创建画布
plt.figure(figsize=(10, 6), dpi=300)

# 4. 遍历每个数据规模 N 并绘制折线
t_values = [1, 2, 4, 8, 16, 32]
for n_val in pivot_df.columns:
    plt.plot(
        pivot_df.index, 
        pivot_df[n_val], 
        marker='o', 
        linewidth=2, 
        label=f"N = {n_val} MB"
    )

# 5. 设置 X 轴为 log2 缩放，并显示明确的刻度标签
plt.xscale('log', base=2)
plt.xticks(t_values, labels=[str(t) for t in t_values])

# 6. 添加坐标轴标签、标题与图例
plt.xlabel("Number of Threads (T)", fontsize=12, fontweight='bold')
plt.ylabel("Execution Time (seconds)", fontsize=12, fontweight='bold')
plt.title("Performance Benchmark: Execution Time vs. Threads (T)", fontsize=14, fontweight='bold', pad=15)

plt.grid(True, which="both", ls="--", alpha=0.5)
plt.legend(title="Data Size (N)", bbox_to_anchor=(1.05, 1), loc='upper left', frameon=True)

# 7. 自动调整布局并保存图像
plt.tight_layout()
plt.savefig("benchmark_plot.png", dpi=300)
plt.show()
