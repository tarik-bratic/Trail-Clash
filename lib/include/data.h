#ifndef data_h
#define data_h

#define MAX_SNKES 1
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

struct bulletData {

    float xCord, yCord;
    float xVel, yVel;
    int time;

};
typedef struct bulletData BulletData;  

// Represent the snakes data
struct snakeData {

    float xCord, yCord;
    float xVel, yVel;
    int angle, alive;
    BulletData bData;

};
typedef struct snakeData SnakeData; 

struct serverData {

    int snkeNum;
    SnakeData snakes[MAX_SNKES];
    GameState gState;

};
typedef struct serverData ServerData;

#endif