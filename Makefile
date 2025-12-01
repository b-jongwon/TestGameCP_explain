CC = gcc
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LDLIBS := $(shell sdl2-config --libs 2>/dev/null)

ifeq ($(SDL_CFLAGS),)
SDL_CFLAGS = -I/usr/include/SDL2 -D_REENTRANT
endif

ifeq ($(SDL_LDLIBS),)
SDL_LDLIBS = -lSDL2
endif

CFLAGS = -Wall -I./include $(SDL_CFLAGS) -pthread
LDFLAGS = $(SDL_LDLIBS) -lSDL2_image -pthread -lm

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d)

TARGET = game

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

clean:
	rm -f $(OBJ) $(DEP) $(TARGET)

run: all
	./$(TARGET)

-include $(DEP)
