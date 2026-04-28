# 04_Decorator/Makefile - 装饰器模式编译脚本
#
# 编译目标：decorator
# 编译命令：make
# 清理命令：make clean

# ============================================================
# 目录配置
# ============================================================
SRCDIR  = .
OBJDIR  = ../obj/04_Decorator
BINDIR  = ../bin

# ============================================================
# 编译器配置
# ============================================================
CC      = gcc
CFLAGS  = -Wall -g

# ============================================================
# 目标配置
# ============================================================
EXEC    = $(BINDIR)/decorator
SRC     = $(SRCDIR)/code.c
OBJ     = $(OBJDIR)/code.o

# ============================================================
# 默认目标
# ============================================================
all: dir build

dir:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

build: $(OBJ) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm -rf $(EXEC) $(OBJ)

.PHONY: help
help:
	@echo "04_Decorator - 装饰器模式"
	@echo "Usage: make && ./decorator"
