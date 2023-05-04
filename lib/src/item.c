#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <math.h>
#include "../include/item.h"
#include "../include/data.h"

struct itemImage{
    SDL_Renderer *pRenderer;
    SDL_Texture *pTexture;    
};

struct item{
    float x, y;
    int window_width,window_height, renderAngle;
    SDL_Renderer *pRenderer;
    SDL_Texture *pTexture;
    SDL_Rect rect;
}; 

static void getStartValues(Item *a);

ItemImage *createItemImage(SDL_Renderer *pRenderer){
    static ItemImage* pItemImage = NULL;
    if(pItemImage==NULL){
        pItemImage = malloc(sizeof(struct itemImage));
        SDL_Surface *surface = IMG_Load("../lib/resources/Item.png");
        if(!surface){
            printf("Error: %s\n",SDL_GetError());
            return NULL;
        }

    //create texture and check for error
        pItemImage->pRenderer = pRenderer;
        pItemImage->pTexture = SDL_CreateTextureFromSurface(pRenderer, surface);
        SDL_FreeSurface(surface);
        if(!pItemImage->pTexture){
            printf("Error: %s\n",SDL_GetError());
            return NULL;
        }
    }
    return pItemImage;
}

Item *createItem(ItemImage *pItemImage, int window_width, int window_height, int test){
    Item *pItem = malloc(sizeof(struct item));
    pItem->pRenderer = pItemImage->pRenderer;
    pItem->pTexture = pItemImage->pTexture;
    pItem->window_width = window_width;
    pItem->window_height = window_height;
    SDL_QueryTexture(pItemImage->pTexture,NULL,NULL,&(pItem->rect.w),&(pItem->rect.h));
    int sizeFactor = 8; //rand()%8+1; //lagt till fixed size på item size
    pItem->rect.w/=sizeFactor;
    pItem->rect.h/=sizeFactor;
    if(test==0) getStartValues(pItem);
    else if (test==1) updateItem(pItem);

    return pItem;
}

static void getStartValues(Item *pItem){
    pItem->x=rand()%750;
    pItem->y=rand()%450;
    pItem->rect.x=pItem->x;
    pItem->rect.y=pItem->y;
}

SDL_Rect getRectItem(Item *pItem){
    return pItem->rect;
}

 void updateItem(Item *pItem){
    pItem->x=1000;
    pItem->y=1000;
    pItem->rect.x=pItem->x;
    pItem->rect.y=pItem->y; 
} 

void drawItem(Item *pItem){
    SDL_RenderCopyEx(pItem->pRenderer,pItem->pTexture,NULL,&(pItem->rect),0,NULL,SDL_FLIP_NONE);
}

void destroyItem(Item *pItem){
    free(pItem);
}

void destroyItemImage(ItemImage *pItemImage){
    SDL_DestroyTexture(pItemImage->pTexture);
}