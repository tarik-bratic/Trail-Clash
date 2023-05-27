CURRETN_OS := windows
ifneq ($(OS), Windows_NT)
	UNAME := $(shell uname)
	ifeq ($(UNAME), Linux)
		CURRETN_OS := linux
	endif
	ifeq ($(UNAME), Darwin)
		CURRETN_OS := mac
	endif
endif

INCLUDE = C:\msys64\mingw64\include
LDFLAGS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_net -lSDL2_mixer -lm -g
LDFLAGS_WINDOWS := -lmingw32
LDFLAGS_MAC := -L /opt/homebrew/lib

ifeq ($(CURRETN_OS), windows)
	LDFLAGS := $(LDFLAGS_WINDOWS) $(LDFLAGS)
endif

ifeq ($(CURRETN_OS), mac)
	LDFLAGS := $(LDFLAGS_MAC) $(LDFLAGS)
endif

server: main.o text.o snake.o sdl_init.o
	gcc -o server main.o text.o snake.o sdl_init.o $(LDFLAGS)

main.o: main.c
	gcc -c main.c

snake.o: ../lib/src/snake.c ../lib/include/snake.h
	gcc -c ../lib/src/snake.c

text.o: ../lib/src/text.c ../lib/include/text.h
	gcc -c ../lib/src/text.c

sdl_init.o: ../lib/src/sdl_init.c ../lib/include/sdl_init.h 
	gcc -c ../lib/src/sdl_init.c

clean:
	rm *.o
	rm *.exe