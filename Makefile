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
LDFLAGS = -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lm
LDFLAGS_WINDOWS := -lmingw32 -mwindows
LDFLAGS_MAC := -L /opt/homebrew/lib

ifeq ($(CURRETN_OS), windows)
	LDFLAGS := $(LDFLAGS_WINDOWS) $(LDFLAGS)
endif

ifeq ($(CURRETN_OS), mac)
	LDFLAGS := $(LDFLAGS_MAC) $(LDFLAGS)
endif

trailClash: main.o snake.o
	gcc -o trailClash main.o snake.o $(LDFLAGS)

main.o: ./src/main.c
	gcc -c ./src/main.c

snake.o: ./src/snake.c ./include/snake.h
	gcc -c ./src/snake.c

clean:
	rm *.o
	rm *.exe