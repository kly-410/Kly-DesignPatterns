# 03_Composite/Makefile - 组合模式编译脚本
#
# 编译目标：composite
# 编译命令：make
# 清理命令：make clean

# ============================================================
# 目录配置
# ============================================================
SRCDIR  = .
OBJDIR  = ../obj/03_Composite
BINDIR  = ../bin

# ============================================================
# 编译器配置
# ============================================================
CC      = gcc
CFLAGS  = -Wall -g

# ============================================================
# 目标配置
# ============================================================
EXEC    = $(BINDIR)/composite
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
	@echo "03_Composite - 组合模式"
	@echo "Usage: make && ./composite"
