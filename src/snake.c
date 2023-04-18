#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../include/snake.h"

#define PI 3.14

struct snake {
  float xCord, yCord;
  float xVel, yVel;
  double angle;
  int wind_Width, wind_Height;
  SDL_Renderer *pRenderer;
  SDL_Texture *pTexture;
  SDL_Rect snkeRect;
};

Snake *create_snake(int x, int y, SDL_Renderer *pRenderer, int wind_Width, int wind_Height) {

    Snake *pSnke = malloc(sizeof(struct snake));

    // Set default values
    pSnke->xVel = 0;
    pSnke->yVel = 0;
    pSnke->angle = 0;
    pSnke->wind_Width = wind_Width;
    pSnke->wind_Height = wind_Height;

    // Load image
    SDL_Surface *pSurface = IMG_Load("resources/snake.png");
    if (!pSurface) {
        printf("Error: %s\n", SDL_GetError());
        return NULL;
    }

    // Create texture and check for error
    pSnke->pRenderer = pRenderer;
    pSnke->pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
    SDL_FreeSurface(pSurface);

    if (!pSnke->pTexture) {
        printf("Error: %s\n", SDL_GetError());
        return NULL;
    }

    // Set attributes to texture
    SDL_QueryTexture(pSnke->pTexture, NULL, NULL, 
      &(pSnke->snkeRect.w), &(pSnke->snkeRect.h));

    // Set start position on the center
    pSnke->snkeRect.w /= 8;
    pSnke->snkeRect.h /= 8;
    pSnke->xCord = x - pSnke->snkeRect.w / 2;
    pSnke->yCord = y - pSnke->snkeRect.h / 2;

    return pSnke;

}

/*void accelerate(Snake *pSnke) {

    Kan användas senare. Låt stå.

    pSnke->xVel = 0.75*sin(pSnke->angle*2*PI/360);
    pSnke->yVel = -(0.75*cos(pSnke->angle*2*PI/360));
}*/

void turn_left(Snake *pSnke) {
    pSnke->angle -= 5.0;
}

void turn_right(Snake *pSnke) {
    pSnke->angle += 5.0;
}

void update_snake(Snake *pSnke) {

    // Update coardinates
    pSnke->xCord += pSnke->xVel = 0.75 * sin(pSnke->angle * (2 * PI/360));
    pSnke->yCord += pSnke->yVel = -(0.75 * cos(pSnke->angle * (2 * PI/360)));

    if (pSnke->xCord < 0) {
        pSnke->xCord = 0;
    } else if (pSnke->xCord > pSnke->wind_Width - pSnke->snkeRect.w) {
        pSnke->xCord = pSnke->wind_Width - pSnke->snkeRect.w;
    }

    if (pSnke->yCord < 0) {
        pSnke->yCord = 0;
    } else if (pSnke->yCord > pSnke->wind_Height - pSnke->snkeRect.h) {
        pSnke->yCord = pSnke->wind_Height - pSnke->snkeRect.h;
    }

    // Set new cordinates
    pSnke->snkeRect.x = pSnke->xCord;
    pSnke->snkeRect.y = pSnke->yCord;

}

void draw_snake(Snake *pSnke) {
    SDL_RenderCopyEx(pSnke->pRenderer, pSnke->pTexture, NULL,
        &(pSnke->snkeRect), pSnke->angle, NULL, SDL_FLIP_NONE);
}

void destroy_snake(Snake *pSnke){
    SDL_DestroyTexture(pSnke->pTexture);
    free(pSnke);
}