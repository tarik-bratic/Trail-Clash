#ifndef data_h
#define data_h

/* CONST VERIABLES */
#define PI 3.14
#define MAX_ITEMS 1
#define MAX_SNKES 1
#define POINT_SIZE 2
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 560
#define ONE_MS 1000/60-15
#define MAX_TRAIL_LENGTH 100
#define INPUT_BUFFER_SIZE 128
#define MAX_TRAIL_POINTS 100000000

#define DATA_SIZE 512
#define UDP_SERVER_PORT 2000
#define UDP_CLIENT_PORT 0

/* Game State struct */
enum gameState { START, RUNNING };
typedef enum gameState GameState;

/* Game Scene struct */
enum gameScene { MENU_SCENE, BUILD_SCENE, LOBBY_SCENE, GAME_SCENE };
typedef enum gameScene GameScene;

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
    int color;

};
typedef struct snakeData SnakeData; 

/* Server Data struct */
struct serverData {

    int snkeNum;
    SnakeData snakes[MAX_SNKES];
    GameState gState;
    int maxClients;
    int presentClients;

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

struct itemData {

    int xcoords, ycoords;

};
typedef struct itemData ItemData;

#endif