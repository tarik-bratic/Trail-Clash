INCLUDE = C:\msys64\mingw64\include
LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -mwindows -lm

trailClash: main.o
	gcc -o trailClash main.o $(LDFLAGS)

main.o: ./src/main.c
	gcc -c ./src/main.c

clean:
	rm *.o
	rm *.exe