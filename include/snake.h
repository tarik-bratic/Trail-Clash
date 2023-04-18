#ifndef snake_h
#define snake_h

typedef struct snake Snake;

Snake *create_snake(int x, int y, SDL_Renderer *pRenderer, int window_width, int window_height);
void update_snake(Snake *pSnke);
void turn_left(Snake *pSnke);
void turn_right(Snake *pSnke);
void draw_snake(Snake *pSnke);
void destroy_snake(Snake *pSnke);

//void accelerate(Snake *pSnke);

#endif