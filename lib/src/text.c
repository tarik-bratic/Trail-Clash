#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../include/text.h"

/* text struct */
struct text {
    SDL_Rect txtRect;
    SDL_Texture *pTexture;
    SDL_Renderer *pRenderer;
};

/** 
 * Allocate a pointer and create a text with the desired content
 * \return Return pointer on success, if not then return NULL
*/
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

/** 
 * Create a font from a file, using a specified point size.
 * \param pFont Identifier of TTF_Font.
 * \param file Path to font file.
 * \param size_f Size of the font.
 * The function also check for error.
 * \returns Returns the font that was created or NULL on failure; TTF_GetError() for more information.
*/
TTF_Font *create_font(TTF_Font *pFont, char *file, int size_f) {

    pFont = TTF_OpenFont(file, size_f);

    if ( !pFont ) {
      printf("Error (font): %s\n", TTF_GetError());
      return 0;
    }

    return pFont;

}

/* Render a copy of the text to screen */
void draw_text(Text *pText) {
    SDL_RenderCopy(pText->pRenderer, pText->pTexture, NULL, &pText->txtRect);
}

/* Destory text texture and free memory */
void destroy_text(Text *pText) {
    SDL_DestroyTexture(pText->pTexture);
    free(pText);
}