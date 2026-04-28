# Makefile for 01_Strategy — 策略模式
#
# 变量说明：
#   CC      = C编译器（gcc）
#   CFLAGS  = 编译选项
#             -Wall      开启所有警告
#             -Wextra    开启额外警告
#             -std=c11   使用C11标准
#             -g         包含调试信息
#             -lm        链接数学库（brightness_exp里用到exp函数）
#   TARGET  = 编译输出的可执行文件名
#   SRCS    = 源文件列表
#   OBJ     = 目标文件列表（.o）

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -g
LDFLAGS := -lm          # 数学库：exp函数需要
TARGET  := test
SRCS    := code.c
OBJ     := $(SRCS:.c=.o)

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET)
	@echo "✅ 编译完成：$(TARGET)"

# 编译.c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  编译: $<"

# 运行
run: $(TARGET)
	@echo "▶ 运行..."
	@echo ""
	./$(TARGET)
	@echo ""
	@echo "✅ 完成"

# 清理
clean:
	rm -f $(TARGET) $(OBJ)
	@echo "🧹 清理完成"

# 重新编译
rebuild: clean all

.PHONY: all run clean rebuild
