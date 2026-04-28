CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE
TARGET = abstract_factory_test

INC_DIR = include
INCLUDES = -I$(INC_DIR)

OBJECTS = src/main.o \
          src/linux_ui.o \
          src/mac_ui.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

src/main.o: src/main.c include/ui_factory.h include/linux_ui.h include/mac_ui.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

src/linux_ui.o: src/linux_ui.c include/linux_ui.h include/ui_product.h include/ui_factory.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

src/mac_ui.o: src/mac_ui.c include/mac_ui.h include/ui_product.h include/ui_factory.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)