#ifndef data_h
#define data_h

#define MAX_SNKES 4
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 560
#define ONE_MS 1000/60-15

enum gameState { START, RUNNING };
typedef enum gameState GameState;

enum clientCommand { READY, LEFT, RIGHT };
typedef enum clientCommand ClientCommand;

struct clientData {
    ClientCommand command;
    int snkeNumber;
};
typedef struct clientData ClientData;

// Represent the snakes data
struct snakeData {

    float xCord, yCord;
    float xVel, yVel;
    int angle, alive;

};
typedef struct snakeData SnakeData; 

struct serverData {

    int snkeNum;
    SnakeData snakes[MAX_SNKES];
    GameState gState;

};
typedef struct serverData ServerData;

/* Snake stuct (cords, vel, angle, render, texture, rect, bullet) */
struct snake {

  float xCord, yCord;
  float xVel, yVel;
  float xSrt, ySrt;
  double angle, alive;

  int wind_Width, wind_Height;

  SDL_Renderer *pRenderer;
  SDL_Texture *pTexture;
  SDL_Rect snkeRect;

};
typedef struct snake Snake;

#endif