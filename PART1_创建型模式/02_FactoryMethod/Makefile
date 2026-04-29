# ============================================================
# PART1/02_FactoryMethod/Makefile — 工厂方法模式
#
# 本章节演示：定义一个创建对象的接口，让子类决定实例化哪个类
# 工厂方法在嵌入式场景常用于：根据硬件型号创建不同驱动实例
# ============================================================

# ------------------------------------------------------------
# 一、编译器与编译选项
# ------------------------------------------------------------

CC = gcc

# -Wall -Wextra -g 同前
# -D_GNU_SOURCE 启用 GNU 扩展
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE

# ------------------------------------------------------------
# 二、目录与路径
# ------------------------------------------------------------

INC_DIR = include
INCLUDES = -I$(INC_DIR)

# ------------------------------------------------------------
# 三、目标配置
# ------------------------------------------------------------

# TARGET = 可执行文件名
TARGET = factory_method_test

# OBJECTS = 所有目标文件（显式列出，便于阅读和添加依赖）
# 换行符 \ 续行，每行末尾加 \ 表示下一行是续接
OBJECTS = src/main.o \
          src/nvme_device.o \
          src/nvme_factory.o \
          src/eth_device.o \
          src/eth_factory.o

# ------------------------------------------------------------
# 四、规则定义
# ------------------------------------------------------------

.PHONY: all clean run

all: $(TARGET)

# 链接：把所有 .o 文件链接成可执行文件
# $^ = 所有依赖目标（即所有 .o 文件）
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# ------------------------------------------------------------
# 五、显式编译规则（Explicit Rules）
#
# 显式规则 vs 隐式规则：
#   隐式规则（Implicit）：%.o: %.c，Make 自动推导
#   显式规则（Explicit）：明确写出每个 .o 依赖哪些 .h
#
# 为什么要用显式规则？
#   因为可以声明 .h 依赖，Make 知道何时需要重新编译
#   比如：修改了 nvme_device.h，那么 nvme_device.o 会自动重新编译
# ------------------------------------------------------------

# main.o 依赖：main.c 和 3 个头文件
src/main.o: src/main.c include/device_factory.h include/nvme_factory.h include/eth_factory.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# nvme_device.o 依赖：nvme_device.c 和它的头文件
src/nvme_device.o: src/nvme_device.c include/nvme_device.h include/pcie_device.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# nvme_factory.o 依赖：nvme_factory.c 和相关头文件
src/nvme_factory.o: src/nvme_factory.c include/nvme_factory.h include/device_factory.h include/nvme_device.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# eth_device.o 依赖
src/eth_device.o: src/eth_device.c include/eth_device.h include/pcie_device.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# eth_factory.o 依赖
src/eth_factory.o: src/eth_factory.c include/eth_factory.h include/device_factory.h include/eth_device.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------------------------------------
# 六、运行与清理
# ------------------------------------------------------------

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)

# ------------------------------------------------------------
# 七、常用命令
# ------------------------------------------------------------
# make        — 编译
# make run    — 编译并运行
# make clean  — 清理
#
# 显式规则的好处：当修改某个 .h 文件时，
# make 会自动重新编译所有依赖它的 .c 文件
