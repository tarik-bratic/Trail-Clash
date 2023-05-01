#ifndef snake_h
#define snake_h

typedef struct snake Snake;
typedef struct snakeData SnakeData;

Snake *create_snake(int number, SDL_Renderer *pRenderer, int window_width, int window_height);
void update_snake(Snake *pSnke);
void reset_snake(Snake *pSnke);
void turn_left(Snake *pSnke);
void turn_right(Snake *pSnke);
void draw_snake(Snake *pSnke);
void draw_trail(Snake *pSnke);
void destroy_snake(Snake *pSnke);
void update_snakeData(Snake *pSnke, SnakeData *pSnkeData);
void update_recived_snake_data(Snake *pSnke, SnakeData *pSnkeData);

//void accelerate(Snake *pSnke);

#endif