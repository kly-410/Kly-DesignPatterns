# Makefile — State Pattern
CC := gcc
CFLAGS := -Wall -Wextra -g -O0
TARGET := state_demo
SRCS := code.c
OBJS := $(SRCS:.c=.o)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "✅  编译成功: $(TARGET)"
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
.PHONY: test clean rebuild
test: $(TARGET)
	./$(TARGET)
clean:
	rm -f $(TARGET) $(OBJS)
rebuild: clean $(TARGET)
