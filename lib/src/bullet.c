#include <SDL2/SDL.h>
#include "../include/data.h"
#include "../include/bullet.h"

/* Bullet struct (cords, vel, window, time) */
struct bullet {

    float xCord, yCord;
    float xVel, yVel;
    int window_width, window_height;
    int time;

};

/* Function to create a bullet */
Bullet *create_bullet(int window_width, int window_height) {

    Bullet *pBullet = malloc(sizeof(struct bullet));

    pBullet->window_width = window_width;
    pBullet->window_height = window_height;
    pBullet->time = 0;

    return pBullet;

}

/* Update bullet with new data */
void update_bullet(Bullet *pBullet) {

    if (pBullet->time == 0) return;

    pBullet->xCord += pBullet->xVel;
    pBullet->yCord += pBullet->yVel;

    if (pBullet->xCord < 0) {
        pBullet->xCord += pBullet->window_width;
    } else if (pBullet->xCord > pBullet->window_width) {
        pBullet->xCord -= pBullet->window_width;
    }

    if (pBullet->yCord < 0) {
        pBullet->yCord += pBullet->window_height;
    } else if (pBullet->yCord > pBullet->window_height) {
        pBullet->yCord -= pBullet->window_height;
    }

    (pBullet->time)--;

    return;
}

void start_bullet(Bullet *pBullet, float xCord, float yCord, float xVel, float yVel) {

    pBullet->xCord = xCord;
    pBullet->yCord = yCord;
    pBullet->xVel = xVel;
    pBullet->yVel = yVel;
    pBullet->time = BULLETLIFETIME;

}

void kill_bullet(Bullet *pBullet) {
    pBullet->time=0;
}

/* Render bullet with point */
void draw_bullet(Bullet *pBullet, SDL_Renderer *pRenderer) {

    if(pBullet->time==0) return;

    SDL_RenderDrawPoint(pRenderer,pBullet->xCord,pBullet->yCord);
    SDL_RenderDrawPoint(pRenderer,pBullet->xCord+1, pBullet->yCord);
    SDL_RenderDrawPoint(pRenderer,pBullet->xCord, pBullet->yCord+1);
    SDL_RenderDrawPoint(pRenderer,pBullet->xCord+1, pBullet->yCord+1);

}

float xBullet(Bullet *pBullet) {
    return pBullet->xCord;
}

float yBullet(Bullet *pBullet) {
    return pBullet->yCord;
}

void destroy_bullet(Bullet *pBullet) {
    free(pBullet);
}

int alive_bullet(Bullet *pBullet) {
    return pBullet->time > 0;
}

/* Update bullet data from bullet */
void send_bullet_data(Bullet *pBullet, BulletData *pBulletData) {
    pBulletData->time = pBullet->time;
    pBulletData->xVel = pBullet->xVel;
    pBulletData->yVel = pBullet->yVel;
    pBulletData->xCord = pBullet->xCord;
    pBulletData->yCord = pBullet->yCord;
}

/* Update bullet data to bullet */
void update_recived_bullet_data(Bullet *pBullet, BulletData *pBulletData) {
    pBullet->time = pBulletData->time;
    pBullet->xVel = pBulletData->xVel;
    pBullet->yVel = pBulletData->yVel;
    pBullet->xCord = pBulletData->xCord;
    pBullet->yCord = pBulletData->yCord;
}