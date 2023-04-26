#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include "../lib/include/data.h"
#include "../lib/include/text.h"
#include "../lib/include/snake.h"
#include "../lib/include/init.h"

/* Client Game struct (Snake, UI, Network) */
typedef struct game {

  // SNAKE
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke[MAX_SNKES];
  int num_of_snkes;
  int snkeID;

  // UI
  TTF_Font *pNetFont;
  Text *pStartText, *pWaitingText;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress serverAdd;

  GameState state;

} Game;

int init_structure(Game *pGame);

void run(Game *pGame);
void close(Game *pGame);
void render_snake(Game *pGame);
void update_server_data(Game *pGame);
void input_handler(Game *pGame, SDL_Event *pEvent);

int main(int argv, char** args) {
  
  Game g = { 0 };

  if ( !init_structure(&g) ) return 1;
  run(&g);
  close(&g);

  return 0;

}

/* Initialize structe of the game with SDL libraries and other attributes */
int init_structure(Game *pGame) {

  srand(time(NULL));

  if ( !init_sdl_libraries() ) return 0;

  pGame->pWindow = main_wind("Trail Clash - client", pGame->pWindow);
  if ( !pGame->pWindow ) close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if ( !pGame->pRenderer) close(pGame);

  // Create font and check for error 
  // FORTSÄTTER MED ATT LÄGGA TILL FUNKTIONER I INIT.C
  pGame->pNetFont = TTF_OpenFont("../lib/resources/arial.ttf", 30);

  if ( !pGame->pNetFont ) {
    printf("Error (font): %s\n", TTF_GetError());
    close(pGame);
    return 0;
  }

  // Establish client to server
  if ( !(pGame->pSocket = SDLNet_UDP_Open(0)) ) {
    printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
    return 0;
  }

  if (SDLNet_ResolveHost(&(pGame->serverAdd), "192.168.1.112", 2000)) {
    printf("SDLNet_ResolveHost (127.0.0.1: 2000): %s\n", SDLNet_GetError());
    return 0;
  }

  if ( !(pGame->pPacket = SDLNet_AllocPacket(512)) ) {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    return 0;
  }

  // set host and port to packet
  pGame->pPacket->address.host = pGame->serverAdd.host;
  pGame->pPacket->address.port = pGame->serverAdd.port;

  // Create all snakes, look for error
  for(int i = 0; i < MAX_SNKES; i++)
    pGame->pSnke[i] = create_snake(i, pGame->pRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);

  for(int i = 0; i < MAX_SNKES; i++) {
    if(!pGame->pSnke[i]) {
      printf("Error: %s", SDL_GetError());
      close(pGame);
      return 0;
    }
  }

  // Create different texts, look for error
  pGame->pStartText = create_text(pGame->pRenderer, 238,168,65, pGame->pNetFont,
    "[SPACE] to join", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);
  pGame->pWaitingText = create_text(pGame->pRenderer, 238,168,65,pGame->pNetFont,
    "Waiting for server...", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  if (!pGame->pStartText || !pGame->pWaitingText) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  pGame->num_of_snkes = MAX_SNKES;
  pGame->state = START;

  return 1;

}

/* Main loop of the game. Updates the snakes attributes. Looking for input.
Sends and recives new data. */
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;
  int joining = 0;
  int closeRequest = 0;

  while(!closeRequest) {

    switch (pGame->state) {

      // The game is running
      case RUNNING:

        // Update new recived data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
          update_server_data(pGame);
        }

        // Looking if there is an input
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) closeRequest = 1;
          else input_handler(pGame, &event);
        }

        // Update snake cord and bullet cord
        for(int i = 0; i < MAX_SNKES; i++)
          update_snake(pGame->pSnke[i]);
        
        // Render snake to the window
        render_snake(pGame);
        
      break;

      // Lobby (waiting to start)
      case START:

        // If you haven't joined display start text if not display waiting text
        if (!joining) {
          draw_text(pGame->pStartText);
        } else {
          SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
          SDL_RenderClear(pGame->pRenderer);
          SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);
          draw_text(pGame->pWaitingText);
        }

        // Update screen with new render
        SDL_RenderPresent(pGame->pRenderer);

        // Looking if player has pressed spacebar
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) {
            closeRequest = 1;
          } else if (!joining && event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
              // Change client data and copy to packet
              joining = 1;
              cData.command = READY;
              cData.snkeNumber = -1;
              memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
		          pGame->pPacket->len = sizeof(ClientData);
          }
        }

        // Send client data packet
        if (joining) SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);

        // Update new recived data
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
          update_server_data(pGame);
          if (pGame->state == RUNNING) joining = 0;
        }

        break;

    }

    //SDL_Delay(ONE_MS); Kanske kan användas senare. Låt stå.

  }

}

/* Copy new data to server data and updates snake data */
void update_server_data(Game *pGame) {

    ServerData srvData;

    memcpy(&srvData, pGame->pPacket->data, sizeof(ServerData));
    pGame->snkeID = srvData.snkeNum;
    pGame->state = srvData.gState;

    for(int i=0; i < MAX_SNKES; i++)
      update_recived_snake_data(pGame->pSnke[i], &(srvData.snakes[i]));

}

/* Manage various inputs (keypress, mouse) */
void input_handler(Game *pGame, SDL_Event *pEvent) {

  if (pEvent->type == SDL_KEYDOWN) {

    ClientData cData;
    cData.snkeNumber = pGame->snkeID;

    switch(pEvent->key.keysym.scancode){
      case SDL_SCANCODE_A:
      case SDL_SCANCODE_LEFT:
        turn_left(pGame->pSnke[pGame->snkeID]);
        cData.command = LEFT;
        break;
      case SDL_SCANCODE_D:
      case SDL_SCANCODE_RIGHT:
        turn_right(pGame->pSnke[pGame->snkeID]);
        cData.command = RIGHT;
      break;
    }

    // Send data
    memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
    pGame->pPacket->len = sizeof(ClientData);
    SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);

  }

}

/* Destoryes various SDL libraries and snakes. Safe way when exiting game. */
void close(Game *pGame) {

  for(int i=0; i < MAX_SNKES; i++) 
    if(pGame->pSnke[i]) destroy_snake(pGame->pSnke[i]);

  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);

  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);

  if(pGame->pWaitingText) destroy_text(pGame->pWaitingText);

  if(pGame->pStartText) destroy_text(pGame->pStartText);  

  if(pGame->pNetFont) TTF_CloseFont(pGame->pNetFont);

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}

/* Render a snake to the window */
void render_snake(Game *pGame) {
  SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);
  for(int i = 0; i < MAX_SNKES; i++)
    draw_snake(pGame->pSnke[i]);
  SDL_RenderPresent(pGame->pRenderer);
}