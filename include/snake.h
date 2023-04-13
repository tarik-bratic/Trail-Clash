#ifndef snake_h
#define snake_h

typedef struct snake Snake;

Snake *createSnake(int x, int y, SDL_Renderer *pRenderer, int window_width, int window_height);
void updateRocket(Snake *pSnk);
void accelerate(Snake *pSnke);
void turnLeft(Snake *pSnke);
void turnRight(Snake *pSnke);
void drawRocket(Snake *pSnk);
void destroyRocket(Snake *pSnk);

#endif