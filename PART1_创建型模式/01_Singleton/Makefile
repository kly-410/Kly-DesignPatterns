CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE
LDFLAGS = -pthread

# 源文件
SRC_DIR = src
INC_DIR = include

# 目标文件
TARGET = singleton_test

# 头文件目录
INCLUDES = -I$(INC_DIR)

# 源文件列表
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/singleton_pthread_once.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)