CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE
TARGET = factory_method_test

INC_DIR = include
INCLUDES = -I$(INC_DIR)

# 显式列出所有源文件和对应的目标文件
OBJECTS = src/main.o \
          src/nvme_device.o \
          src/nvme_factory.o \
          src/eth_device.o \
          src/eth_factory.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# 显式规则：main.o
src/main.o: src/main.c include/device_factory.h include/nvme_factory.h include/eth_factory.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 显式规则：nvme_device.o
src/nvme_device.o: src/nvme_device.c include/nvme_device.h include/pcie_device.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 显式规则：nvme_factory.o
src/nvme_factory.o: src/nvme_factory.c include/nvme_factory.h include/device_factory.h include/nvme_device.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 显式规则：eth_device.o
src/eth_device.o: src/eth_device.c include/eth_device.h include/pcie_device.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 显式规则：eth_factory.o
src/eth_factory.o: src/eth_factory.c include/eth_factory.h include/device_factory.h include/eth_device.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)