#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include "../lib/include/data.h"
#include "../lib/include/text.h"
#include "../lib/include/snake.h"
#include "../lib/include/init.h"
#include "../lib/include/item.h"

/* Client Game struct */
typedef struct game {

  // SNAKE
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke[MAX_SNKES];
  int num_of_snkes;
  int snkeID;
  int collided;

  // SOUND & AUDIO
  Mix_Music *menuSong, *playSong;

  // ITEM
  ItemImage *pItemImage[MAX_ITEMS];
  Item *pItems[MAX_ITEMS];

  // TIMER
  int startTime;

  // UI
  TTF_Font *pStrdFont, *pTitleBigFont, *pTitleSmallFont, *pNetFont;
  Text *pInGameTitle, *pTitleBigText, *pTitleSmallText, *pStartText, *pStartDark, *pWaitingText, *pQuitText, *pQuitDark, *pListPlayers;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress serverAdd;
  char ipAddr[INPUT_BUFFER_SIZE];
  char playerName[INPUT_BUFFER_SIZE];

  GameState state;

} Game;

// Temporary struct for testing leaderboard
typedef struct {
  char playerName[20];
  int playerScore;
} Player;
//

int init_Items(Game *pGame);
int conn_server(Game *pGame);
int text_getError(Game *pGame);
int init_structure(Game *pGame);
int init_allSnakes(Game *pGame);
int input_text_handler(Game *pGame);
int disconnect_fromGame(Game *pGame);
int spawnItem(Game *pGame, int nrOfItems);
void draw_interface(Game* pGame);

void run(Game *pGame);
void close(Game *pGame);
void render_snake(Game *pGame);
void render_background(Game *pGame);
void update_server_data(Game *pGame);
void input_handler(Game *pGame, SDL_Event *pEvent);
void collision_counter(Game *pGame);
void reset_game(Game *pGame);

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

  pGame->state = START;
  pGame->num_of_snkes = MAX_SNKES;

  if ( !init_sdl_libraries() ) return 0;

  pGame->pWindow = client_wind("Trail Clash", pGame->pWindow);
  if ( !pGame->pWindow ) close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if ( !pGame->pRenderer) close(pGame);

  // Create own font with create_font function. Value is stored in a Text pointer.
  pGame->pTitleBigFont = create_font(pGame->pTitleBigFont, "../lib/resources/ATW.ttf", 120);
  if ( !pGame->pTitleBigFont ) close(pGame);

  pGame->pTitleSmallFont = create_font(pGame->pTitleSmallFont, "../lib/resources/ATW.ttf", 100);
  if ( !pGame->pTitleSmallFont ) close(pGame);

  pGame->pStrdFont = create_font(pGame->pStrdFont, "../lib/resources/ATW.ttf", 25);
  if ( !pGame->pStrdFont ) close(pGame);

  pGame->pNetFont = create_font(pGame->pNetFont, "../lib/resources/ATW.ttf", 20);
  if ( !pGame->pNetFont ) close(pGame);

  // Create own text with create_text function. Value is stored in a Text pointer.
  pGame->pTitleSmallText = create_text(pGame->pRenderer, 38, 175, 255, pGame->pTitleSmallFont,
    "Trail", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 160);

  pGame->pTitleBigText = create_text(pGame->pRenderer, 255, 110, 51, pGame->pTitleBigFont,
    "Clash", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 70);

  pGame->pStartText = create_text(pGame->pRenderer, 38, 175, 255, pGame->pStrdFont,
    "START GAME", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 50);

  pGame->pStartDark = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "START GAME", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 50);

  pGame->pQuitText = create_text(pGame->pRenderer, 38, 175, 255, pGame->pStrdFont,
    "QUIT GAME", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  pGame->pQuitDark = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "QUIT GAME", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  pGame->pWaitingText = create_text(pGame->pRenderer, 238, 168, 65,pGame->pStrdFont,
    "Waiting to start ...", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  // Checking if there is an error regarding all the Text pointer.
  text_getError(pGame);

  //Initializes audios/sounds & checks for error
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
  pGame->menuSong = Mix_LoadMUS("../lib/resources/main_menu.mp3"); // ta bort
  pGame->playSong = Mix_LoadMUS("../lib/resources/play_game10.mp3");
  if(!pGame->menuSong || !pGame->playSong) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  init_Items(pGame); // seems to be working now?

  // Create all snakes
  init_allSnakes(pGame);

  return 1;

}

/* Main loop of the game. Updates the snakes attributes. Looking for input. Sends and recives new data. */
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;
  int joining = 0;
  int send = 0;
  int text_index = 0;
  int boostKey = 0;
  int nrOfItems = 0;
  int replace;

  int closeRequest = 0;
  while(!closeRequest) {

    switch (pGame->state) {
      // The game is running
      case RUNNING:
        //Mix_PlayMusic(pGame->playSong, 0);
        // Update new recived data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
          update_server_data(pGame);
        }

        // Looking if there is an input
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) {
            closeRequest = 1;
          }
          else input_handler(pGame, &event);
        }

        nrOfItems = spawnItem(pGame, nrOfItems);

        for (int i = 0; i < MAX_ITEMS; i++) {
          if(collideSnake(pGame->pSnke[i],getRectItem(pGame->pItems[i]))) {
              /*boostKey = 1;
              pGame->startTime = 0;*/
              updateItem(pGame->pItems[i]);
              nrOfItems--;
              replace = i;
            }
            /*if(boostKey>0) {
              pGame->startTime++;
              if(pGame->startTime==200) {
                boostKey=0;
              }
            }*/
        }

        // Create an array of pointers to other snakes
        for(int i = 0; i < MAX_SNKES; i++) {

          Snake *otherSnakes[MAX_SNKES - 1];
          int otherSnakesIndex = 0;

          // Looping through all the other snakes to add them to the array
          for (int j = 0; j < MAX_SNKES; j++) {
            if (j != i) otherSnakes[otherSnakesIndex++] = pGame->pSnke[j];
          }
  
          // Update snake cord, send data
          update_snake(pGame->pSnke[i], otherSnakes, MAX_SNKES - 1, boostKey);

        }
        
        // Render snake
        render_snake(pGame);

        //Check if one Snake left, if so reset game and display winner (filip)
        //collision_counter(pGame);
        if (pGame->collided==1) reset_game(pGame);
      break;
      // Main Menu
      case START:
        //Mix_PlayMusic(pGame->menuSong, 0);
        // Not connected to server
        if (!joining) {
          
          // Render content
          render_background(pGame);
          draw_text(pGame->pTitleSmallText);
          draw_text(pGame->pTitleBigText);

          // Which content to highlight
          if (text_index == 0) {
            draw_text(pGame->pStartText);
            draw_text(pGame->pQuitDark);
          } else if (text_index == 1) {
            draw_text(pGame->pStartDark);
            draw_text(pGame->pQuitText);
          }

        } else {
          SDL_SetRenderDrawColor(pGame->pRenderer,0,0,0,255);
          SDL_RenderClear(pGame->pRenderer);
          SDL_SetRenderDrawColor(pGame->pRenderer,230,230,230,255);
          draw_text(pGame->pWaitingText);
        }

        // Update screen
        SDL_RenderPresent(pGame->pRenderer);

        // Input
        if (SDL_PollEvent(&event)) {

          // Close window
          if (event.type == SDL_QUIT) closeRequest = 1;

          if (!joining && event.type == SDL_KEYDOWN) {
            // Arrow Up
            if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
              text_index -= 1;
              if (text_index < 0) text_index = 0;
            }

            // Arrow Down
            if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
              text_index += 1;
              if (text_index > 1) text_index = 1;
            }

            // Select index
            if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
              // START GAME
              if (text_index == 0) {

                input_text_handler(pGame); // Enter playerName and ipAddr
                conn_server(pGame);    // Connect to server with ipAddr

                // Store ClientData (cData) to packet
                send = 1;
                joining = 1;
                cData.command = READY;
                cData.snkeNumber = -1;
                strcpy(cData.playerName, pGame->playerName);
                memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
	              pGame->pPacket->len = sizeof(ClientData);
              
              }

              // QUIT
              if (text_index == 1) {
                closeRequest = 1;
              }

            }

          }

        }

        // Send data
        if (!closeRequest && send) {
          if ( !SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket) ) {
            printf("Error (UDP_Send): %s", SDLNet_GetError());
          }
          send = 0;
        }

        // Update new recived data
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
          printf("Got new data...\n");
          update_server_data(pGame);
          if (pGame->state == RUNNING) joining = 0;
        }
      break;

    }

    //SDL_Delay(ONE_MS); Kanske kan användas senare. Låt stå.

  }

  disconnect_fromGame(pGame);

}

int init_Items(Game *pGame) {
  // creates Items and creates their image

  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer, 230, 230, 230, 255);

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

/* Manage various inputs (keypress, mouse) */
void input_handler(Game *pGame, SDL_Event *pEvent) {

  if (pEvent->type == SDL_KEYDOWN) {

    ClientData cData;
    cData.snkeNumber = pGame->snkeID;

    switch (pEvent->key.keysym.scancode) {
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

/* 
*  Text handler to be able to type input to window with SDL_StartTextInput()
*  It returns two values, ipAddr and playerName, to struct Game (pGame)
*/
int input_text_handler(Game *pGame) {

  // Enable text input handling
  SDL_StartTextInput();

  SDL_Event event;
  ClientData cData;

  // Input mananger
  int input_index = 0;

  char messAddress[30] = "Enter a server IP address";
  char messPlayerName[30] = "Enter player name";

  char clientName[INPUT_BUFFER_SIZE] = "";
  int clientName_pos = 0;

  char ipAddr[INPUT_BUFFER_SIZE] = "";
  int ipAddr_pos = 0;

  int closeRequest = 0;
  while (!closeRequest) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          closeRequest = 1;
        break;
        case SDL_TEXTINPUT:
          // Enter Player Name
          if (input_index == 0) {
            if (clientName_pos < INPUT_BUFFER_SIZE) {
              strcat(clientName, event.text.text);
              clientName_pos += strlen(event.text.text);
            }
          }

          // Enter Ip Address
          if (input_index == 1) {
            if (ipAddr_pos < 15) {
              strcat(ipAddr, event.text.text);
              ipAddr_pos += strlen(event.text.text);
            }
          }
        break;
        case SDL_KEYDOWN:
          // Confirm input
          if (event.key.keysym.sym == SDLK_RETURN) closeRequest = 1;

          // Change between clientName (0) and ipAddr (1)
          if (event.key.keysym.sym == SDLK_UP) {
            input_index -= 1;
            if (input_index < 0) input_index = 0;
          }

          if (event.key.keysym.sym == SDLK_DOWN) {
            input_index += 1;
            if (input_index > 1) input_index = 1;
          }

          // Erase input depening on which input_index
          if (input_index == 0) {
            if (event.key.keysym.sym == SDLK_BACKSPACE && clientName_pos > 0) {
              clientName[clientName_pos - 1] = '\0';
              clientName_pos--;
            }
          }

          if (input_index == 1) {
            if (event.key.keysym.sym == SDLK_BACKSPACE && ipAddr_pos > 0) {
              ipAddr[ipAddr_pos - 1] = '\0';
              ipAddr_pos--;
            }
          }
        break;
      }
    }

    for (int i = 0, j = 0; i < 2; i++) {
      if (clientName[i] != ' ') {
        clientName[j] = clientName[i];
        j++;
      }
    }

    for (int i = 0, j = 0; i < 2; i++) {
      if (ipAddr[i] != ' ') {
        ipAddr[j] = ipAddr[i];
        j++;
      }
    }

    // Render background
    render_background(pGame);

    // Rect input field (ipAddr/clientName)
    SDL_Rect ipAddr_rect = { 
      WINDOW_WIDTH / 2 - 150,   // Rectangle x cord
      WINDOW_HEIGHT / 2 + 50,   // Rectangle y cord
      300,                      // Width of the rectangle
      50                        // Height of the rectangle
    };

    SDL_Rect clientName_rect = { 
      WINDOW_WIDTH / 2 - 150,   // Rectangle x cord
      WINDOW_HEIGHT / 2 - 120,  // Rectangle y cord
      300,                      // Width of the rectangle
      50                        // Height of the rectangle
    };

    // Rect message field (ipAddr/clientName)
    SDL_Rect messIpAddr_rect = { 
      300,                      // Rectangle x cord
      250,                      // Rectangle y cord
      300,                      // Width of the rectangle
      70                        // Height of the rectangle
    };

    SDL_Rect messClientName_rect = { 
      300,                      // Rectangle x cord
      75,                       // Rectangle y cord
      300,                      // Width of the rectangle
      80                        // Height of the rectangle
    };

    SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(pGame->pRenderer, &ipAddr_rect);
    SDL_RenderDrawRect(pGame->pRenderer, &clientName_rect);
        
    // Input rect (ipAddr/clientName)
    SDL_Rect input_ipAddr_rect = { 
      ipAddr_rect.x + 15,
      ipAddr_rect.y + 15,
      ipAddr_rect.w - 30,
      ipAddr_rect.h - 30
    };

    SDL_Rect input_clientName_rect = { 
      clientName_rect.x + 15,
      clientName_rect.y + 15,
      clientName_rect.w - 30,
      clientName_rect.h - 30
    };

    // Colors (light, dark)
    SDL_Color text_color = { 
      255, 255, 255, 255 
    };

    SDL_Color dark_text_color = { 
      153, 153, 153, 255 
    };

    // Highlight the chosen input_index
    if (input_index == 0) {
      SDL_Surface* client_surface = TTF_RenderText_Solid(pGame->pNetFont, clientName, text_color);
      SDL_Surface* message_clientName_surface = TTF_RenderText_Solid(pGame->pNetFont, messPlayerName, text_color);
      SDL_Texture* client_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, client_surface);
      SDL_Texture* message_clientName_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, message_clientName_surface);
      SDL_Surface* ipAddr_surface = TTF_RenderText_Solid(pGame->pNetFont, ipAddr, dark_text_color);
      SDL_Surface* message_ipAddr_surface = TTF_RenderText_Solid(pGame->pNetFont, messAddress, dark_text_color);
      SDL_Texture* ipAddr_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, ipAddr_surface);
      SDL_Texture* message_ipAddr_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, message_ipAddr_surface);
      SDL_RenderCopy(pGame->pRenderer, client_texture, NULL, &input_clientName_rect);
      SDL_RenderCopy(pGame->pRenderer, message_clientName_texture, NULL, &messClientName_rect);
      SDL_RenderCopy(pGame->pRenderer, ipAddr_texture, NULL, &input_ipAddr_rect);
      SDL_RenderCopy(pGame->pRenderer, message_ipAddr_texture, NULL, &messIpAddr_rect);
    }

    if (input_index == 1) {
      SDL_Surface* client_surface = TTF_RenderText_Solid(pGame->pNetFont, clientName, dark_text_color);
      SDL_Surface* message_clientName_surface = TTF_RenderText_Solid(pGame->pNetFont, messPlayerName, dark_text_color);
      SDL_Texture* client_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, client_surface);
      SDL_Texture* message_clientName_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, message_clientName_surface);
      SDL_Surface* ipAddr_surface = TTF_RenderText_Solid(pGame->pNetFont, ipAddr, text_color);
      SDL_Surface* message_ipAddr_surface = TTF_RenderText_Solid(pGame->pNetFont, messAddress, text_color);
      SDL_Texture* ipAddr_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, ipAddr_surface);
      SDL_Texture* message_ipAddr_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, message_ipAddr_surface);
      SDL_RenderCopy(pGame->pRenderer, client_texture, NULL, &input_clientName_rect);
      SDL_RenderCopy(pGame->pRenderer, message_clientName_texture, NULL, &messClientName_rect);
      SDL_RenderCopy(pGame->pRenderer, ipAddr_texture, NULL, &input_ipAddr_rect);
      SDL_RenderCopy(pGame->pRenderer, message_ipAddr_texture, NULL, &messIpAddr_rect);
    }

    SDL_RenderPresent(pGame->pRenderer);

  }

  // Disable text input handling, clear screen
  SDL_StopTextInput();
  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);

  // Copy string to pGame for future use
  strcpy(pGame->ipAddr, ipAddr);
  strcpy(pGame->playerName, clientName);

  return 1;

}

/**
*  Establish a client to server connection.
*  Text prompt to enter IP address.
*  \param pSocket Open a UDP network socket.
*  \param pPacket Allocate/resize/free a single UDP packet.
*/
int conn_server(Game *pGame) {

  if ( !(pGame->pSocket = SDLNet_UDP_Open(0)) ) {
    printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
    return 0;
  }

  if (SDLNet_ResolveHost(&(pGame->serverAdd), pGame->ipAddr, 2000)) {
    printf("SDLNet_ResolveHost (%s: 2000): %s\n", pGame->ipAddr, SDLNet_GetError());
    return 0;
  }

  if ( !(pGame->pPacket = SDLNet_AllocPacket(512)) ) {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    return 0;
  }

  pGame->pPacket->address.host = pGame->serverAdd.host;
  pGame->pPacket->address.port = pGame->serverAdd.port;

}

/* When player has closed window or left the game */
int disconnect_fromGame(Game *pGame) {
  
  ClientData cData;

  cData.command = DISC;
  strcpy(cData.playerName, pGame->playerName);
  memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
	pGame->pPacket->len = sizeof(ClientData);

  if ( !SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket) ) {
    printf("Error (UDP_Send): %s", SDLNet_GetError());
  }

}

/* Copy new data to struct (ServerData) and update snake */
void update_server_data(Game *pGame) {

    ServerData srvData;

    memcpy(&srvData, pGame->pPacket->data, sizeof(ServerData));
    pGame->snkeID = srvData.snkeNum;
    pGame->state = srvData.gState;

    for (int i = 0; i < MAX_SNKES; i++)
      update_recived_snake_data(pGame->pSnke[i], &(srvData.snakes[i]));

}

/* 
*  Create all snakes (players) in the array.
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

void draw_interface(Game* pGame) {

  // Temporary assigned names and score (test)
  Player players[4] = {
    {"Player 1", 7},
    {"Player 2", 10},
    {"Player 3", 3},
    {"Player 4", 13}
  };

  float xCord = pGame->pSnke[0]->xCord;

  // Sorting the player with highest score first
  int i, j;
  Player temp;
  for (i = 0; i < 4 - 1; i++) {
    for (j = 0; j < 4 - i - 1; j++) {
      if (players[j].playerScore < players[j+1].playerScore) {
        temp = players[j];
        players[j] = players[j+1];
        players[j+1] = temp;
      }
    }
  }

  // Render the playing field
  SDL_Rect input_walls = {
    WINDOW_WIDTH * 0.25,    // Rectangle x cord
    WINDOW_HEIGHT * 0.02,   // Rectangle y cord
    WINDOW_WIDTH * 0.74,    // Width of the rectangle
    WINDOW_HEIGHT * 0.95    // Width of the rectangle
  };

  SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
  SDL_RenderFillRect(pGame->pRenderer, &input_walls);

  SDL_Rect input_rect = {
    WINDOW_WIDTH * 0.255,   // Rectangle x cord
    WINDOW_HEIGHT * 0.03,   // Rectangle y cord
    WINDOW_WIDTH * 0.73,    // Width of the rectangle
    WINDOW_HEIGHT * 0.935   // Width of the rectangle
  };

  SDL_SetRenderDrawColor(pGame->pRenderer, 40, 40, 40, 255);
  SDL_RenderFillRect(pGame->pRenderer, &input_rect);


  // Render the leaderboard
  for(int i=0;i<4;i++){
    SDL_Color White = {255, 255, 255};
    char scoreStr[10];
    sprintf(scoreStr, "%d", players[i].playerScore);

    SDL_Surface* surfaceNumber =
    TTF_RenderText_Solid(pGame->pStrdFont, scoreStr, White);

    SDL_Texture* Number = SDL_CreateTextureFromSurface(pGame->pRenderer, surfaceNumber);
    
    SDL_Rect Number_rect; 
    Number_rect.x = 250;  
    Number_rect.y = 40 + 60 * i; 
    Number_rect.w = 40; 
    Number_rect.h = 50; 

    SDL_RenderCopy(pGame->pRenderer, Number, NULL, &Number_rect);

    SDL_Surface* surfaceMessage =
    TTF_RenderText_Solid(pGame->pStrdFont, players[i].playerName, White);

    SDL_Texture* Message = SDL_CreateTextureFromSurface(pGame->pRenderer, surfaceMessage);

    SDL_Rect Message_rect;
    Message_rect.x = 100;  
    Message_rect.y = 40 + 60 * i; 
    Message_rect.w = 130; 
    Message_rect.h = 50; 

    SDL_RenderCopy(pGame->pRenderer, Message, NULL, &Message_rect);


    SDL_FreeSurface(surfaceNumber);
    SDL_DestroyTexture(Number);
    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(Message);
  }
  draw_text(pGame->pInGameTitle);
}


/* Render a snake (player) to the window */
void render_snake(Game *pGame) {

  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);       // Black
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer, 230, 230, 230, 255); // White-ish

  for (int i = 0; i < MAX_ITEMS; i++)
    drawItem(pGame->pItems[i]);

  for (int i = 0; i < MAX_SNKES; i++) {
    // draw_interface(pGame);
    draw_snake(pGame->pSnke[i]);
    draw_trail(pGame->pSnke[i]);
  }

  SDL_RenderPresent(pGame->pRenderer);

}

int spawnItem(Game *pGame, int NrOfItems) {
  int spawn = rand() % 50;
  if (spawn == 0) {
    if (NrOfItems == MAX_ITEMS) {
    }
    else {
      pGame->pItemImage[NrOfItems] = createItemImage(pGame->pRenderer);
      pGame->pItems[NrOfItems] = createItem(pGame->pItemImage[NrOfItems], WINDOW_WIDTH, WINDOW_HEIGHT, 0, 500, 500);
      NrOfItems++;
    }
  }
  return NrOfItems;
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
  
}

/* Render a background for the game */
void render_background(Game *pGame) {

  SDL_SetRenderDrawColor(pGame->pRenderer, 9, 66, 100, 255); // Dark Blue
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer, 7, 52, 80, 255);  // Darker color then the previous

  for (int i = 0; i <= WINDOW_WIDTH; i += 10) {
    for(int j = 0; j <= WINDOW_HEIGHT; j += 10) {
      SDL_Rect pointRect = { i - POINT_SIZE / 2, j - POINT_SIZE / 2, POINT_SIZE, POINT_SIZE };
      SDL_RenderFillRect(pGame->pRenderer, &pointRect);      // Creates a small rect
    }
  }

}

/* Check error with every text pointer and close game if error is true */
int text_getError(Game *pGame) {

  if (!pGame->pStartText || !pGame->pStartDark || !pGame->pWaitingText || !pGame->pTitleBigText || !pGame->pTitleSmallText || !pGame->pQuitText || !pGame->pQuitDark) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

}

/* Destoryes various SDL libraries and snakes. Safe way when exiting game. */
void close(Game *pGame) {

  for (int i = 0; i < MAX_SNKES; i++) 
    if (pGame->pSnke[i]) destroy_snake(pGame->pSnke[i]);

  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);

  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);

  if(pGame->pWaitingText) destroy_text(pGame->pWaitingText);

  if(pGame->pTitleBigText) destroy_text(pGame->pTitleBigText); 

  if(pGame->pTitleSmallText) destroy_text(pGame->pTitleSmallText); 

  if(pGame->pInGameTitle) destroy_text(pGame->pInGameTitle);

  if(pGame->pStartText) destroy_text(pGame->pStartText);  

  if(pGame->pStartDark) destroy_text(pGame->pStartDark);

  if(pGame->pQuitDark) destroy_text(pGame->pQuitDark);
  
  if(pGame->pQuitText) destroy_text(pGame->pQuitText);

  if(pGame->pTitleBigFont) TTF_CloseFont(pGame->pTitleBigFont);

  if(pGame->pTitleSmallFont) TTF_CloseFont(pGame->pTitleSmallFont);

  if(pGame->pStrdFont) TTF_CloseFont(pGame->pStrdFont);

  if(pGame->pNetFont) TTF_CloseFont(pGame->pNetFont);

  Mix_FreeMusic(pGame->menuSong); 
  Mix_FreeMusic(pGame->playSong);
  Mix_CloseAudio();

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}