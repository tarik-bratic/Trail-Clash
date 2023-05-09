#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../include/data.h"
#include "../include/snake.h"

char *snakeColors[] = {
  
  "../lib/resources/redSquare.png",
  "../lib/resources/blueSquare.png",
  "../lib/resources/greenSquare.png",
  "../lib/resources/yellowSquare.png",

};

SDL_Color trailColors[] = {{255, 0, 0, 255},{0, 0, 255, 255},{0, 255, 0, 255},{255, 255, 0, 255}};

/* Function to create a snake with attributes with default values */
Snake *create_snake(int number, SDL_Renderer *pRenderer, int wind_Width, int wind_Height, int color) {

  Snake *pSnke = malloc(sizeof(struct snake));

  // Set default values
  pSnke->angle = 0;
  pSnke->trailLength = 0;
  pSnke->trailCounter = 0;
  pSnke->gapTrailCounter = 0;
  pSnke->gapDuration = 200;
  pSnke->spawnTrailPoints = 1;
  pSnke->xVel = pSnke->yVel = 0;
  pSnke->wind_Width = wind_Width;
  pSnke->wind_Height = wind_Height;
  pSnke->color = color;

  // Load desired image
  SDL_Surface *pSurface = IMG_Load(snakeColors[color]);
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

  // Set width and height of object
  pSnke->snkeRect.w /= 48;
  pSnke->snkeRect.h /= 48;

  // Set start position on the center
  pSnke->xSrt = pSnke->xCord = pSnke->snkeRect.x = 
      wind_Width * (number + 1) / 6 - pSnke->snkeRect.w / 2;
  pSnke->ySrt = pSnke->yCord = pSnke->snkeRect.y = 
      wind_Height / 2 - pSnke->snkeRect.h / 2;

  return pSnke;

}

void accelerate(Snake *pSnke) {

    pSnke->xCord += pSnke->xVel = 0.5 * sin(pSnke->angle * (2 * PI/360));
    pSnke->yCord += pSnke->yVel = -(0.5 * cos(pSnke->angle * (2 * PI/360)));
}

/* Command to turn left */
void turn_left(Snake *pSnke) {
    pSnke->angle -= 5.0;
}

/* Command to turn right */
void turn_right(Snake *pSnke) {
    pSnke->angle += 5.0;
}

/* PRATA OM DESSA TVÅ FUNKTIONER */

/* If a collision has occured return true, else false */
int check_collision_with_self(Snake *pSnke) {

  for (int i = 0; i < pSnke->trailLength; i++) {
    SDL_Rect *trailRect = &pSnke->trailPoints[i];
    if (SDL_HasIntersection(&(pSnke->snkeRect), trailRect)) return 1;
  }

  return 0;

}

/* If a collision with other snake trails has occured return true, else false */
int check_collision_with_other_snakes(Snake *pSnke, Snake **otherSnakes, int nrOfSnakes) {
  for (int s = 0; s < nrOfSnakes; s++) {
    Snake *otherSnake = otherSnakes[s];
    for (int i = 0; i < otherSnake->trailLength; i++) {
      SDL_Rect *trailRect = &otherSnake->trailPoints[i];
      if (SDL_HasIntersection(&(pSnke->snkeRect), trailRect)) return 1;
    }
  }
  return 0;
}

/* The snake has collided */
void check_and_handle_collision(Snake *pSnke, Snake **otherSnakes, int nrOfSnakes) {
  if (check_collision_with_self(pSnke) || check_collision_with_other_snakes(pSnke, otherSnakes, nrOfSnakes)) {
    pSnke->snakeCollided = 1;
  }
}

/* Update and set new cords and look if player is not outside of the screen */
void update_snake(Snake *pSnke, Snake **otherSnakes, int nrOfSnakes, int key) {

  // Changes distance between snake and trail
  float trail_offset = 8;

  // Collision has not happened, run code below
  if (!pSnke->snakeCollided) {

    // Tracking previous position of snake's center point to render trail points later
    float prev_xCord = pSnke->xCord + (pSnke->snkeRect.w / 2);
    float prev_yCord = pSnke->yCord + (pSnke->snkeRect.h / 2);

    // Update coardinates
    pSnke->xCord += pSnke->xVel = 1.5 * sin(pSnke->angle * (2 * PI/360));
    pSnke->yCord += pSnke->yVel = -(1.5 * cos(pSnke->angle * (2 * PI/360)));
    
    if(key==1){
    pSnke->xCord += pSnke->xVel*3;
    pSnke->yCord += pSnke->yVel*3;
    }
    // Check for collision
    check_and_handle_collision(pSnke, otherSnakes, nrOfSnakes);

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

    // Set new cordinates
    pSnke->snkeRect.x = pSnke->xCord;
    pSnke->snkeRect.y = pSnke->yCord;
    
    pSnke->gapTrailCounter++;
    // Check if it's time to create a new gap
    if (pSnke->spawnTrailPoints && pSnke->gapTrailCounter >= pSnke->gapDuration) {
      pSnke->spawnTrailPoints = 0;
      pSnke->gapTrailCounter = 0;
      pSnke->gapDuration = 30;
    } else if (!pSnke->spawnTrailPoints && pSnke->gapTrailCounter >= pSnke->gapDuration) {
      pSnke->spawnTrailPoints = 1;
      pSnke->gapTrailCounter = 0;
      pSnke->gapDuration = 200;
    }
              
    // Add new trail points if spawnTrailPoints is true
    if (pSnke->spawnTrailPoints && pSnke->trailLength < MAX_TRAIL_POINTS) {
      pSnke->trailPoints[pSnke->trailLength].x = prev_xCord - pSnke->snkeRect.w / 2 - pSnke->xVel * trail_offset;
      pSnke->trailPoints[pSnke->trailLength].y = prev_yCord - pSnke->snkeRect.h / 2 - pSnke->yVel * trail_offset;
      pSnke->trailPoints[pSnke->trailLength].w = pSnke->snkeRect.w;
      pSnke->trailPoints[pSnke->trailLength].h = pSnke->snkeRect.h;
      pSnke->trailLength++;
    }

  }

}

/* Reset the snake to default values */
void reset_snake(Snake *pSnke) {
  pSnke->snkeRect.x=pSnke->xCord=pSnke->xSrt;
  pSnke->snkeRect.y=pSnke->yCord=pSnke->ySrt;
  pSnke->angle=0;
  pSnke->xVel=pSnke->yVel=0;
  pSnke->alive = 1;
}

/* Render a copy of a snake */
void draw_snake(Snake *pSnke) {
  SDL_RenderCopyEx(pSnke->pRenderer, pSnke->pTexture, NULL,
    &(pSnke->snkeRect), pSnke->angle, NULL, SDL_FLIP_NONE);
}

/* Render the trail behind the player */
void draw_trail(Snake *pSnke) {
  SDL_SetRenderDrawColor(pSnke->pRenderer, trailColors[pSnke->color].r, trailColors[pSnke->color].g, trailColors[pSnke->color].b, trailColors[pSnke->color].a);

  for (int i = 0; i < pSnke->trailLength; i++) {
    SDL_RenderFillRect(pSnke->pRenderer, &pSnke->trailPoints[i]);
  }
}

/* Destory snake texture and free */
void destroy_snake(Snake *pSnke) {
    SDL_DestroyTexture(pSnke->pTexture);
    free(pSnke);
}

/* Update snake data from snake (angle, cord, vel) */
void update_snakeData(Snake *pSnke, SnakeData *pSnkeData) {
    pSnkeData->alive = pSnke->alive;
    pSnkeData->angle = pSnke->angle;
    pSnkeData->xVel = pSnke->xVel;
    pSnkeData->yVel = pSnke->yVel;
    pSnkeData->xCord = pSnke->xCord;
    pSnkeData->yCord = pSnke->yCord;
    pSnkeData->snakeCollided = pSnke->snakeCollided;
    pSnkeData->trailCounter = pSnke->trailCounter;
    pSnkeData->trailLength = pSnke->trailLength;
}

/* Update snake data to snake (angle, cord, vel) */
void update_recived_snake_data(Snake *pSnke, SnakeData *pSnkeData) {
    pSnke->alive = pSnkeData->alive;
    pSnke->angle = pSnkeData->angle;
    pSnke->xVel = pSnkeData->xVel;
    pSnke->yVel = pSnkeData->yVel;
    pSnke->xCord = pSnkeData->xCord;
    pSnke->yCord = pSnkeData->yCord;
    pSnke->snakeCollided = pSnkeData->snakeCollided;
    pSnke->trailCounter = pSnkeData->trailCounter;
    pSnke->trailLength = pSnkeData->trailLength;
}

int collideSnake(Snake *pSnake, SDL_Rect rect){
    //return SDL_HasIntersection(&(pSnake->snkeRect),&rect);
    return distance(pSnake->snkeRect.x+pSnake->snkeRect.w/2,pSnake->snkeRect.y+pSnake->snkeRect.h/2,rect.x+rect.w/2,rect.y+rect.h/2)<(pSnake->snkeRect.w+rect.w)/2;
}

static float distance(int x1, int y1, int x2, int y2){
    return sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}