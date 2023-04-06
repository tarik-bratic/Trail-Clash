#include <stdio.h>
#include <SDL2/SDL.h>

#define WINDOW_HEIGHT 1200
#define WINDOW_WIDTH 800

typedef struct game {
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
} Game;

int init(Game *pGameStrct);
void close(Game *pGameStrct);
void run(Game *pGameStrct);
void handleInput(Game *pGameStrct, SDL_Event *pEvent);

int main(int argv, char** args) {
  Game g;

  init(&g);
  run(&g);
  close(&g);
  

  return 0;

}

int init(Game *pGameStrct) {
  pGameStrct->pWindow = SDL_CreateWindow("Snake.io", SDL_WINDOWPOS_CENTERED, 
    SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
}

void run(Game *pGameStrct) {
  SDL_Event event;
  int closeRequest = 0;

  while(!closeRequest) {
    while(SDL_PollEvent(&event)) {
      if (event.type == SDL_KEYDOWN) closeRequest = 1;
      else handleInput(pGameStrct, &event);
    }
  }
}

void handleInput(Game *pGameStrct, SDL_Event *pEvent) {
  if (pEvent->type == SDL_KEYDOWN) {
    switch(pEvent->key.keysym.scancode) {
      case SDL_SCANCODE_Q:
        break;
    }
  }
}

void close(Game *pGameStrct) {
  if (pGameStrct->pWindow) SDL_DestroyWindow(pGameStrct->pWindow);
  SDL_Quit();
}