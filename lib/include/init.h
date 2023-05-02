#ifndef init_h
#define init_h

int init_sdl_libraries();
SDL_Window *main_wind(const char *title, SDL_Window *pWindow);
SDL_Renderer *create_render(SDL_Renderer *pRenderer, SDL_Window *pWindow);

#endif