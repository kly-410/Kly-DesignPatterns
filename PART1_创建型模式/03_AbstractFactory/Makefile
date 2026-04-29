# ============================================================
# PART1/03_AbstractFactory/Makefile — 抽象工厂模式
#
# 本章节演示：创建一族相关对象，而无需指定具体类
# 抽象工厂在嵌入式场景常用于：
#   不同芯片系列的外设初始化（STM32 系列、NXP 系列）
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

TARGET = abstract_factory_test

# 源文件列表：
#   main.c     — 测试入口
#   linux_ui.c — Linux 风格 UI 实现（LinuxButton + LinuxMenu）
#   mac_ui.c   — Mac 风格 UI 实现（MacButton + MacMenu）
OBJECTS = src/main.o \
          src/linux_ui.o \
          src/mac_ui.o

# ------------------------------------------------------------
# 四、规则定义
# ------------------------------------------------------------

.PHONY: all clean run

all: $(TARGET)

# 链接
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# main.o：依赖 main.c + 工厂和产品头文件
src/main.o: src/main.c include/ui_factory.h include/linux_ui.h include/mac_ui.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# linux_ui.o：依赖 linux_ui.c + 抽象产品和工厂
src/linux_ui.o: src/linux_ui.c include/linux_ui.h include/ui_product.h include/ui_factory.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# mac_ui.o：依赖 mac_ui.c + 抽象产品和工厂
src/mac_ui.o: src/mac_ui.c include/mac_ui.h include/ui_product.h include/ui_factory.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------------------------------------
# 五、运行与清理
# ------------------------------------------------------------

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)
