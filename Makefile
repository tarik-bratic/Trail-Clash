INCLUDE = C:\msys64\mingw64\include
LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -mwindows -lm

trailClash: main.o snake.o
	gcc -o trailClash main.o snake.o $(LDFLAGS)

main.o: ./src/main.c
	gcc -c ./src/main.c

snake.o: ./src/snake.c ./include/snake.h
	gcc -c ./src/snake.c

clean:
	rm *.o
	rm *.exe