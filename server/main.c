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
#include "../lib/include/item.h"

/* Server Game struct */
typedef struct game {

  // SNAKE
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke[MAX_SNKES];
  int collided;

  ClientName cName[MAX_SNKES];

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress clients[MAX_SNKES];
  ServerData sData;
  int connected_Clients;

  //ITEM
  ItemImage *pItemImage[MAX_ITEMS];
  Item *pItems[MAX_ITEMS];
  int numItems;

  //TIMER
  int startTime;

  GameState state;

} Game;

int init_structure(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int init_Items(Game *pGame);
int init_allSnakes(Game *pGame);
int spawnItem(Game *pGame, int NrOfItems);
int create_server(Game *pGame);
void collision_counter(Game *pGame);
void reset_game(Game *pGame);
void set_up_game(Game *pGame);
void render_snake(Game *pGame);
void send_gameData(Game *pGame);
void execute_command(Game *pGame, ClientData cData);
void add_client(Game *pGame, ClientName cName[], IPaddress address, IPaddress clients[], int *pNumOfClients);

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
  pGame->state = START;
  pGame->numItems = MAX_ITEMS;
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

  if ( !init_Items(pGame) ) return 0;

  return 1;

}

/* Main loop of the game. Updates the snakes attributes. Looking for input. Sends and recives new data. */
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;

  int replace = 0;
  int boostKey = 0;
  int nrOfItems = 0;

  int showMess = 0;

  int closeRequest = 0;
  while(!closeRequest) {
    switch (pGame->state) {
      case RUNNING: // The game is running

        if (showMess) {
          printf("Game state: Running\n");
          showMess--;
        }
        
        send_gameData(pGame);
        
        // Update new recived data to client data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {
          memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));
          execute_command(pGame, cData);
        }

        // All players has disconnected
        if (pGame->sData.maxClients == 0) closeRequest = 1;

        // Exit the program
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) closeRequest = 1;
        }

        // Create an array of pointers to other snakes
        for(int i = 0; i < MAX_SNKES; i++) {

          Snake *otherSnakes[MAX_SNKES - 1];
          int otherSnakesIndex = 0;

          // Looping through all the other snakes to add them to the array
          for (int j = 0; j < MAX_SNKES; j++) {
            if (j != i) otherSnakes[otherSnakesIndex++] = pGame->pSnke[j];
          }
  
          // Update snake cord
          update_snake(pGame->pSnke[i], otherSnakes, MAX_SNKES - 1, boostKey);

          for(int j = 0; j < MAX_ITEMS; j++) {

            if(collideSnake(pGame->pSnke[i], getRectItem(pGame->pItems[j]))) {
              boostKey = 1;
              pGame->startTime = 0;
              updateItem(pGame->pItems[j]);
              nrOfItems--;
              replace = j;
            }

            if(boostKey > 0) {
              pGame->startTime++;
              if(pGame->startTime == 200) boostKey = 0;
            }

          }

        }

        nrOfItems = spawnItem(pGame, nrOfItems);

        render_snake(pGame);

        //Check if one Snake left, if so reset game and display winner (filip)
        //collision_counter(pGame);
        // if (pGame->collided==1) reset_game(pGame);

      break;
      case START: // Waiting for all clients

        if (!showMess) {
          printf("Game state: Start\n");
          showMess++;
        }

        // Exit the program (ctrl + c) 
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) closeRequest = 1;

        // If new data recived add client
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {

          if (cData.command != DISC) {
            add_client(pGame, pGame->cName, pGame->pPacket->address, pGame->clients, &(pGame->connected_Clients));

            if (pGame->connected_Clients == MAX_SNKES) set_up_game(pGame);
          }

        }

      break;
    }
  }

}

/* Setting up the game with default values and other attributes */
void set_up_game(Game *pGame) {

  printf("Setting up the game...\n");

  pGame->state = RUNNING;

  for (int i = 0; i < MAX_SNKES; i++) 
    reset_snake(pGame->pSnke[i]);

}

/* Send data to Game Data to packet */
void send_gameData(Game *pGame) {
   
    pGame->sData.gState = pGame->state;

    for (int i = 0; i < MAX_SNKES; i++)
      update_snakeData(pGame->pSnke[i], &(pGame->sData.snakes[i]));

    for (int i = 0; i < MAX_SNKES; i++) {

      pGame->sData.snkeNum = i;
      for (int j = 0; j < MAX_SNKES; j++) {
        strcpy(pGame->sData.playerName[j], pGame->cName[j].name);
      }
      memcpy(pGame->pPacket->data, &(pGame->sData), sizeof(ServerData));
		  pGame->pPacket->len = sizeof(ServerData);
      pGame->pPacket->address = pGame->clients[i];
      
		  SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);

    }

}

/* Add new clients to the server if they joined */
void add_client(Game *pGame, ClientName cName[], IPaddress address, IPaddress clients[], int *pNumOfClients) {

  ClientData cData;

  memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));

	for (int i = 0; i < *pNumOfClients; i++) 
    if(address.host == clients[i].host && address.port == clients[i].port) return;

	clients[*pNumOfClients] = address;
  strcpy(cName[*pNumOfClients].name, cData.playerName);
	(*pNumOfClients)++;

  printf("Client (%s) has joined...\n", cData.playerName);

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
        printf("Client (%s) has disconnected...\n", cData.playerName);
      break;
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

/* Checks nrOfCollisions, if 1 snake alive sets collided to 1 */
void collision_counter(Game *pGame) {

  int nrOfCollisions = 0;

  for (int i = 0; i < MAX_SNKES; i++) {
    if (pGame->pSnke[i]->snakeCollided == 1) nrOfCollisions++;
  }

  if (nrOfCollisions == MAX_SNKES - (MAX_SNKES - 1)) pGame->collided = 1;

}

/* Checks nrOfCollisions, if 1 snake alive sets collided to 1 */
void reset_game(Game *pGame) {
  
  for (int i = 0; i < MAX_SNKES; i++) {
    reset_snake(pGame->pSnke[i]);
  }

  pGame->collided = 0;
  pGame->state = START;
  pGame->connected_Clients = 0;
  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);

}

/* Spawn Item by creating image and creating object */
int spawnItem(Game *pGame, int NrOfItems) {

  int spawn = rand() % 500;

  if(spawn == 0 && NrOfItems != MAX_ITEMS) {
    pGame->pItemImage[NrOfItems] = createItemImage(pGame->pRenderer);
    pGame->pItems[NrOfItems] = createItem(pGame->pItemImage[NrOfItems],WINDOW_WIDTH,WINDOW_HEIGHT, 0, 500, 500); 
    NrOfItems++;
  }

  return NrOfItems;

}

/* Creates Items and creates their image */
int init_Items(Game *pGame) {

  SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);

  for (int i = 0; i < MAX_ITEMS; i++) {
    pGame->pItemImage[i] = createItemImage(pGame->pRenderer);
    pGame->pItems[i] = createItem(pGame->pItemImage[i], WINDOW_WIDTH, WINDOW_HEIGHT, 0, 500, 500);
  }

  for (int i = 0; i < MAX_ITEMS; i++) {
    if (!pGame->pItemImage[i] || !pGame->pItems[i]) {
      printf("Error: %s", SDL_GetError());
      close(pGame);
      return 0;
    }
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

  if (pGame->pPacket) SDLNet_FreePacket(pGame->pPacket);

	if (pGame->pSocket) SDLNet_UDP_Close(pGame->pSocket);

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}