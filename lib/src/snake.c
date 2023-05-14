#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../include/data.h"
#include "../include/snake.h"

void check_outOfBounds(Snake *pSnke);
int check_collision_with_self(Snake *pSnke);
static float distance(int x1, int y1, int x2, int y2);
void check_and_handle_collision(Snake *pSnke, Snake **otherSnakes, int nrOfSnakes);
int check_collision_with_other_snakes(Snake *pSnke, Snake **otherSnakes, int nrOfSnakes);

char *snakeColors[] = {
  
  "../lib/resources/redSquare.png",
  "../lib/resources/blueSquare.png",
  "../lib/resources/greenSquare.png",
  "../lib/resources/yellowSquare.png",

};

SDL_Color trailColors[] = {{255, 0, 0, 255},{0, 0, 255, 255},{0, 255, 0, 255},{255, 255, 0, 255}};

/* Create a snake with attributes */
Snake *create_snake(SDL_Renderer *pRenderer, int number, int color) {

  Snake *pSnke = malloc(sizeof(struct snake));

  // Set default values
  pSnke->angle = 0;
  pSnke->color = color;
  pSnke->trailLength = 0;
  pSnke->trailCounter = 0;
  pSnke->gapDuration = 100;
  pSnke->gapTrailCounter = 0;
  pSnke->spawnTrailPoints = 1;
  pSnke->xVel = pSnke->yVel = 0;
  pSnke->wind_Width = WINDOW_WIDTH;
  pSnke->wind_Height = WINDOW_HEIGHT;

  // Load desired image
  SDL_Surface *pSurface = IMG_Load(snakeColors[color]);
  if (!pSurface) {
      printf("Error: %s\n", SDL_GetError());
      return NULL;
  }

  // Create texture
  pSnke->pRenderer = pRenderer;
  pSnke->pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
  SDL_FreeSurface(pSurface);
  if (!pSnke->pTexture) {
    printf("Error: %s\n", SDL_GetError());
    return NULL;
  }

  // Set attributes to texture
  SDL_QueryTexture(pSnke->pTexture, NULL, NULL, &(pSnke->snkeRect.w), &(pSnke->snkeRect.h));

  // Set width and height of object
  pSnke->snkeRect.w /= 48;
  pSnke->snkeRect.h /= 48;

  return pSnke;

}

/* Command to turn left */
void turn_left(Snake *pSnke) {
    pSnke->angle -= 5.0;
}

/* Command to turn right */
void turn_right(Snake *pSnke) {
    pSnke->angle += 5.0;
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

    if(key == 1) {
      pSnke->xCord += pSnke->xVel * 3;
      pSnke->yCord += pSnke->yVel * 3;
    }
    
    // Check for collision
    check_and_handle_collision(pSnke, otherSnakes, nrOfSnakes);

    // Check if player is in field
    check_outOfBounds(pSnke);

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
      pSnke->gapDuration = 100;
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

/* Function that creates collisions around the field, that the player dosen't escape */
void check_outOfBounds(Snake *pSnke) {

  // Check if snake goes beyond left or right wall
    if (pSnke->xCord < WINDOW_WIDTH - 695) {
      pSnke->xCord = WINDOW_WIDTH - 695;
      pSnke->snakeCollided = 1;
    } else if (pSnke->xCord > (pSnke->wind_Width - pSnke->snkeRect.w) - 5) {
      pSnke->xCord = (pSnke->wind_Width - pSnke->snkeRect.w) - 5;
      pSnke->snakeCollided = 1;
    }

    // Check if snake goes beyond top or bottom wall
    if (pSnke->yCord < 5) {
      pSnke->yCord = 5;
      pSnke->snakeCollided = 1;
    } else if (pSnke->yCord > (pSnke->wind_Height - pSnke->snkeRect.h) - 5) {
      pSnke->yCord = (pSnke->wind_Height - pSnke->snkeRect.h) - 5;
      pSnke->snakeCollided = 1;
    }

}

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

/* Snake has collided */
int collideSnake(Snake *pSnake, SDL_Rect rect){
    return distance(pSnake->snkeRect.x + pSnake->snkeRect.w / 2, pSnake->snkeRect.y + pSnake->snkeRect.h / 2, 
      rect.x + rect.w / 2, rect.y + rect.h / 2) < (pSnake->snkeRect.w + rect.w) / 2;
}

/* The distance */
static float distance(int x1, int y1, int x2, int y2) {
    return sqrt((x2-x1) * (x2-x1) + (y2-y1) * (y2-y1));
}

/* Reset the snake to default values */
void reset_snake(Snake *pSnke) {

  pSnke->alive = 1;
  pSnke->trailLength = 0;
  pSnke->trailCounter = 0;
  pSnke->snakeCollided = 0;
  pSnke->xVel=pSnke->yVel=0;

  for (int i = 0; i < MAX_SNKES; i++) {
    if (i == 0) {
      pSnke->xSrt = pSnke->xCord = pSnke->snkeRect.x = WINDOW_WIDTH / 4 + 50;
      pSnke->ySrt = pSnke->yCord = pSnke->snkeRect.y = WINDOW_HEIGHT / 4 - 80;
      pSnke->angle = 130;
    }

    if (i == 1) {
      pSnke->xSrt = pSnke->xCord = pSnke->snkeRect.x = (WINDOW_WIDTH / 4 + 45) * 3;
      pSnke->ySrt = pSnke->yCord = pSnke->snkeRect.y = WINDOW_HEIGHT / 4 - 80;
      pSnke->angle = -130;
    }

    if (i == 2) {
      pSnke->xSrt = pSnke->xCord = pSnke->snkeRect.x = WINDOW_WIDTH / 4 + 50;
      pSnke->ySrt = pSnke->yCord = pSnke->snkeRect.y = WINDOW_HEIGHT - 80;
      pSnke->angle = 50;
    }

    if (i == 3) {
      pSnke->xSrt = pSnke->xCord = pSnke->snkeRect.x = (WINDOW_WIDTH / 4 + 45) * 3;
      pSnke->ySrt = pSnke->yCord = pSnke->snkeRect.y = WINDOW_HEIGHT - 80;
      pSnke->angle = -50;
    }
  }

  for(int i = 0; i < MAX_TRAIL_POINTS; i++) {
    pSnke->trailPoints[i].x = 0;
    pSnke->trailPoints[i].y = 0;
    pSnke->trailPoints[i].w = 0;
    pSnke->trailPoints[i].h = 0;
  }

}

/* Render a copy of a snake */
void draw_snake(Snake *pSnke) {
  SDL_RenderCopyEx(pSnke->pRenderer, pSnke->pTexture, NULL, &(pSnke->snkeRect), pSnke->angle, NULL, SDL_FLIP_NONE);
}

/* Render the trail behind the player */
void draw_trail(Snake *pSnke) {
  SDL_SetRenderDrawColor(pSnke->pRenderer, trailColors[pSnke->color].r, trailColors[pSnke->color].g, 
    trailColors[pSnke->color].b, trailColors[pSnke->color].a);

  for (int i = 0; i < pSnke->trailLength; i++) {
    SDL_RenderFillRect(pSnke->pRenderer, &pSnke->trailPoints[i]);
  }
}

/* Destory snake texture and free memory */
void destroy_snake(Snake *pSnke) {
    SDL_DestroyTexture(pSnke->pTexture);
    free(pSnke);
}

/* Update data Snake -> SnakeData */
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

/* Update data SnakeData -> Snake */
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