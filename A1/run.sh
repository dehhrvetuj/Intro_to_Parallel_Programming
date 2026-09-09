#!/bin/bash

# 1. 编译 C++ 程序
#echo "正在编译 performance.cpp..."
#g++ -std=c++11 -Wall -pthread performance.cpp -o performance

#if [ $? -ne 0 ]; then
#    echo "编译失败，请检查 C++ 代码及编译选项！"
#    exit 1
#fi

# 2. 定义参数范围
T_VALUES=(1 2 4 8 16 32)
N_VALUES=(1 2 4 8 16 32)

OUTPUT_FILE="results.csv"

# 3. 初始化 CSV 标头
echo "T,N,Time_sec" > "$OUTPUT_FILE"

echo "开始执行性能测量..."

# 4. 双重循环测试所有组合
for N in "${N_VALUES[@]}"; do
    for T in "${T_VALUES[@]}"; do
        echo -n "运行中: T=$T, N=$N MB ... "
        
        # 执行程序，捕获输出
        # 提示：如果程序输出格式为 "Time: X seconds"，可通过 awk/grep 提取纯数字
        # 这里假设 ./performance N T 的输出中直接包含了运行时间
        EXEC_OUTPUT=$(./performance "$N" "$T")
        
        # 提取输出中的浮点数/数字时间（根据你程序实际输出的字符串格式调微）
        TIME_TAKEN=$(echo "$EXEC_OUTPUT" | grep -oE '[0-9]+(\.[0-9]+)?' | tail -n 1)
        
        # 保存至 CSV
        echo "$T,$N,$TIME_TAKEN" >> "$OUTPUT_FILE"
        echo "完成 (用时: ${TIME_TAKEN}s)"
    done
done

echo "所有测试完成！结果已存入 $OUTPUT_FILE"
