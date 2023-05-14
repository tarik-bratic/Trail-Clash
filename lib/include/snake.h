#ifndef snake_h
#define snake_h

typedef struct snake Snake;
typedef struct snakeData SnakeData;

Snake *create_snake(SDL_Renderer *pRenderer, int number, int color);
void update_snake(Snake *pSnke, Snake **otherSnakes, int nrOfSnakes, int key);
void reset_snake(Snake *pSnke, int snakeNum);
void turn_left(Snake *pSnke);
void turn_right(Snake *pSnke);
void draw_snake(Snake *pSnke);
void draw_trail(Snake *pSnke);
void destroy_snake(Snake *pSnke);
void update_snakeData(Snake *pSnke, SnakeData *pSnkeData);
void update_recived_snake_data(Snake *pSnke, SnakeData *pSnkeData);
int collideSnake(Snake *pSnake, SDL_Rect rect);

#endif