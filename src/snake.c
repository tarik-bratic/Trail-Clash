#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../include/snake.h"

#define PI 3.14

struct snake {
  float xCord, yCord;   // Cordinates
  float xVel, yVel;     // Velocity
  double angle;
  int wind_Width, wind_Height;
  SDL_Renderer *pRenderer;
  SDL_Texture *pTexture;
  SDL_Rect snkRect;
};

Snake *createSnake(int x, int y, SDL_Renderer *pRenderer, int wind_Width, int wind_Height) {
    Snake *pSnke = malloc(sizeof(struct snake));
    pSnke->xVel = 0;
    pSnke->yVel = 0;
    pSnke->angle = 0;
    pSnke->wind_Width = wind_Width;
    pSnke->wind_Height = wind_Height;

    // Load the image
    SDL_Surface *pSurface = IMG_Load("resources/snake.png");
    if (!pSurface) {
        printf("Error: %s\n", SDL_GetError());
        return NULL;
    }

    pSnke->pRenderer = pRenderer;
    pSnke->pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
    SDL_FreeSurface(pSurface);

    if (!pSnke->pTexture) {
        printf("Error: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_QueryTexture(pSnke->pTexture, NULL, NULL, 
      &(pSnke->snkRect.w),&(pSnke->snkRect.h));

    pSnke->snkRect.w /= 4;
    pSnke->snkRect.h /= 4;
    pSnke->xCord = x - pSnke->snkRect.w / 2;
    pSnke->yCord = y - pSnke->snkRect.h / 2;

    return pSnke;

}

void accelerate(Snake *pSnke) {
    pSnke->xVel = 0.75*sin(pSnke->angle*2*PI/360);
    pSnke->yVel = -(0.75*cos(pSnke->angle*2*PI/360));
}

void turnLeft(Snake *pSnke) {
    pSnke->angle-=5.0;
}

void turnRight(Snake *pSnke) {
    pSnke->angle+=5.0;
}

void updateRocket(Snake *pSnke) {
    pSnke->xCord += pSnke->xVel = 0.75*sin(pSnke->angle*(2*PI/360));
    pSnke->yCord += pSnke->yVel = -(0.75*cos(pSnke->angle*(2*PI/360)));

    if (pSnke->xCord < 0) {
        pSnke->xCord += pSnke-> wind_Width;
    } else if (pSnke->xCord > pSnke->wind_Width) {
        pSnke->xCord -= pSnke->wind_Width;
    }

    if (pSnke->yCord < 0) {
        pSnke->yCord += pSnke->wind_Height;
    } else if (pSnke->yCord > pSnke->wind_Height) {
        pSnke->yCord -= pSnke->wind_Height;
    }

    pSnke->snkRect.x = pSnke->xCord;
    pSnke->snkRect.y = pSnke->yCord;
}

void drawRocket(Snake *pSnke) {
    SDL_RenderCopyEx(pSnke->pRenderer, pSnke->pTexture, NULL,
        &(pSnke->snkRect), pSnke->angle, NULL, SDL_FLIP_NONE);
}

void destroyRocket(Snake *pSnke){
    SDL_DestroyTexture(pSnke->pTexture);
    free(pSnke);
}