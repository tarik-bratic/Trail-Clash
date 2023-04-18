#include <stdio.h>
#include <SDL2/SDL.h>
#include "../include/snake.h"

#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 800
#define ONE_MS 1000/60-15

typedef struct game {
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke;
} Game;

int init_structure(Game *pGame);
int error_forceExit(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);
void input_handler(Game *pGame, SDL_Event *pEvent);
void render_snake(Game *pGame);

int main(int argv, char** args) {
  
  Game g;

  init_structure(&g);
  run(&g);
  close(&g);

  return 0;

}

int init_structure(Game *pGame) {

  // Checks if SDL Library works
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0){
      printf("Error: %s\n", SDL_GetError());
      return 0;
  }

  // Create window and check for error
  pGame->pWindow = SDL_CreateWindow("Trail Clash", SDL_WINDOWPOS_CENTERED, 
    SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  if (!pGame->pWindow) error_forceExit(pGame);

  // Create renderer and check for error
  pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, 
    SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  if (!pGame->pRenderer) error_forceExit(pGame);

  // Creates player and checks for error
  pGame->pSnke = create_snake(WINDOW_WIDTH/2, WINDOW_HEIGHT/2, 
    pGame->pRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);
  if (!pGame->pSnke) error_forceExit(pGame);

}

void run(Game *pGame) {

  SDL_Event event;
  int closeRequest = 0;

  while(!closeRequest) {

    while(SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) closeRequest = 1;
      else input_handler(pGame, &event);
    }

    update_snake(pGame->pSnke);
    render_snake(pGame);

    SDL_Delay(ONE_MS);

  }

}

void input_handler(Game *pGame, SDL_Event *pEvent) {

  // If input is a keypress
  if (pEvent->type == SDL_KEYDOWN) {
    switch(pEvent->key.keysym.scancode){
      case SDL_SCANCODE_A:
      case SDL_SCANCODE_LEFT:
        turn_left(pGame->pSnke);
        break;
      case SDL_SCANCODE_D:
      case SDL_SCANCODE_RIGHT:
        turn_right(pGame->pSnke);
      break;
    }
  }

}

void close(Game *pGame) {
  if (pGame->pSnke) destroy_snake(pGame->pSnke);
  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);
  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);
  SDL_Quit();
}

void render_snake(Game *pGame) {
  SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);
  draw_snake(pGame->pSnke);
  SDL_RenderPresent(pGame->pRenderer);
}

int error_forceExit(Game *pGame) {
  printf("Error: %s\n", SDL_GetError());
  close(pGame);
  return 0;  
}