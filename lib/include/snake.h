#ifndef snake_h
#define snake_h

typedef struct snake Snake;
typedef struct snakeData SnakeData;

Snake *create_snake(int number, SDL_Renderer *pRenderer, int window_width, int window_height, int color);
void update_snake(Snake *pSnke, Snake **otherSnakes, int nrOfSnakes, int key);
void reset_snake(Snake *pSnke);
void accelerate(Snake *pSnke);
void turn_left(Snake *pSnke);
void turn_right(Snake *pSnke);
void draw_snake(Snake *pSnke);
void draw_trail(Snake *pSnke);
void destroy_snake(Snake *pSnke);
void update_snakeData(Snake *pSnke, SnakeData *pSnkeData);
void update_recived_snake_data(Snake *pSnke, SnakeData *pSnkeData);
int collideSnake(Snake *pSnake, SDL_Rect rect);
static float distance(int x1, int y1, int x2, int y2); //behöver man?

#endif