#ifndef data_h
#define data_h

#define PI 3.14
#define MAX_SNKES 4
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 560
#define ONE_MS 1000/60-15
#define MAX_TRAIL_LENGTH 100
#define INPUT_BUFFER_SIZE 128
#define MAX_TRAIL_POINTS 100000000

enum gameState { START, RUNNING };
typedef enum gameState GameState;

enum clientCommand { READY, LEFT, RIGHT, DISC };
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
    int trailLength;
    int trailCounter;
    int snakeCollided;
    int color;

};
typedef struct snakeData SnakeData; 

struct serverData {

    int snkeNum;
    SnakeData snakes[MAX_SNKES];
    GameState gState;
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
  int color;
  int gapTrailCounter;     
  int gapDuration;
 
  int spawnTrailPoints;
  SDL_Rect trailPoints[MAX_TRAIL_POINTS];

  SDL_Renderer *pRenderer;
  SDL_Texture *pTexture;
  SDL_Rect snkeRect;

};
typedef struct snake Snake;

#endif