# ============================================================
# PART1/04_Builder/Makefile — 建造者模式
#
# 本章节演示：分步构建复杂对象，步骤顺序固定，细节可定制
# 建造者在嵌入式场景常用于：
#   传感器配置包构建（ID + 类型 + 校准参数 + 数据格式）
#   网络协议包构建（Header + Payload + CRC + Padding）
#   PCIe TLP 包构建
# ============================================================

# ------------------------------------------------------------
# 一、编译器与编译选项
# ------------------------------------------------------------

CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE

# ------------------------------------------------------------
# 二、目录与路径
# ------------------------------------------------------------

INC_DIR = include
INCLUDES = -I$(INC_DIR)

# ------------------------------------------------------------
# 三、目标配置
# ------------------------------------------------------------

TARGET = builder_test

# 源文件：
#   main.c              — 测试入口
#   sensor_builder_impl.c — 具体建造者实现（实现 Builder 接口）
#   sensor_director.c   — 指挥者（控制构建顺序）
OBJECTS = src/main.o \
          src/sensor_builder_impl.o \
          src/sensor_director.o

# ------------------------------------------------------------
# 四、规则定义
# ------------------------------------------------------------

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# main.o：依赖 main.c + 建造者和指挥者头文件
src/main.o: src/main.c include/sensor_builder_impl.h include/sensor_director.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# sensor_builder_impl.o：依赖源文件 + 3 个头文件
# 注意：sensor_builder.h 是抽象基类，sensor_builder_impl.h 是具体实现
src/sensor_builder_impl.o: src/sensor_builder_impl.c \
    include/sensor_builder_impl.h include/sensor_builder.h include/sensor_config.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# sensor_director.o：依赖指挥者源文件 + 建造者头文件
src/sensor_director.o: src/sensor_director.c include/sensor_director.h \
    include/sensor_builder_impl.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------------------------------------
# 五、运行与清理
# ------------------------------------------------------------

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)
