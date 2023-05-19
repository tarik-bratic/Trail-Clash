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
#include "../lib/include/item.h"
#include "../lib/include/snake.h"

/* Server Game struct */
typedef struct game {

  int deathCount;
  int ctrlDeath[MAX_SNKES];
  int roundCount;
  int showMess;
  int firstStart;
  int countdownCompleted;
  int created;

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

  //ITEM
  ItemImage *pItemImage[MAX_ITEMS];
  Item *pItems[MAX_ITEMS];
  ItemData iData;
  int numItems;
  int boostKey[MAX_SNKES];
  int nrOfItems;
  int replace;
  int items;

  //TIMER
  int startTime;

  GameState state;

} Game;

int init_structure(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int init_Items(Game *pGame);
int spawnItem(Game *pGame, int NrOfItems);
void send_itemData(Game *pGame, int spawn);
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
  pGame->deathCount = 1;
  pGame->roundCount = 0;
  pGame->connected_Clients = 0;
  pGame->sData.maxClients = MAX_SNKES;
  for (int i = 0; i < MAX_SNKES; i++)
    pGame->ctrlDeath[i] = 0;
  pGame->showMess = 0;
  pGame->firstStart = 0;
  pGame->countdownCompleted = 0;
  pGame->created = 0;
  pGame->numItems = MAX_ITEMS;
  pGame->nrOfItems = 1;
  pGame->items = 0;

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
        
        // Update new recived data to client data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {
          memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));
          execute_command(pGame, cData);
        }

        if (pGame->showMess) {
          printf("Game state: Running\n");
          pGame->showMess--;
        }

        if (!pGame->items) {
          init_Items(pGame);
          pGame->items++;
        }

        create_snakePointers(pGame);
        
        send_gameData(pGame);

        render_game(pGame);

        if (!pGame->firstStart) {
          SDL_Delay(3000);
          pGame->firstStart++;
        }

        if (pGame->collided == 1) reset_game(pGame);

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
          printf("Game state: Start\n");
          pGame->showMess++;
        }

        // Exit the program (ctrl + c) 
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) closeRequest = 1;

        // If new data recived add client
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) && !pGame->created) {

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

  pGame->nrOfItems = spawnItem(pGame, pGame->nrOfItems);

  for(int i = 0; i < MAX_SNKES; i++) {

    Snake *otherSnakes[MAX_SNKES - 1];
    int otherSnakesIndex = 0;

    // Looping through all the other snakes to add them to the array
    for (int j = 0; j < MAX_SNKES; j++) {
      if (j != i) otherSnakes[otherSnakesIndex++] = pGame->pSnke[j];
    }

    for(int j = 0; j < MAX_ITEMS; j++) {

      if( collideSnake(pGame->pSnke[i], getRectItem(pGame->pItems[j])) ) {
        pGame->boostKey[i] = 1;
        pGame->startTime = 0;
        updateItem(pGame->pItems[j]);
        pGame->nrOfItems--;
        pGame->replace = j;
      }
      
      if(pGame->boostKey > 0) {
        pGame->startTime++;
        if(pGame->startTime == 100) {
          for (int j = 0; j < MAX_SNKES; j++)
            pGame->boostKey[j] = 0;
        }
      }
    }
  
    // Update snake cord, send data
    update_snake(pGame->pSnke[i], otherSnakes, MAX_SNKES - 1, pGame->boostKey[i]);

  }

}

/* Creates Items and creates their image */
int init_Items(Game *pGame) {

  SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);

  for (int i = 0; i < MAX_ITEMS; i++) {
    send_itemData(pGame, 0);
    pGame->pItemImage[i] = createItemImage(pGame->pRenderer);
    pGame->pItems[i] = createItem(pGame->pItemImage[i], WINDOW_WIDTH, WINDOW_HEIGHT, 0, pGame->iData.xcoords, pGame->iData.ycoords);
  }

  for (int i = 0; i < MAX_ITEMS; i++) {
    if (!pGame->pItemImage[i] || !pGame->pItems[i]) {
      printf("Error: %s", SDL_GetError());
      close(pGame);
      return 0;
    }
  }

}

/* Spawn an item */
int spawnItem(Game *pGame, int NrOfItems) {

  int spawn = rand() % 500;
  if(spawn == 0) {
    if(NrOfItems != MAX_ITEMS) {
      send_itemData(pGame, 1);
      pGame->pItemImage[NrOfItems] = createItemImage(pGame->pRenderer);
      pGame->pItems[NrOfItems] = createItem(pGame->pItemImage[NrOfItems],WINDOW_WIDTH,WINDOW_HEIGHT, 0, pGame->iData.xcoords, pGame->iData.ycoords); 
      NrOfItems++;
    }
  }

  return NrOfItems;

}

void send_itemData(Game *pGame, int spawn) {

  pGame->iData.spawn = spawn;
  pGame->iData.xcoords = (rand() % 900 - 205 + 1) + 205;
  pGame->iData.ycoords = rand() % WINDOW_HEIGHT;

  for (int i = 0; i < MAX_SNKES; i++) {

    memcpy(pGame->pPacket->data, &(pGame->iData), sizeof(ItemData));
    pGame->pPacket->len = sizeof(ItemData);
    pGame->pPacket->address = pGame->clients[i];

    SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);

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

/* Checks nrOfCollisions, if 1 snake alive sets collided to 1 */
void collision_counter(Game *pGame) {

  int nrOfCollisions = 0;

  for (int i = 0; i < MAX_SNKES; i++) {
    if (pGame->pSnke[i]->snakeCollided == 1) nrOfCollisions++;
  }

  if (nrOfCollisions >= MAX_SNKES - 1) {
    pGame->collided = 1;
    for (int i = 0; i < MAX_SNKES; i++) {
      if (!pGame->pSnke[i]->snakeCollided) {
        pGame->sData.died[i] += pGame->deathCount;
      }
    }
  }

}

/* Checks nrOfCollisions, if 1 snake alive sets collided to 1 */
void reset_game(Game *pGame) {
  
  for (int i = 0; i < MAX_SNKES; i++) {
    reset_snake(pGame->pSnke[i], i);
    pGame->ctrlDeath[i] = 0;
  }

  pGame->collided = 0;
  pGame->roundCount++;
  pGame->deathCount = 1;

  if(pGame->roundCount == MAX_ROUNDS){
    pGame->state = MENU;
    pGame->created = 0;
    pGame->deathCount = 1;
    pGame->roundCount = 0;
    pGame->connected_Clients = 0;
    pGame->sData.maxClients = MAX_SNKES;
    for (int i = 0; i < MAX_SNKES; i++) {
      pGame->ctrlDeath[i] = 0;
      pGame->sData.died[i] = 0;
    }
    pGame->showMess = 0;
    pGame->firstStart = 0;
    pGame->countdownCompleted = 0;
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

  for (int i = 0; i < MAX_ITEMS; i++) {
    if (pGame->pItems[i]) destroyItem(pGame->pItems[i]);
    if (pGame->pItemImage[i]) destroyItemImage(pGame->pItemImage[i]);
  }

  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);
  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);

  // Network
  if (pGame->pPacket) SDLNet_FreePacket(pGame->pPacket);
	if (pGame->pSocket) SDLNet_UDP_Close(pGame->pSocket);

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}