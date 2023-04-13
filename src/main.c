#include <stdio.h>
#include <SDL2/SDL.h>
#include "../include/snake.h"

#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 800
#define one_ms 1000/60-15

typedef struct game {
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnk;
} Game;

int initStructure(Game *pGame);
int displayError(Game *pGame);
void closeGame(Game *pGame);
void run(Game *pGame);
void handleInput(Game *pGame, SDL_Event *pEvent);
void createCanvas(Game *pGame);

int main(int argv, char** args) {
  
  Game g;

  initStructure(&g);
  run(&g);
  close(&g);

  return 0;

}

int initStructure(Game *pGame) {
  // Checks if SDL Library works
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0){
      printf("Error: %s\n", SDL_GetError());
      return 0;
  }
  // Create window and check for error
  pGame->pWindow = SDL_CreateWindow("trailClash", SDL_WINDOWPOS_CENTERED, 
    SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  if (!pGame->pWindow) displayError(pGame);

  // Create renderer and check for error
  pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, 
    SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  if (!pGame->pRenderer) displayError(pGame);

  // Creates player and checks for error
  pGame->pSnk = createSnake(WINDOW_WIDTH/2, WINDOW_HEIGHT/2, 
    pGame->pRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
  if (!pGame->pSnk) displayError(pGame);

}

void run(Game *pGame) {
  SDL_Event event;
  int closeRequest = 0;

  while(!closeRequest) {

    while(SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) closeRequest = 1;
      else handleInput(pGame, &event);
    }

    updateRocket(pGame->pSnk);
    createCanvas(pGame);

    SDL_Delay(one_ms);

  }
}

void handleInput(Game *pGame, SDL_Event *pEvent) {
  if(pEvent->type == SDL_KEYDOWN){
        switch(pEvent->key.keysym.scancode){
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_LEFT:
                turnLeft(pGame->pSnk);
            break;
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                turnRight(pGame->pSnk);
            break;
        }
    }
}

void close(Game *pGame) {
  if (pGame->pSnk) destroyRocket(pGame->pSnk);
  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);
  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);
  SDL_Quit();
}

void createCanvas(Game *pGame) {
  SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);
  drawRocket(pGame->pSnk);
  SDL_RenderPresent(pGame->pRenderer);
}

int displayError(Game *pGame) {
  printf("Error: %s\n", SDL_GetError());
  close(pGame);
  return 0;  
}