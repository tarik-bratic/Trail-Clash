#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../include/snake.h"

#define PI 3.14
#define MAX_TRAIL_LENGTH 100
#define MAX_TRAIL_POINTS 100000000

struct snake {
  float xCord, yCord;
  float xVel, yVel;
  double angle;
  int wind_Width, wind_Height;
  int trailLength;
  int trailCounter;
  int snakeCollided;
  SDL_Renderer *pRenderer;
  SDL_Texture *pTexture;
  SDL_Rect snkeRect;
  SDL_Rect trailPoints[MAX_TRAIL_POINTS];
};


Snake *create_snake(int x, int y, SDL_Renderer *pRenderer, int wind_Width, int wind_Height) {

    Snake *pSnke = malloc(sizeof(struct snake));

    // Set default values
    pSnke->xVel = 0;
    pSnke->yVel = 0;
    pSnke->angle = 0;
    pSnke->trailLength = 0;
    pSnke->trailCounter = 0;
    pSnke->wind_Width = wind_Width;
    pSnke->wind_Height = wind_Height;
    

    // Load image
    SDL_Surface *pSurface = IMG_Load("resources/square.png");
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
    pSnke->snkeRect.w /= 24;
    pSnke->snkeRect.h /= 24;
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

int check_collision(Snake *pSnke) {
  for (int i = 0; i < pSnke->trailLength; i++) {
    SDL_Rect *trailRect = &pSnke->trailPoints[i];
    if (SDL_HasIntersection(&(pSnke->snkeRect), trailRect)) {
      return 1; // Return true if there is a collision
    }
  }
  return 0; // Return false if there is no collision
}

void check_and_handle_collision(Snake *pSnke) {
  if (check_collision(pSnke)) {
    pSnke->snakeCollided = 1;
  }
}

void update_snake(Snake *pSnke) {
  
  // If snake has not collided it runs the code below
  if (!pSnke->snakeCollided) {
    // Tracking previous position of snake's center point to render trail points later
    float prev_xCord = pSnke->xCord + (pSnke->snkeRect.w / 2);
    float prev_yCord = pSnke->yCord + (pSnke->snkeRect.h / 2);

    // Update coordinates
    pSnke->xCord += pSnke->xVel = 0.75 * sin(pSnke->angle * (2 * PI/360));
    pSnke->yCord += pSnke->yVel = -(0.75 * cos(pSnke->angle * (2 * PI/360)));
  
    // Check for collision
    check_and_handle_collision(pSnke);

    // Check if snake goes beyond left or right wall
    if (pSnke->xCord < 0) {
      pSnke->xCord = 0;
    } else if (pSnke->xCord > pSnke->wind_Width - pSnke->snkeRect.w) {
      pSnke->xCord = pSnke->wind_Width - pSnke->snkeRect.w;
    }

    // Check if snake goes beyond top or bottom wall
    if (pSnke->yCord < 0) {
      pSnke->yCord = 0;
    } else if (pSnke->yCord > pSnke->wind_Height - pSnke->snkeRect.h) {
      pSnke->yCord = pSnke->wind_Height - pSnke->snkeRect.h;
    }

    // Set new coordinates
    pSnke->snkeRect.x = pSnke->xCord;
    pSnke->snkeRect.y = pSnke->yCord;
  
    // Add new trail points every frame with a small offset based on the snake's velocity
    if (pSnke->trailLength < MAX_TRAIL_POINTS) {
      float offset = 32; // Changes distance between snake and trail
      pSnke->trailPoints[pSnke->trailLength].x = prev_xCord - pSnke->snkeRect.w / 2 - pSnke->xVel * offset;
      pSnke->trailPoints[pSnke->trailLength].y = prev_yCord - pSnke->snkeRect.h / 2 - pSnke->yVel * offset;
      pSnke->trailPoints[pSnke->trailLength].w = pSnke->snkeRect.w;
      pSnke->trailPoints[pSnke->trailLength].h = pSnke->snkeRect.h;
      pSnke->trailLength++;
    }
  }
}

void draw_snake(Snake *pSnke) {
  SDL_RenderCopyEx(pSnke->pRenderer, pSnke->pTexture, NULL,
    &(pSnke->snkeRect), pSnke->angle, NULL, SDL_FLIP_NONE);
}

void draw_trail(Snake *pSnke) {
  SDL_SetRenderDrawColor(pSnke->pRenderer, 255, 0, 0, 255);

  // Draws the rectangles that is the trail
  for (int i = 0; i < pSnke->trailLength; i++) {
    SDL_RenderFillRect(pSnke->pRenderer, &pSnke->trailPoints[i]);
  }
}

void destroy_snake(Snake *pSnke){
    SDL_DestroyTexture(pSnke->pTexture);
    free(pSnke);
}
