#ifndef data_h
#define data_h

/* CONST VERIABLES */
#define PI 3.14
#define MAX_SNKES 2
#define POINT_SIZE 2
#define DATA_SIZE 512
#define UDP_SERVER_PORT 2000
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 560
#define ONE_MS 1000/60-15
#define MAX_TRAIL_LENGTH 100
#define INPUT_BUFFER_SIZE 128
#define MAX_TRAIL_POINTS 100000000
#define MAX_ITEMS 1

/* Game State struct */
enum gameState { START, RUNNING };
typedef enum gameState GameState;

/* Client Command struct */
enum clientCommand { READY, LEFT, RIGHT, DISC };
typedef enum clientCommand ClientCommand;

/* Client Data struct */
struct clientData {
    ClientCommand command;
    int snkeNumber;
    char playerName[INPUT_BUFFER_SIZE];
};
typedef struct clientData ClientData;

/* Snake Data struct */
struct snakeData {

    float xCord, yCord;
    float xVel, yVel;
    int angle, alive;
    int trailLength;
    int trailCounter;
    int snakeCollided;

};
typedef struct snakeData SnakeData; 

/* Server Data struct */
struct serverData {

    int snkeNum;
    SnakeData snakes[MAX_SNKES];
    GameState gState;
    int maxConnPlayers;
    int connPlayers;

};
typedef struct serverData ServerData;

/* Snake stuct  */
struct snake {

  float xCord, yCord;
  float xVel, yVel;
  float xSrt, ySrt;
  double angle, alive;

  int wind_Width, wind_Height;

  int trailLength;
  int trailCounter;
  int snakeCollided;
  SDL_Rect trailPoints[MAX_TRAIL_POINTS];

  SDL_Renderer *pRenderer;
  SDL_Texture *pTexture;
  SDL_Rect snkeRect;

};
typedef struct snake Snake;

#endif