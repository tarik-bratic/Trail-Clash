#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../include/text.h"

/* text struct (rect, texture, render)*/
struct text {
    SDL_Rect txtRect;
    SDL_Texture *pTexture;
    SDL_Renderer *pRenderer;
};

/* Function to create a text with various attributes */
Text *create_text(SDL_Renderer *pRenderer, int r, int g, int b, TTF_Font *pFont, char *pString, int x, int y) {
    
    Text *pText = malloc(sizeof(struct text));
    
    pText->pRenderer = pRenderer;
    SDL_Color color = { r, g, b };
    SDL_Surface *pSurface = TTF_RenderText_Solid(pFont, pString, color);
    if (!pSurface) {
        printf("Error: %s\n", SDL_GetError());
        return NULL;
    }

    pText->pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
    SDL_FreeSurface(pSurface);
    if (!pText->pTexture) {
        printf("Error: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_QueryTexture(pText->pTexture, NULL, NULL, &pText->txtRect.w, &pText->txtRect.h);
    pText->txtRect.x = x - pText->txtRect.w/2;
    pText->txtRect.y = y - pText->txtRect.h/2;

    return pText;

}

/* Render a copy of the text to screen */
void draw_text(Text *pText) {
    SDL_RenderCopy(pText->pRenderer, pText->pTexture, NULL, &pText->txtRect);
}

/* Destory text texture and free */
void destroy_text(Text *pText) {
    SDL_DestroyTexture(pText->pTexture);
    free(pText);
}