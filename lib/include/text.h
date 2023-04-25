#ifndef text_h
#define text_h

typedef struct text Text;

Text *create_text(SDL_Renderer *pRenderer, int r, int g, int b, TTF_Font *pFont, char *pString, int x, int y);
void draw_text(Text *pText);
void destroy_text(Text *pText);

#endif