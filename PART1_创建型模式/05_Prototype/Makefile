# ============================================================
# PART1/05_Prototype/Makefile — 原型模式
#
# 本章节演示：通过复制现有对象创建新对象，无需知道具体类型
# 原型模式在嵌入式场景常用于：
#   固件配置克隆（一份基准配置，复制后修改局部参数）
#   测试用例模板（复制模板生成多个测试用例）
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

TARGET = prototype_test

# 源文件：
#   main.c               — 测试入口
#   network_config.c     — 可克隆的网络配置对象
#   prototype_manager.c  — 原型管理器（存储和克隆原型）
OBJECTS = src/main.o \
          src/network_config.o \
          src/prototype_manager.o

# ------------------------------------------------------------
# 四、规则定义
# ------------------------------------------------------------

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# main.o：依赖 main.c + 原型相关头文件
src/main.o: src/main.c \
    include/network_config.h include/prototype_manager.h include/config_prototype.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# network_config.o：依赖源文件和原型接口
src/network_config.o: src/network_config.c include/network_config.h include/config_prototype.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# prototype_manager.o：依赖管理器源文件 + 原型接口
src/prototype_manager.o: src/prototype_manager.c include/prototype_manager.h include/config_prototype.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ------------------------------------------------------------
# 五、运行与清理
# ------------------------------------------------------------

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)
