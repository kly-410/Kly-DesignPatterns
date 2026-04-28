# Makefile — Observer Pattern

CC      := gcc
CFLAGS  := -Wall -Wextra -g -O0
TARGET  := observer_demo
SRCS    := code.c
OBJS    := $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "✅  编译成功: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: test
test: $(TARGET)
	@echo "========================================"
	@echo "  运行 Observer Pattern 演示程序"
	@echo "========================================"
	./$(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)
	@echo "🧹  清理完成"

.PHONY: rebuild
rebuild: clean $(TARGET)
