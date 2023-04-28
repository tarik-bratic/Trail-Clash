#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include "../lib/include/data.h"
#include "../lib/include/text.h"
#include "../lib/include/snake.h"

/* Game struct (Snake, UI, Network) */
typedef struct game {

  // SNAKE
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke[MAX_SNKES];
  int num_of_snkes;

  // UI
  TTF_Font *pNetFont;
  Text *pWaitingText;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress clients[MAX_SNKES];
  ServerData sData;
  int num_of_clients;

  GameState state;

} Game;

void run(Game *pGame);
void close(Game *pGame);
void set_up_game(Game *pGame);
void render_snake(Game *pGame);
void send_gameData(Game *pGame);
void execute_command(Game *pGame, ClientData cData);
void add_client(IPaddress address, IPaddress clients[], int *pNumOfClients);
int init_structure(Game *pGame);

int main(int argv, char** args) {
  
  Game g = { 0 };

  if (!init_structure(&g)) return 1;
  run(&g);
  close(&g);

  return 0;

}

/* Initialize structe of the game with SDL libraries and other attributes */
int init_structure(Game *pGame) {

  srand(time(NULL));
  pGame->sData.connPlayers = MAX_SNKES;

  // Initialaze SDL library
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
    printf("Error (SDL): %s\n", SDL_GetError());
    return 0;
  }

  // Initialaze TTF library
  if (TTF_Init() != 0) {
    printf("Error (TTF): %s\n", TTF_GetError());
    SDL_Quit();
    return 0;
  }

  // Initialaze SDLNet library
  if (SDLNet_Init()) {
    fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
    TTF_Quit();
    SDL_Quit();
		return 0;
  }

  // Create a window, look for error
  pGame->pWindow = SDL_CreateWindow("Trail Clash - server", SDL_WINDOWPOS_CENTERED, 
    SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

  if (!pGame->pWindow) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;  
  }

  // Create renderer, look for error
  pGame->pRenderer = SDL_CreateRenderer(pGame->pWindow, -1, 
    SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

  if (!pGame->pRenderer) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;  
  }

  // Create font and check for error
  pGame->pNetFont = TTF_OpenFont("../lib/resources/arial.ttf", 30);

  if ( !pGame->pNetFont ) {
    printf("Error: %s\n", TTF_GetError());
    close(pGame);
    return 0;
  }

  // Establish server to client 
  if ( !(pGame->pSocket = SDLNet_UDP_Open(2000)) ) {
    printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
    close(pGame);
    return 0;
  }

  if ( !(pGame->pPacket = SDLNet_AllocPacket(512)) ) {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    close(pGame);
    return 0;
  }

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

  // Create texts, look for error
  pGame->pWaitingText = create_text(pGame->pRenderer, 238,168,65,pGame->pNetFont,
    "Waiting for client...", WINDOW_WIDTH/2, WINDOW_HEIGHT/2+100);

  if (!pGame->pWaitingText) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  pGame->state = START;
  pGame->num_of_clients = 0;
  pGame->num_of_snkes = MAX_SNKES;

  return 1;

}

/* Main loop of the game. Updates the snakes attributes. Looking for input.
Sends and recives new data. */
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;
  int closeRequest = 0;

  while(!closeRequest) {

    switch (pGame->state) {

      // The game is running
      case RUNNING:

        send_gameData(pGame);
        
        // Update new recived data to client data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {
          memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));
          execute_command(pGame, cData);
        }

        // If exiting the program
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) closeRequest = 1;
        }

        // Update snake cord and bullet cord
        for(int i = 0; i < MAX_SNKES; i++)
          update_snake(pGame->pSnke[i]);

        // Render snake to the window
        render_snake(pGame);

        break;

      // Lobby (waiting to start)
      case START:

        // Display waiting text
        draw_text(pGame->pWaitingText);
        SDL_RenderPresent(pGame->pRenderer);

        // If not exiting program recive new data 
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
          closeRequest = 1;
        }

        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {
          add_client(pGame->pPacket->address, pGame->clients, &(pGame->num_of_clients));
          if (pGame->num_of_clients == MAX_SNKES) set_up_game(pGame);
        }

        break;
    }

    //SDL_Delay(ONE_MS); Kanske kan användas senare. Låt stå.

  }

}

/* Setting up the game with default values and other attributes */
void set_up_game(Game *pGame) {

    for(int i = 0; i < MAX_SNKES; i++) 
      reset_snake(pGame->pSnke[i]);

    pGame->num_of_snkes = MAX_SNKES;
    pGame->state = RUNNING;

}

/* Send data to Game Data to packet */
void send_gameData(Game *pGame) {
   
    pGame->sData.gState = pGame->state;

    for(int i = 0; i < MAX_SNKES; i++)
      update_snakeData(pGame->pSnke[i], &(pGame->sData.snakes[i]));

    for(int i = 0; i < MAX_SNKES; i++) {
      pGame->sData.snkeNum = i;
      memcpy(pGame->pPacket->data, &(pGame->sData), sizeof(ServerData));
		  pGame->pPacket->len = sizeof(ServerData);
      pGame->pPacket->address = pGame->clients[i];
		  SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);
    }

}

/* Add new clients to the server if they joined*/
void add_client(IPaddress address, IPaddress clients[], int *pNumOfClients) {

	for(int i = 0; i < *pNumOfClients; i++) 
    if(address.host == clients[i].host && address.port == clients[i].port) return;

	clients[*pNumOfClients] = address;
	(*pNumOfClients)++;

}

/* Execute various commands for the player based on the recived data from client data */
void execute_command(Game *pGame, ClientData cData) {

    switch (cData.command)
    {
      case LEFT:
        turn_left(pGame->pSnke[cData.snkeNumber]);
        break;
      case RIGHT:
        turn_right(pGame->pSnke[cData.snkeNumber]);
        break;
    }

}

/* Destoryes various SDL libraries and snakes. Safe way when exiting game. */
void close(Game *pGame) {

  for(int i=0; i < MAX_SNKES; i++) 
    if(pGame->pSnke[i]) destroy_snake(pGame->pSnke[i]);

  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);

  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);

  if(pGame->pWaitingText) destroy_text(pGame->pWaitingText);

  if(pGame->pNetFont) TTF_CloseFont(pGame->pNetFont);

  if(pGame->pPacket) SDLNet_FreePacket(pGame->pPacket);

	if(pGame->pSocket) SDLNet_UDP_Close(pGame->pSocket);

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