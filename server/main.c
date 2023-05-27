#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "../lib/include/data.h"
#include "../lib/include/text.h"
#include "../lib/include/init.h"
#include "../lib/include/snake.h"

/* Server Game struct */
typedef struct game {

  int roundCount;

  // SNAKE
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke[MAX_SNKES];
  int collided;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress clients[MAX_SNKES];
  ServerData sData;
  int connected_Clients;

  GameState state;

} Game;

int init_structure(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int init_allSnakes(Game *pGame);
int create_server(Game *pGame);
void reset_game(Game *pGame);
void set_up_game(Game *pGame);
void render_game(Game *pGame);
void send_gameData(Game *pGame);
void create_snakePointers(Game *pGame);
void collision_counter(Game *pGame);
void execute_command(Game *pGame, ClientData cData);
void add_client(Game *pGame, IPaddress address, IPaddress clients[], int *pNumOfClients);

int main(int argv, char** args) {
  
  Game g = { 0 };

  printf("Initiating structure...\n");
  if (!init_structure(&g)) return 1;

  printf("Running... \n");
  run(&g);

  printf("Closeing...\n");
  close(&g);

  return 0;

}

/* Initialize structe of the game with SDL libraries and other attributes */
int init_structure(Game *pGame) {

  srand(time(NULL));

  // Default values
  pGame->state = MENU;
  pGame->roundCount = 0;
  pGame->connected_Clients = 0;
  pGame->sData.maxClients = MAX_SNKES;

  if ( !init_sdl_libraries() ) return 0; 

  if ( !create_server(pGame) ) return 0;

  // Hidden window
  pGame->pWindow = server_wind("Server", pGame->pWindow);
  if ( !pGame->pWindow) close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if ( !pGame->pRenderer) close(pGame);

  if ( !init_allSnakes(pGame) ) return 0;

  return 1;

}

/* Main loop of the game. Updates the snakes attributes. Looking for input. Sends and recives new data. */
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;

  int showMess = 0;
  int firstStart = 0;
  int countdownCompleted = 0;

  int closeRequest = 0;
  while(!closeRequest) {
    switch (pGame->state) {
      // The game is running
      case RUNNING:

        if (showMess) {
          printf("Game state: Running\n");
          showMess--;
        }

        create_snakePointers(pGame);
        
        send_gameData(pGame);

        render_game(pGame);

        if (!firstStart) {
          SDL_Delay(3000);
          firstStart++;
        }
        
        // Update new recived data to client data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {
          memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));
          execute_command(pGame, cData);
        }

        // All players has disconnected
        if (pGame->sData.maxClients == 0) closeRequest = 1;

        // Looking if there is an input
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) closeRequest = 1;
        }

        //Check if one Snake left, if so reset game and display winner (filip)
        collision_counter(pGame);
        if (pGame->collided == 1) reset_game(pGame);

      break;
      // Waiting for all clients
      case MENU:

        if (!showMess) {
          printf("Game state: Start\n");
          showMess++;
        }

        // Exit the program (ctrl + c) 
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) closeRequest = 1;

        // If new data recived add client
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {

          if (cData.command != DISC) {
            add_client(pGame, pGame->pPacket->address, pGame->clients, &(pGame->connected_Clients));

            if (pGame->connected_Clients == MAX_SNKES) set_up_game(pGame);
          }

        }

      break;
    }
  }

}

/* Create an array of pointers to other snakes */
void create_snakePointers(Game *pGame) {

  for(int i = 0; i < MAX_SNKES; i++) {

    Snake *otherSnakes[MAX_SNKES - 1];
    int otherSnakesIndex = 0;

    // Looping through all the other snakes to add them to the array
    for (int j = 0; j < MAX_SNKES; j++) {
      if (j != i) otherSnakes[otherSnakesIndex++] = pGame->pSnke[j];
    }
  
    // Update snake cord, send data
    update_snake(pGame->pSnke[i], otherSnakes, MAX_SNKES - 1);

  }

}

/* Setting up the game with default values and other attributes */
void set_up_game(Game *pGame) {

  printf("Setting up the game...\n");

  pGame->state = RUNNING;

  for (int i = 0; i < MAX_SNKES; i++) 
    reset_snake(pGame->pSnke[i], i);

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
      
		SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);

  }

}

/* Add new clients to the server if they joined */
void add_client(Game *pGame, IPaddress address, IPaddress clients[], int *pNumOfClients) {

  ClientData cData;

  memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));

	for (int i = 0; i < *pNumOfClients; i++) 
    if(address.host == clients[i].host && address.port == clients[i].port) return;

	clients[*pNumOfClients] = address;
  strcpy(pGame->sData.playerName[*pNumOfClients], cData.clientName);
	(*pNumOfClients)++;

  printf("Client (%s) has joined...\n", cData.clientName);

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
        pGame->sData.maxClients -= 1;
        printf("Client (%s) has disconnected...\n", cData.clientName);
      break;
    }

}

/* Render a snake to the window */
void render_game(Game *pGame) {

  for (int i = 0; i < MAX_SNKES; i++) {
    draw_snake(pGame->pSnke[i]);
    draw_trail(pGame->pSnke[i]);
  }

  SDL_RenderPresent(pGame->pRenderer);

}

/* Checks nrOfCollisions, if 1 snake alive sets collided to 1 */
void collision_counter(Game *pGame) {

  int nrOfCollisions = 0;

  for (int i = 0; i < MAX_SNKES; i++) {
    if (pGame->pSnke[i]->snakeCollided == 1) nrOfCollisions++;
  }

  if (nrOfCollisions >= MAX_SNKES - 1) pGame->collided = 1;

}

/* Checks nrOfCollisions, if 1 snake alive sets collided to 1 */
void reset_game(Game *pGame) {
  
  for (int i = 0; i < MAX_SNKES; i++) {
    reset_snake(pGame->pSnke[i], i);
  }

  pGame->collided = 0;
  pGame->roundCount++;

  if(pGame->roundCount == MAX_ROUNDS){
    pGame->state = MENU;
    pGame->roundCount = 0;
    pGame->connected_Clients = 0;
  } else {
    SDL_Delay(3000);
  }

}

/** 
 * Create all snakes (players) in an array.
 * Checking for errors.
*/
int init_allSnakes(Game *pGame) {

  for (int i = 0; i < MAX_SNKES; i++)
    pGame->pSnke[i] = create_snake(pGame->pRenderer, i, i);

  for (int i = 0; i < MAX_SNKES; i++) {
    if (!pGame->pSnke[i]) {
      printf("Error: %s", SDL_GetError());
      close(pGame);
      return 0;
    }
  }

}

/* Create a server */
int create_server(Game *pGame) {

  if ( !(pGame->pSocket = SDLNet_UDP_Open(UDP_SERVER_PORT)) ) {
    printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
    close(pGame);
    return 0;
  }

  if ( !(pGame->pPacket = SDLNet_AllocPacket(DATA_SIZE)) ) {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    close(pGame);
    return 0;
  }

}

/* Destoryes various SDL libraries and snakes. Safe way when exiting game. */
void close(Game *pGame) {

  for(int i=0; i < MAX_SNKES; i++) 
    if(pGame->pSnke[i]) destroy_snake(pGame->pSnke[i]);

  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);
  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);

  // Network
  if (pGame->pPacket) SDLNet_FreePacket(pGame->pPacket);
	if (pGame->pSocket) SDLNet_UDP_Close(pGame->pSocket);

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}