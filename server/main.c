#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "../lib/include/text.h"
#include "../lib/include/snake.h"
#include "../lib/include/sdl_init.h"
#include "../lib/include/game_data.h"

/* Server Game struct */
typedef struct game {

  int deathCount;
  int ctrlDeath[MAX_SNKES];

  // FLAGS
  int created;
  int showMess;
  int roundCount;
  int firstStart;

  // SNAKE
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke[MAX_SNKES];
  int collision;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress clients[MAX_SNKES];
  ServerData sData;
  int connectedClients;

  GameState state;

} Game;

int init_structure(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int init_allSnakes(Game *pGame);
void finish_game(Game *pGame);
int create_server(Game *pGame);
void new_game(Game *pGame);
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

  // Default values
  pGame->state = MENU;

  pGame->connectedClients = 0;
  pGame->sData.maxClients = MAX_SNKES;

  pGame->roundCount = 0;

  pGame->deathCount = 1;
  for (int i = 0; i < MAX_SNKES; i++)
    pGame->ctrlDeath[i] = 0;
  
  pGame->created = 0;
  pGame->showMess = 0;
  pGame->firstStart = 0;

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

  int closeRequest = 0;
  while(!closeRequest) {
    switch (pGame->state) {
      // The game is running
      case RUNNING:

        create_snakePointers(pGame);
        
        send_gameData(pGame);

        if (!pGame->firstStart) {
          printf("Game state: Running\n");
          render_game(pGame);
          SDL_Delay(3000);
          pGame->firstStart++;
        }
        
        // Update new recived data to client data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {
          memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));
          execute_command(pGame, cData);
        }

        render_game(pGame);

        if (pGame->collision == 1) new_game(pGame);

        // All players has disconnected
        if (pGame->sData.maxClients == 0) closeRequest = 1;

        // Looking if there is an input
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) closeRequest = 1;
        }

        //Check if one Snake left, if so reset game and display winner (filip)
        collision_counter(pGame);

      break;
      // Waiting for all clients
      case MENU:

        if (!pGame->showMess) {
          printf("Game state: Menu\n");
          pGame->showMess++;
        }

        // Exit the program (ctrl + c) 
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) closeRequest = 1;

        // If new data recived add client
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) && !pGame->created) {

          if (cData.command != DISC) {
            add_client(pGame, pGame->pPacket->address, pGame->clients, &(pGame->connectedClients));

            if (pGame->connectedClients == MAX_SNKES) set_up_game(pGame);
          }

        }

      break;
    }
  }

}

/* Create an array of pointers to other snakes */
void create_snakePointers(Game *pGame) {

  for(int i = 0; i < MAX_SNKES; i++) {

    int oppIndex = 0;
    Snake *opponents[MAX_SNKES - 1];

    // Looping through all the other snakes to add them to the array
    for (int j = 0; j < MAX_SNKES; j++)
      if (j != i) opponents[oppIndex++] = pGame->pSnke[j];
  
    // Update snake cord, send data
    update_snake(pGame->pSnke[i], opponents, MAX_SNKES - 1);

  }

}

/* Setting up the game with default values and other attributes */
void set_up_game(Game *pGame) {

  printf("Setting up the game...\n");

  pGame->state = RUNNING;
  pGame->created = 1;

  for (int i = 0; i < MAX_SNKES; i++) {
    reset_snake(pGame->pSnke[i], i);
    pGame->sData.died[i] = 0;
  }

}

void finish_game(Game *pGame) {

  printf("Leaving the game...\n");

  pGame->created = 0;
  pGame->state = MENU;
  pGame->showMess = 0;
  pGame->firstStart = 0;
  pGame->deathCount = 1;
  pGame->roundCount = 0;
  pGame->connectedClients = 0;
  pGame->sData.maxClients = MAX_SNKES;
  for (int i = 0; i < MAX_SNKES; i++) {
    pGame->ctrlDeath[i] = 0;
    pGame->sData.died[i] = 0;
  }

}

/* Send data to Game Data to packet */
void send_gameData(Game *pGame) {

  pGame->sData.gState = pGame->state;

  for (int i = 0; i < MAX_SNKES; i++)
    update_snakeData(pGame->pSnke[i], &(pGame->sData.snakes[i]));

  for (int i = 0; i < MAX_SNKES; i++) {

    pGame->sData.snkeNum = i;
    if (pGame->pSnke[i]->snakeCollided && !pGame->ctrlDeath[i]) {
      pGame->sData.died[i] += pGame->deathCount;
      pGame->deathCount += 1;
      pGame->ctrlDeath[i] = 1;
    }

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

/* Check if there is one snake left alive to trigger flag collision to 1 */
void collision_counter(Game *pGame) {

  int numOfCollisions = 0;

  for (int i = 0; i < MAX_SNKES; i++)
    if (pGame->pSnke[i]->snakeCollided == 1) numOfCollisions++;

  if (numOfCollisions >= MAX_SNKES - 1) {

    pGame->collision = 1;
    for (int i = 0; i < MAX_SNKES; i++)
      if (!pGame->pSnke[i]->snakeCollided) pGame->sData.died[i] += pGame->deathCount;

  }

}

/* Check if a new round should start or to display the winner */
void new_game(Game *pGame) {

  pGame->roundCount++;
  pGame->collision = 0;
  pGame->deathCount = 1;
  
  for (int i = 0; i < MAX_SNKES; i++) {
    reset_snake(pGame->pSnke[i], i);
    pGame->ctrlDeath[i] = 0;
  }

  if(pGame->roundCount == MAX_ROUNDS){
    finish_game(pGame);
  } 
  else SDL_Delay(3000);

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