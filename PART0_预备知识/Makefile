# Makefile for PART0 — C语言的面向对象写法
#
# 变量说明：
#   CC      = C编译器，通常是gcc或clang
#   CFLAGS  = 编译选项
#             -Wall    开启所有警告
#             -Wextra  开启额外警告
#             -std=c11 使用C11标准（C语言版，本项目用）
#             -g       包含调试信息（用于gdb）
#   TARGET  = 编译输出的可执行文件名
#   SRCS    = 源文件列表（.c文件）
#   OBJ     = 目标文件列表（.o文件，由SRCS替换生成）

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -g
TARGET  := test
SRCS    := code.c
OBJ     := $(SRCS:.c=.o)

# 默认目标：编译并链接
all: $(TARGET)

# 链接：把目标文件合并成可执行文件
# $(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
#         编译器  编译选项  目标文件   输出文件名
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	@echo "✅ 编译完成：$(TARGET)"

# 编译：把每个.c文件编译成.o目标文件
# $< 是自动变量，代表依赖列表中的第一个（即正在编译的.c文件）
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  编译: $< -> $@"

# 运行测试
run: $(TARGET)
	@echo "▶ 运行测试..."
	@echo ""
	./$(TARGET)
	@echo ""
	@echo "✅ 测试完成"

# 清理：删除编译产物
# -f 忽略文件不存在的错误
# -r 递归删除目录
clean:
	rm -f $(TARGET) $(OBJ)
	@echo "🧹 清理完成"

# 重新编译：先清理再编译
rebuild: clean all

# 显示帮助
help:
	@echo "PART0 — Makefile 使用说明"
	@echo "=============================="
	@echo "make        编译（默认）"
	@echo "make run    编译并运行"
	@echo "make clean  清理编译产物"
	@echo "make rebuild 先清理再编译"
	@echo "make help   显示本帮助"

.PHONY: all run clean rebuild help
