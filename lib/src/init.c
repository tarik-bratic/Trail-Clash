#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include "../include/data.h"
#include "../include/init.h"

/*
*  Initialize necesery SDL libraries.
*  SDL, TTF and SDLNet are initiated after function call.
*/
int init_sdl_libraries() {

  // Initialaze SDL library
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
    printf("Error (SDL_Init): %s\n", SDL_GetError());
    return 0;
  }

  // Initialaze TTF library
  if (TTF_Init() != 0) {
    printf("Error (TTF_Init): %s\n", TTF_GetError());
    SDL_Quit();
    return 0;
  }

  // Initialaze SDL_net library
  if (SDLNet_Init()) {
    fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
    TTF_Quit();
    SDL_Quit();
    return 0;
  }

  return 1;

}

/** 
*  Create the main window for the game.
*  \param title The title of the window.
*  \param pWindow Identifier of SDL_Window
*  The function also check for error.
*  \returns Returns the window that was created or NULL on failure; SDL_GetError() for more information.
*/
SDL_Window *main_wind(const char *title, SDL_Window *pWindow) {

  pWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, 
    SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

  if (!pWindow) {
    printf("Error (Window): %s\n", SDL_GetError());
    return 0;  
  }

  return pWindow;

}

/** 
*  Create a 2D rendering context for a window.
*  \param pRenderer Identifier of SDL_Renderer.
*  \param pWindow The window where rendering is displayed.
*  The function also check for error.
*  \returns Returns a valid rendering context or NULL if there was an error; SDL_GetError() for more information.
*/
SDL_Renderer *create_render(SDL_Renderer *pRenderer, SDL_Window *pWindow) {

  pRenderer = SDL_CreateRenderer(pWindow, -1, 
    SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

  if (!pRenderer) {
    printf("Error (Renderer): %s\n", SDL_GetError());
    return 0;  
  }

  return pRenderer;

}