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
#include "../lib/include/init.h"
#include "../lib/include/snake.h"

/* Server Game struct (Snake, UI, Network) */
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

int init_structure(Game *pGame);
int init_allSnakes(Game *pGame);

void run(Game *pGame);
void close(Game *pGame);
void set_up_game(Game *pGame);
void render_snake(Game *pGame);
void send_gameData(Game *pGame);
void execute_command(Game *pGame, ClientData cData);
void add_client(IPaddress address, IPaddress clients[], int *pNumOfClients);

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
  pGame->state = START;
  pGame->num_of_clients = 0;
  pGame->num_of_snkes = MAX_SNKES;
  pGame->sData.connPlayers = MAX_SNKES;

  if ( !init_sdl_libraries() ) return 0; 

  pGame->pWindow = main_wind("Trail Clash - server", pGame->pWindow);
  if ( !pGame->pWindow ) close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if ( !pGame->pRenderer) close(pGame);

  pGame->pNetFont = create_font(pGame->pNetFont, "../lib/resources/Sigmar-Regular.ttf", 20);
  if ( !pGame->pNetFont ) close(pGame);

  // Create texts, look for error
  pGame->pWaitingText = create_text(pGame->pRenderer, 238,168,65,pGame->pNetFont,
    "Waiting for client ...", WINDOW_WIDTH/2, WINDOW_HEIGHT/2+100);

  if (!pGame->pWaitingText) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  init_allSnakes(pGame);

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

        // If no player is connected close window
        if (pGame->sData.connPlayers == 0) closeRequest = 1;

        // If exiting the program
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) closeRequest = 1;
        }

        // Update snake cord and bullet cord
        for(int i = 0; i < MAX_SNKES; i++) {
          // Create an array of pointers to other snakes
          Snake *otherSnakes[MAX_SNKES - 1];
          int otherSnakesIndex = 0;
          // Looping through all the other snakes to add them to the array
          for (int j = 0; j < MAX_SNKES; j++) {
            if (j != i) {
              otherSnakes[otherSnakesIndex++] = pGame->pSnke[j];
            }
          }

          update_snake(pGame->pSnke[i], otherSnakes, MAX_SNKES - 1);
        }


        // Render snake to the window
        render_snake(pGame);
      break;
      // Waiting for all players
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

    for (int i = 0; i < MAX_SNKES; i++) 
      reset_snake(pGame->pSnke[i]);

    pGame->num_of_snkes = MAX_SNKES;
    pGame->state = RUNNING;

}

/* Send data to Game Data to packet */
void send_gameData(Game *pGame) {
   
    pGame->sData.gState = pGame->state;

    for (int i = 0; i < MAX_SNKES; i++)
      update_snakeData(pGame->pSnke[i], &(pGame->sData.snakes[i]));

    for (int i = 0; i < MAX_SNKES; i++) {
      pGame->sData.snkeNum = i;
      memcpy(pGame->pPacket->data, &(pGame->sData), sizeof(ServerData));
		  pGame->pPacket->len = sizeof(ServerData);
      pGame->pPacket->address = pGame->clients[i];
      
		  if ( !SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket) ) {
        printf("Error (UDP_Send): %s", SDLNet_GetError());
      }

    }

}

/* Add new clients to the server if they joined */
void add_client(IPaddress address, IPaddress clients[], int *pNumOfClients) {

	for (int i = 0; i < *pNumOfClients; i++) 
    if(address.host == clients[i].host && address.port == clients[i].port) return;

	clients[*pNumOfClients] = address;
	(*pNumOfClients)++;

}

/* Execute various commands for the player based on the recived data from client data */
void execute_command(Game *pGame, ClientData cData) {

    switch (cData.command) {
      case LEFT:
        turn_left(pGame->pSnke[cData.snkeNumber]);
        break;
      case RIGHT:
        turn_right(pGame->pSnke[cData.snkeNumber]);
        break;
      case DISC:
        pGame->sData.connPlayers -= 1;
    }

}

/* 
*  Create all snakes (players) in an array.
*  Checking for errors.
*/
int init_allSnakes(Game *pGame) {

  for (int i = 0; i < MAX_SNKES; i++)
    pGame->pSnke[i] = create_snake(i, pGame->pRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);

  for (int i = 0; i < MAX_SNKES; i++) {
    if (!pGame->pSnke[i]) {
      printf("Error: %s", SDL_GetError());
      close(pGame);
      return 0;
    }
  }

}

/* Render a snake to the window */
void render_snake(Game *pGame) {

  SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);

  for (int i = 0; i < MAX_SNKES; i++) {
    draw_snake(pGame->pSnke[i]);
    draw_trail(pGame->pSnke[i]);
  }

  SDL_RenderPresent(pGame->pRenderer);

}

/* Destoryes various SDL libraries and snakes. Safe way when exiting game. */
void close(Game *pGame) {

  for(int i=0; i < MAX_SNKES; i++) 
    if(pGame->pSnke[i]) destroy_snake(pGame->pSnke[i]);

  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);

  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);

  if (pGame->pWaitingText) destroy_text(pGame->pWaitingText);

  if (pGame->pNetFont) TTF_CloseFont(pGame->pNetFont);

  if (pGame->pPacket) SDLNet_FreePacket(pGame->pPacket);

	if (pGame->pSocket) SDLNet_UDP_Close(pGame->pSocket);

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}