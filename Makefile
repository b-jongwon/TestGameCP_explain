CC = gcc
CFLAGS = -Wall -Iinclude -pthread
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
TARGET = game

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) $(TARGET)

run: all
	./$(TARGET)
