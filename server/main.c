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
  int num_of_snkes;
  int collided;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress clients[MAX_SNKES];
  ServerData sData;
  int num_of_clients;

  //ITEM
  ItemImage *pItemImage[MAX_ITEMS];
  Item *pItems[MAX_ITEMS];
  ItemData iData;
  int numItems;

  //TIMER
  int startTime;

  Mix_Music *menuSong, *playSong;

  GameState state;

} Game;

int init_structure(Game *pGame);
int init_allSnakes(Game *pGame);
int init_Items(Game *pGame);
int spawnItem(Game *pGame, int NrOfItems);

void run(Game *pGame);
void close(Game *pGame);
void set_up_game(Game *pGame);
void render_snake(Game *pGame);
void send_gameData(Game *pGame);
void send_itemData(Game *pGame, int spawn);
void execute_command(Game *pGame, ClientData cData);
void add_client(IPaddress address, IPaddress clients[], int *pNumOfClients);
void reset_game(Game *pGame);
void collision_counter(Game *pGame);

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
  pGame->state = START;
  pGame->num_of_clients = 0;
  pGame->num_of_snkes = MAX_SNKES;
  pGame->sData.maxConnPlayers = MAX_SNKES;
  pGame->numItems = MAX_ITEMS;

  if ( !init_sdl_libraries() ) return 0; 

  pGame->pWindow = server_wind("Server", pGame->pWindow);
  if ( !pGame->pWindow) close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if ( !pGame->pRenderer) close(pGame);

  //Initializes audios/sounds & checks for error
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
  pGame->menuSong = Mix_LoadMUS("../lib/resources/main_menu.mp3"); // ta bort
  pGame->playSong = Mix_LoadMUS("../lib/resources/play_game10.mp3");
  if(!pGame->menuSong || !pGame->playSong)
  {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  // Create all snakes
  init_allSnakes(pGame);

  // Establish server to client 
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

  return 1;

}

/* Main loop of the game. Updates the snakes attributes. Looking for input. Sends and recives new data. */
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;
  int showMess = 0;
  int boostKey[MAX_SNKES];
  int nrOfItems = 1;
  int replace;
  int items = 0;

  int closeRequest = 0;
  while(!closeRequest) {

    switch (pGame->state) {
      // The game is running
      case RUNNING:
        //Mix_PlayMusic(pGame->playSong, 0);
        // Show message
        if (showMess) {
          printf("Game state: Running\n");
          showMess--;
        }
        
        send_gameData(pGame);
        if(items==0)
        {
          init_Items(pGame);
          items++;
        }
        
        // Update new recived data to client data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {
          memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));
          execute_command(pGame, cData);
        }

        // All player disconnected close window
        if (pGame->sData.maxConnPlayers == 0) closeRequest = 1;

        // Exit the program
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) closeRequest = 1;
        }

        nrOfItems = spawnItem(pGame, nrOfItems);

        // Create an array of pointers to other snakes
        for(int i = 0; i < MAX_SNKES; i++) {

          Snake *otherSnakes[MAX_SNKES - 1];
          int otherSnakesIndex = 0;

          // Looping through all the other snakes to add them to the array
          for (int j = 0; j < MAX_SNKES; j++) {
            if (j != i) otherSnakes[otherSnakesIndex++] = pGame->pSnke[j];
          }

          for(int j=0;j<MAX_ITEMS;j++) 
          {
            if(collideSnake(pGame->pSnke[i],getRectItem(pGame->pItems[j]))) {
              boostKey[i] = 1;
              pGame->startTime = 0;
              updateItem(pGame->pItems[j]);
              nrOfItems--;
              replace = j;
            }
            if(boostKey>0) {
              pGame->startTime++;
              if(pGame->startTime==100) {
                for (int j = 0; j < MAX_SNKES; j++) 
                {
                  boostKey[j]=0;
                }
              }
            }
          }
          // Update snake cord
          update_snake(pGame->pSnke[i], otherSnakes, MAX_SNKES - 1, boostKey[i]);
        }

        // Render snake
        render_snake(pGame);

        //Check if one Snake left, if so reset game and display winner (filip)
        //collision_counter(pGame);
        if (pGame->collided==1) reset_game(pGame);
      break;
      // Waiting for all clients
      case START:
        // Show message
        if (!showMess) {
          printf("Game state: Start\n");
          showMess++;
        }

        // Exit the program (ctrl + c) 
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) closeRequest = 1;

        // If new data recived add client
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket) == 1) {

          memcpy(&cData, pGame->pPacket->data, sizeof(ClientData));
          printf("Client (%s) has joined...\n", cData.playerName);

          add_client(pGame->pPacket->address, pGame->clients, &(pGame->num_of_clients));

          if (pGame->num_of_clients == MAX_SNKES) set_up_game(pGame);

        }
      break;
    }

    //SDL_Delay(ONE_MS); Kanske kan användas senare. Låt stå.

  }

}

int init_Items(Game *pGame) {
   // creates Items and creates their image
   SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);

  for (int i = 0; i < MAX_ITEMS; i++)
  {
    send_itemData(pGame, 0);
    pGame->pItemImage[i] = createItemImage(pGame->pRenderer);
    pGame->pItems[i] = createItem(pGame->pItemImage[i], WINDOW_WIDTH, WINDOW_HEIGHT, 0, pGame->iData.xcoords, pGame->iData.ycoords);
  }

  for (int i = 0; i < MAX_ITEMS; i++)
  {
    if (!pGame->pItemImage[i] || !pGame->pItems[i])
    {
      printf("Error: %s", SDL_GetError());
      close(pGame);
      return 0;
    }
    }
}

int spawnItem(Game *pGame, int NrOfItems)
{
  int spawn = rand() % 500;
    if(spawn == 0)
    {
      if(NrOfItems!=MAX_ITEMS)
      {
        send_itemData(pGame, 1);
        pGame->pItemImage[NrOfItems] = createItemImage(pGame->pRenderer);
        pGame->pItems[NrOfItems] = createItem(pGame->pItemImage[NrOfItems],WINDOW_WIDTH,WINDOW_HEIGHT, 0, pGame->iData.xcoords, pGame->iData.ycoords); 
        NrOfItems++;
      }
    }
    return NrOfItems;
}

/* Setting up the game with default values and other attributes */
void set_up_game(Game *pGame) {

  printf("Setting up the game...\n");

  pGame->state = RUNNING;
  pGame->num_of_snkes = MAX_SNKES;

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
      memcpy(pGame->pPacket->data, &(pGame->sData), sizeof(ServerData));
		  pGame->pPacket->len = sizeof(ServerData);
      pGame->pPacket->address = pGame->clients[i];
      
		  if ( !SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket) ) {
        printf("Error (UDP_Send): %s", SDLNet_GetError());
      }

    }

}

void send_itemData(Game *pGame, int spawn)
{
  pGame->iData.spawn = spawn;
  pGame->iData.xcoords = rand()%850;
  pGame->iData.ycoords = rand()%510;

  for (int i = 0; i < MAX_SNKES; i++) {
  memcpy(pGame->pPacket->data, &(pGame->iData), sizeof(ItemData));
  pGame->pPacket->len = sizeof(ItemData);
  pGame->pPacket->address = pGame->clients[i];

  if(!SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket)) 
  {
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
        pGame->sData.maxConnPlayers -= 1;
      break;
    }

}

/** 
 * Create all snakes (players) in an array.
 * Checking for errors.
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

//Checks nrOfCollisions, if 1 snake alive sets collided to 1 (filip)
void collision_counter(Game *pGame) {
  int nrOfCollisions = 0;
  for (int i = 0; i < MAX_SNKES; i++) {
    if (pGame->pSnke[i]->snakeCollided == 1) nrOfCollisions++;
  }
  if (nrOfCollisions==MAX_SNKES-(MAX_SNKES-1)) pGame->collided=1;
}

//sets the game state to START and resets to default values (filip)
void reset_game(Game *pGame) {
  
  for (int i = 0; i < MAX_SNKES; i++){
    reset_snake(pGame->pSnke[i]);
  }
  pGame->collided = 0;
  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);
  pGame->state = START;
  pGame->num_of_clients=0;
}

/* Destoryes various SDL libraries and snakes. Safe way when exiting game. */
void close(Game *pGame) {

  for(int i=0; i < MAX_SNKES; i++) 
    if(pGame->pSnke[i]) destroy_snake(pGame->pSnke[i]);

  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);

  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);

  if (pGame->pPacket) SDLNet_FreePacket(pGame->pPacket);

	if (pGame->pSocket) SDLNet_UDP_Close(pGame->pSocket);

  Mix_FreeMusic(pGame->menuSong); 
  Mix_FreeMusic(pGame->playSong);
  Mix_CloseAudio();

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}