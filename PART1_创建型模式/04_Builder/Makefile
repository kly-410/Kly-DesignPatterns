CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE
TARGET = builder_test

INC_DIR = include
INCLUDES = -I$(INC_DIR)

OBJECTS = src/main.o \
          src/sensor_builder_impl.o \
          src/sensor_director.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

src/main.o: src/main.c include/sensor_builder_impl.h include/sensor_director.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

src/sensor_builder_impl.o: src/sensor_builder_impl.c \
    include/sensor_builder_impl.h include/sensor_builder.h include/sensor_config.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

src/sensor_director.o: src/sensor_director.c include/sensor_director.h \
    include/sensor_builder_impl.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)