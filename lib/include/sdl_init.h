#ifndef sdl_init_h
#define sdl_init_h

int init_sdl_libraries();
SDL_Window *client_wind(const char *title, SDL_Window *pWindow);
SDL_Window *server_wind(const char *title, SDL_Window *pWindow);
SDL_Renderer *create_render(SDL_Renderer *pRenderer, SDL_Window *pWindow);

#endif