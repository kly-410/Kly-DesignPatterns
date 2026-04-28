CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE
TARGET = prototype_test

INC_DIR = include
INCLUDES = -I$(INC_DIR)

OBJECTS = src/main.o \
          src/network_config.o \
          src/prototype_manager.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

src/main.o: src/main.c \
    include/network_config.h include/prototype_manager.h include/config_prototype.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

src/network_config.o: src/network_config.c include/network_config.h include/config_prototype.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

src/prototype_manager.o: src/prototype_manager.c include/prototype_manager.h include/config_prototype.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)