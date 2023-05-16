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

  char ipAddr[INPUT_BUFFER_SIZE];
  char myName[INPUT_BUFFER_SIZE];
  char SnakeNames[MAX_SNKES][INPUT_BUFFER_SIZE];

  int curentClients;
  int roundCount;
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;

  SDL_Texture *pSpaceTexture, *pEnterTexture, *pEscTexture;
  SDL_Surface *pSpaceSurface, *pEnterSurface, *pEscSurface;

  // SNAKE
  Snake *pSnke[MAX_SNKES];
  int collided;
  int snkeID;

  // SOUND & AUDIO
  Mix_Music *pClickSound, *pSelectSound, *pMusic;

  // ITEM
  ItemImage *pItemImage[MAX_ITEMS];
  Item *pItems[MAX_ITEMS];

  // UI
  TTF_Font *pStrdFont, *pTitleBigFont, *pTitleSmallFont, *pNameFont, *pNumberFont;
  Text *pInGameTitle, *pTitleBigText, *pTitleSmallText, *pStartText, *pStartDark, *pQuitText, *pQuitDark, 
  *pListPlayers, *pSelectText, *pContinueText, *pBackText, *pWaitingText, *pMessIP, *pMessName, *pSnkeName, *pBoardText,
  *pBoardInfo, *pNumThree, *pNumTwo, *pNumOne;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress serverAdd;
  int send_data;

  GameState state;
  GameScene scene;

} Game;

int init_structure(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int init_Music(Game *pGame);
int init_Items(Game *pGame);
int init_image(Game *pGame);
int create_allFonts(Game *pGame);
int create_allText(Game *pGame);
int init_allSnakes(Game *pGame);
int build_handler(Game *pGame);
void input_handler(Game *pGame, SDL_Event *pEvent);
void render_game(Game *pGame);
void render_background(Game *pGame);
int render_scene(Game *pGame, GameScene Scene, int index);
void looby(Game *pGame);
void highlight_text(Game *pGame, int index);
void disconnect(Game *pGame);
int conn_server(Game *pGame);
void update_ServerData(Game *pGame);
void reset_game(Game *pGame);
void draw_interface(Game* pGame);
void collision_counter(Game *pGame);
int spawnItem(Game *pGame, int numOfItems);
void clientReady(Game *pGame);
void draw_snakeName(Game *pGame, int i);
void update_PlayerInfo(Game *pGame);
void create_snakePointers(Game *pGame, int boostkey);
void interacteItem(Game *pGame, int numOfItems, int replace);
void countDown(Game *pGame);

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

  // Default values
  pGame->state = START;
  pGame->scene = MENU_SCENE;
  pGame->curentClients = 0;
  pGame->send_data = 0;
  pGame->roundCount = 0;

  if ( !init_sdl_libraries() ) return 0;

  if ( !init_Music(pGame) ) return 0;

  pGame->pWindow = client_wind("Trail Clash", pGame->pWindow);
  if ( !pGame->pWindow ) close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if ( !pGame->pRenderer) close(pGame);

  if ( !init_image(pGame) ) return 0;

  if ( !init_Items(pGame) ) return 0;

  if ( !init_allSnakes(pGame) ) return 0;

  if ( !create_allFonts(pGame) ) return 0;

  if ( !create_allText(pGame) ) return 0;

  return 1;

}

/* Main loop of the game. Updates the snakes attributes. Looking for input. Sends and recives new data. */
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;

  int replace = 0;
  int boostKey = 0;
  int numOfItems = 0;

  int text_index = 0;
  int recvPlayerInfo = 0;
  int firstStart = 0;

  int closeRequest = 0;
  while(!closeRequest) {
    switch (pGame->state) {
      // The game is running
      case RUNNING:

        // Update new recived data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
          update_ServerData(pGame);
        }

        // Looking if there is an input
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) {
            closeRequest = 1;
            disconnect(pGame);
          }
          else input_handler(pGame, &event);
        }

        numOfItems = spawnItem(pGame, numOfItems);
        interacteItem(pGame, numOfItems, replace);

        create_snakePointers(pGame, boostKey);

        // Recive data of names
        if (!recvPlayerInfo) {
          update_PlayerInfo(pGame);
          recvPlayerInfo++;
        }

        render_game(pGame);

        if (!firstStart) {
          countDown(pGame);
          firstStart++;
        }

        // Side note: This one leads to a segmentation fault
        if (!Mix_PlayingMusic()) {
          Mix_PlayMusic(pGame->pMusic, -1);
        }

        //Check if one Snake left, if so reset game and display winner (filip)
        collision_counter(pGame);
         if (pGame->collided==1) reset_game(pGame);

      break;
      // Main Menu
      case START:

        if ( !render_scene(pGame, pGame->scene, text_index) ) closeRequest = 1;

        // Looking if there is an input
        if (SDL_PollEvent(&event)) {

          if (event.type == SDL_QUIT) closeRequest = 1;

          if (pGame->scene == MENU_SCENE && event.type == SDL_KEYDOWN) {
            // Arrow up, W
            if (event.key.keysym.scancode == SDL_SCANCODE_UP || event.key.keysym.scancode == SDL_SCANCODE_W) {
              Mix_PlayMusic(pGame->pClickSound, 0);
              text_index -= 1;
              if (text_index < 0) text_index = 0;
            }

            // Arrow down, S
            if (event.key.keysym.scancode == SDL_SCANCODE_DOWN || event.key.keysym.scancode == SDL_SCANCODE_S) {
              Mix_PlayMusic(pGame->pClickSound, 0);
              text_index += 1;
              if (text_index > 1) text_index = 1;
            }

            // Select content (Space)
            if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
              Mix_PlayMusic(pGame->pSelectSound, 0);
              // START GAME
              if (text_index == 0) pGame->scene = BUILD_SCENE;

              // QUIT
              if (text_index == 1) closeRequest = 1;
            }

          }

        }

        if (pGame->send_data) {
          pGame->send_data = 0;
          SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);
        }

        // Update new recived data
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
          update_ServerData(pGame);
        }

      break;
    }
  }

}

void countDown(Game *pGame) {

  draw_text(pGame->pNumThree);
  SDL_RenderPresent(pGame->pRenderer);
  Mix_PlayMusic(pGame->pSelectSound, 0);
  SDL_Delay(1000);
  render_game(pGame);
  draw_text(pGame->pNumTwo);
  SDL_RenderPresent(pGame->pRenderer);
  Mix_PlayMusic(pGame->pSelectSound, 0);
  SDL_Delay(1000);
  render_game(pGame);
  draw_text(pGame->pNumOne);
  SDL_RenderPresent(pGame->pRenderer);
  Mix_PlayMusic(pGame->pSelectSound, 0);
  SDL_Delay(1000);

}

/* Create an array of pointers to other snakes */
void create_snakePointers(Game *pGame, int boostKey) {

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

}

/* Look for an interacting between player and item */
void interacteItem(Game *pGame, int numOfItems, int replace) {

  for (int i = 0; i < MAX_ITEMS; i++) {
    if(collideSnake(pGame->pSnke[i], getRectItem(pGame->pItems[i]))) {
      updateItem(pGame->pItems[i]);
      numOfItems--;
      replace = i;
    }
  }

}

/* Show the selected scene */
int render_scene(Game *pGame, GameScene Scene, int index) {

  switch (Scene) {
    case MENU_SCENE:
      // Title, background
      render_background(pGame);
      draw_text(pGame->pTitleSmallText);
      draw_text(pGame->pTitleBigText);
      // Highligt specific content
      highlight_text(pGame, index);
    break;
    case BUILD_SCENE:
      if ( !build_handler(pGame) ) return 0;
    break;
    case LOBBY_SCENE:
      looby(pGame);
    break;
  }

  // Update screen
  SDL_RenderPresent(pGame->pRenderer);

  return 1;

}

/* Render a background for the game */
void render_background(Game *pGame) {

  // Render the background color
  SDL_SetRenderDrawColor(pGame->pRenderer, 9, 66, 100, 255); // Dark Blue
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer, 7, 52, 80, 255);  // Darker color then the previous

  for (int i = 0; i <= WINDOW_WIDTH; i += 10) {
    for(int j = 0; j <= WINDOW_HEIGHT; j += 10) {
      SDL_Rect pointRect = { i - POINT_SIZE / 2, j - POINT_SIZE / 2, POINT_SIZE, POINT_SIZE };
      SDL_RenderFillRect(pGame->pRenderer, &pointRect);      // Creates a small rect
    }
  }

  // Baner
  SDL_Rect baner_rect; 
  baner_rect.x = 0;
  baner_rect.y = WINDOW_HEIGHT / 2 + 200;
  baner_rect.w = WINDOW_WIDTH;
  baner_rect.h = 50;

  // Images on the left side
  SDL_Rect leftImg_rect;
  leftImg_rect.x = WINDOW_WIDTH - 890;
  leftImg_rect.y = WINDOW_HEIGHT - 73;
  leftImg_rect.w = 80;
  leftImg_rect.h = 35;

  // Images on the right side
  SDL_Rect rightImg_rect;
  rightImg_rect.x = WINDOW_WIDTH - 245;
  rightImg_rect.y = WINDOW_HEIGHT - 73;
  rightImg_rect.w = 50;
  rightImg_rect.h = 35;

  // Render the selected content with baner
  if (pGame->scene == MENU_SCENE || pGame->scene == BUILD_SCENE) {
    SDL_SetRenderDrawColor(pGame->pRenderer, 58, 103, 131, 255);
    SDL_RenderFillRect(pGame->pRenderer, &baner_rect);
    SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(pGame->pRenderer, 0, WINDOW_HEIGHT / 2 + 200, WINDOW_WIDTH, WINDOW_HEIGHT / 2 + 200);
    SDL_RenderDrawLine(pGame->pRenderer, 0, WINDOW_HEIGHT / 2 + 250, WINDOW_WIDTH, WINDOW_HEIGHT / 2 + 250);
    if (pGame->scene == MENU_SCENE) {
      SDL_RenderCopy(pGame->pRenderer, pGame->pSpaceTexture, NULL, &leftImg_rect);
      draw_text(pGame->pSelectText);
    }
    if (pGame->scene == BUILD_SCENE) {
      SDL_RenderCopy(pGame->pRenderer, pGame->pEnterTexture, NULL, &leftImg_rect);
      draw_text(pGame->pContinueText);
      SDL_RenderCopy(pGame->pRenderer, pGame->pEscTexture, NULL, &rightImg_rect);
      draw_text(pGame->pBackText);
    }
  }

}

/* Render things for the game to the window */
void render_game(Game *pGame) {

  draw_interface(pGame);

  for (int i = 0; i < MAX_ITEMS; i++)
    drawItem(pGame->pItems[i]);

  for (int i = 0; i < MAX_SNKES; i++) {
    draw_snake(pGame->pSnke[i]);
    draw_trail(pGame->pSnke[i]);
    draw_snakeName(pGame, i);
  }

  SDL_RenderPresent(pGame->pRenderer);

}

/* Display a name on top of the snake object */
void draw_snakeName(Game *pGame, int i) {

  pGame->pSnkeName = create_text(pGame->pRenderer, 255, 255, 255, pGame->pNameFont,
    pGame->SnakeNames[i], pGame->pSnke[i]->xCord + 3, pGame->pSnke[i]->yCord - 10);

  if (!pGame->pSnkeName) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
  }

  draw_text(pGame->pSnkeName);

}

/* Function to highligt specific content */
void highlight_text(Game *pGame, int index) {

  if (index == 0) {
    draw_text(pGame->pStartText);
    draw_text(pGame->pQuitDark);
  } else if (index == 1) {
    draw_text(pGame->pStartDark);
    draw_text(pGame->pQuitText);
  }

}

/* Render scene lobby */
void looby(Game *pGame) {
  render_background(pGame);
  draw_text(pGame->pWaitingText);
}

/* Spawn Item by creating image and creating object */
int spawnItem(Game *pGame, int numOfItems) {

  int spawn = rand() % 50;

  if (spawn == 0 && numOfItems != MAX_ITEMS) {
    pGame->pItemImage[numOfItems] = createItemImage(pGame->pRenderer);
    pGame->pItems[numOfItems] = createItem(pGame->pItemImage[numOfItems], WINDOW_WIDTH, WINDOW_HEIGHT, 0, 500, 500);
    numOfItems++;
  }

  return numOfItems;

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
*  Build scene and text handler.
*  Text handler to be able to type input to window with SDL_StartTextInput()
*  It returns two values, ipAddr and playerName, to struct Game (pGame)
*/
int build_handler(Game *pGame) {

  // Enable text input handling
  SDL_StartTextInput();

  SDL_Event event;
  ClientData cData;

  // Flags
  int ESC = 0;
  int hasErased = 0;

  // Input mananger
  int input_index = 0;

  // Input values
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
          close(pGame);
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

          // Return back to menu
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            Mix_PlayMusic(pGame->pSelectSound, 0);
            closeRequest = 1;
            ESC = 1;
            pGame->scene = MENU_SCENE;
          }

          // Confirm input
          if (event.key.keysym.sym == SDLK_RETURN) {
            Mix_PlayMusic(pGame->pSelectSound, 0);
            closeRequest = 1;
            pGame->scene = LOBBY_SCENE;
          }

          // Change between clientName (0) or ipAddr (1)
          if (event.key.keysym.sym == SDLK_UP) {
            Mix_PlayMusic(pGame->pClickSound, 0);
            input_index -= 1;
            if (input_index < 0) input_index = 0;
          }

          if (event.key.keysym.sym == SDLK_DOWN) {
            Mix_PlayMusic(pGame->pClickSound, 0);
            input_index += 1;
            if (input_index > 1) input_index = 1;
          }

          // Erase a char clientName (0) or ipAddr (1)
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

    // Erase space in input when starting
    if (!hasErased) {
      for (int i = 0, j = 0, k = 0; i < 2; i++) {
        if (clientName[i] != ' ') {
          clientName[j] = clientName[i];
          j++;
        }
        if (ipAddr[i] != ' ') {
          ipAddr[k] = ipAddr[i];
          k++;
        }
      }
      hasErased++;
    }

    render_background(pGame);

    // A part of the background (table)
    SDL_Rect table_rect;
    table_rect.x = WINDOW_WIDTH / 2 - 200;
    table_rect.y = WINDOW_HEIGHT / 2 - 175;
    table_rect.w = 400;
    table_rect.h = 300;

    SDL_SetRenderDrawColor(pGame->pRenderer, 58, 103, 131, 255);
    SDL_RenderFillRect(pGame->pRenderer, &table_rect);
    SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(pGame->pRenderer, &table_rect);

    // Input field ipAddr
    SDL_Rect ipAddr_rect; 
    ipAddr_rect.x = WINDOW_WIDTH / 2 - 150;
    ipAddr_rect.y = WINDOW_HEIGHT / 2 + 45;
    ipAddr_rect.w = 300;
    ipAddr_rect.h = 50;

    // Input field ipAddr
    SDL_Rect clientName_rect;
    clientName_rect.x = WINDOW_WIDTH / 2 - 150;
    clientName_rect.y = WINDOW_HEIGHT / 2 - 100;
    clientName_rect.w = 300;
    clientName_rect.h = 50;

    SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(pGame->pRenderer, &ipAddr_rect);
    SDL_RenderDrawRect(pGame->pRenderer, &clientName_rect);
        
    // Char rect ipAddr
    SDL_Rect char_ipAddr_rect = {
      ipAddr_rect.x + 15,
      ipAddr_rect.y + 15,
      ipAddr_rect.w - 30,
      ipAddr_rect.h - 30
    };

    // Char rect clientName
    SDL_Rect char_clientName_rect = {
      clientName_rect.x + 15,
      clientName_rect.y + 15,
      clientName_rect.w - 30,
      clientName_rect.h - 30
    };

    // A guiding dots
    SDL_Rect dotTop_rect;
    dotTop_rect.x = WINDOW_WIDTH / 2 - 179;
    dotTop_rect.y = WINDOW_HEIGHT / 2 - 80;
    dotTop_rect.w = 10;
    dotTop_rect.h = 10;

    SDL_Rect dotBottom_rect;
    dotBottom_rect.x = WINDOW_WIDTH / 2 - 179;
    dotBottom_rect.y = WINDOW_HEIGHT / 2 + 65;
    dotBottom_rect.w = 10;
    dotBottom_rect.h = 10;

    // Colors light version
    SDL_Color text_color = { 255, 255, 255, 255 };

    // Colors dark version
    SDL_Color dark_text_color = { 153, 153, 153, 255 };

    // Highlight the chosen input_index
    if (input_index == 0) {
      SDL_Surface* client_surface = TTF_RenderText_Solid(pGame->pStrdFont, clientName, text_color);
      SDL_Texture* client_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, client_surface);
      SDL_Surface* ipAddr_surface = TTF_RenderText_Solid(pGame->pStrdFont, ipAddr, dark_text_color);
      SDL_Texture* ipAddr_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, ipAddr_surface);
      SDL_RenderCopy(pGame->pRenderer, client_texture, NULL, &char_clientName_rect);
      SDL_RenderCopy(pGame->pRenderer, ipAddr_texture, NULL, &char_ipAddr_rect);
      SDL_RenderFillRect(pGame->pRenderer, &dotTop_rect);
      draw_text(pGame->pMessIP);
      draw_text(pGame->pMessName);
    }

    if (input_index == 1) {
      SDL_Surface* client_surface = TTF_RenderText_Solid(pGame->pStrdFont, clientName, dark_text_color);
      SDL_Texture* client_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, client_surface);
      SDL_Surface* ipAddr_surface = TTF_RenderText_Solid(pGame->pStrdFont, ipAddr, text_color);
      SDL_Texture* ipAddr_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, ipAddr_surface);
      SDL_RenderCopy(pGame->pRenderer, client_texture, NULL, &char_clientName_rect);
      SDL_RenderCopy(pGame->pRenderer, ipAddr_texture, NULL, &char_ipAddr_rect);
      SDL_RenderFillRect(pGame->pRenderer, &dotBottom_rect);
      draw_text(pGame->pMessIP);
      draw_text(pGame->pMessName);
    }

    SDL_RenderPresent(pGame->pRenderer);

  }

  // Disable text input handling, clear screen
  SDL_StopTextInput();
  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);

  // Copy string to pGame for future use
  strcpy(pGame->ipAddr, ipAddr);
  strcpy(pGame->myName, clientName);

  // Has not exited build_handler function
  if (!ESC) {
    if ( !conn_server(pGame) ) return 0;
    clientReady(pGame);
  }

  return 1;

}

/**
*  Establish a client to server connection.
*  Text prompt to enter IP address.
*  \param pSocket Open a UDP network socket.
*  \param pPacket Allocate/resize/free a single UDP packet.
*/
int conn_server(Game *pGame) {

  if ( !(pGame->pSocket = SDLNet_UDP_Open(UDP_CLIENT_PORT)) ) {
    printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
    return 0;
  }

  if (SDLNet_ResolveHost(&(pGame->serverAdd), pGame->ipAddr, UDP_SERVER_PORT)) {
    printf("SDLNet_ResolveHost (%s: 2000): %s\n", pGame->ipAddr, SDLNet_GetError());
    return 0;
  }

  if ( !(pGame->pPacket = SDLNet_AllocPacket(DATA_SIZE)) ) {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    return 0;
  }

  pGame->pPacket->address.host = pGame->serverAdd.host;
  pGame->pPacket->address.port = pGame->serverAdd.port;

  return 1;

}

/* Wrap in packet that client is ready */
void clientReady(Game *pGame) {

  ClientData cData;

  cData.command = READY;
  cData.snkeNumber = -1;
  strcpy(cData.clientName, pGame->myName);
  memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
	pGame->pPacket->len = sizeof(ClientData);

  pGame->send_data = 1;

}

/* When player has closed window or left the game */
void disconnect(Game *pGame) {
  
  ClientData cData;

  cData.command = DISC;
  strcpy(cData.clientName, pGame->myName);
  memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
	pGame->pPacket->len = sizeof(ClientData);

  SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);

}

/* Update the names of every client in the game */
void update_PlayerInfo(Game *pGame) {

  ServerData sData;

  memcpy(&sData, pGame->pPacket->data, sizeof(ServerData));
  for (int i = 0; i < MAX_SNKES; i++) {
    strcpy(pGame->SnakeNames[i], sData.playerName[i]);
  }

}

/* Copy new data to struct (ServerData) and update snake */
void update_ServerData(Game *pGame) {

  ServerData sData;

  memcpy(&sData, pGame->pPacket->data, sizeof(ServerData));
  pGame->snkeID = sData.snkeNum;
  pGame->state = sData.gState;

  for (int i = 0; i < MAX_SNKES; i++) {
    update_recived_snake_data(pGame->pSnke[i], &(sData.snakes[i]));
  }

}

/* Draw the interface the user sees when playing */
void draw_interface(Game* pGame) {

  // Sorting the player with highest score first
  // int i, j;
  // Player temp;
  // for (i = 0; i < 4 - 1; i++) {
  //   for (j = 0; j < 4 - i - 1; j++) {
  //     if (players[j].playerScore < players[j+1].playerScore) {
  //       temp = players[j];
  //       players[j] = players[j+1];
  //       players[j+1] = temp;
  //     }
  //   }
  // }

  int txtY = 0;

  render_background(pGame);

  // Render the walls of the playing field
  SDL_Rect field_Walls;
  field_Walls.x = WINDOW_WIDTH - 700;
  field_Walls.y = 0;
  field_Walls.w = WINDOW_WIDTH;
  field_Walls.h = WINDOW_HEIGHT;

  SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
  SDL_RenderFillRect(pGame->pRenderer, &field_Walls);

  // Render the field
  SDL_Rect field_rect;
  field_rect.x = WINDOW_WIDTH - 695;
  field_rect.y = 5;
  field_rect.w = WINDOW_WIDTH - 210;
  field_rect.h = WINDOW_HEIGHT - 10;

  SDL_SetRenderDrawColor(pGame->pRenderer, 58, 103, 131, 255);
  SDL_RenderFillRect(pGame->pRenderer, &field_rect);

  // Render the leaderboard
  draw_text(pGame->pBoardInfo);
  for(int i = 0; i < MAX_SNKES; i++) {

    SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(pGame->pRenderer, 13, 46 + txtY, 185, 46 + txtY);
    
    pGame->pBoardText = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    pGame->SnakeNames[i], 55, 65 + txtY);

    if (!pGame->pBoardText) {
      printf("Error: %s\n", SDL_GetError());
      close(pGame);
    }

    draw_text(pGame->pBoardText);
    txtY += 35;

  }
  
}

/* Checks nrOfCollisions, if 1 snake alive sets collided to 1 */
void collision_counter(Game *pGame) {

  int nrOfCollisions = 0;
  for (int i = 0; i < MAX_SNKES; i++) {
    if (pGame->pSnke[i]->snakeCollided == 1) nrOfCollisions++;
  }

  if (nrOfCollisions == MAX_SNKES - (MAX_SNKES-1)) pGame->collided = 1;

}

/* Sets the game state to START and resets to default values */
void reset_game(Game *pGame) {
  
  for (int i = 0; i < MAX_SNKES; i++){
    reset_snake(pGame->pSnke[i], i);
  }

  pGame->roundCount++;
  pGame->collided = 0;
  if(pGame->roundCount == MAX_ROUNDS){
    pGame->state = START;
    pGame->roundCount = 0;
  } else {
    countDown(pGame);
  }
  
}

/* Create own font with create_font function. Value is stored in a Text pointer. */
int create_allFonts(Game *pGame) {

  pGame->pTitleBigFont = create_font(pGame->pTitleBigFont, "../lib/resources/ATW.ttf", 150);
  pGame->pTitleSmallFont = create_font(pGame->pTitleSmallFont, "../lib/resources/ATW.ttf", 110);
  pGame->pNumberFont = create_font(pGame->pStrdFont, "../lib/resources/PixeloidSansBold-PKnYd.ttf", 50);
  pGame->pStrdFont = create_font(pGame->pStrdFont, "../lib/resources/PixeloidSansBold-PKnYd.ttf", 25);
  pGame->pNameFont = create_font(pGame->pNameFont, "../lib/resources/PixeloidSansBold-PKnYd.ttf", 15);

  if (!pGame->pNumberFont || !pGame->pNameFont || !pGame->pTitleBigFont || !pGame->pTitleSmallFont || !pGame->pStrdFont) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  return 1;

}

/* Create own text with create_text function. Value is stored in a Text pointer. */
int create_allText(Game *pGame) {

  pGame->pTitleSmallText = create_text(pGame->pRenderer, 38, 175, 255, pGame->pTitleSmallFont,
    "Trail", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 180);

  pGame->pTitleBigText = create_text(pGame->pRenderer, 255, 110, 51, pGame->pTitleBigFont,
    "Clash", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 70);

  pGame->pStartText = create_text(pGame->pRenderer, 38, 175, 255, pGame->pStrdFont,
    "START GAME", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 50);

  pGame->pStartDark = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "START GAME", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 50);

  pGame->pQuitText = create_text(pGame->pRenderer, 38, 175, 255, pGame->pStrdFont,
    "QUIT", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  pGame->pQuitDark = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "QUIT", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  pGame->pWaitingText = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "waiting for other players...", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

  pGame->pSelectText = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "to select", 174, WINDOW_HEIGHT / 2 + 223);

  pGame->pContinueText = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "to continue", 187, WINDOW_HEIGHT / 2 + 223);

  pGame->pBackText = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "to go back", 800, WINDOW_HEIGHT / 2 + 223);

  pGame->pMessIP = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "Enter IP Address", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 10);

  pGame->pMessName = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "Enter Player Name", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 135);

  pGame->pBoardInfo = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "Player  -  P", (WINDOW_WIDTH - 695) / 2, 25);

  pGame->pNumThree = create_text(pGame->pRenderer, 255, 255, 255, pGame->pNumberFont,
    "3", (WINDOW_WIDTH + 205) / 2, WINDOW_HEIGHT / 2);

  pGame->pNumTwo = create_text(pGame->pRenderer, 255, 255, 255, pGame->pNumberFont,
    "2", (WINDOW_WIDTH + 205) / 2, WINDOW_HEIGHT / 2);

  pGame->pNumOne = create_text(pGame->pRenderer, 255, 255, 255, pGame->pNumberFont,
    "1", (WINDOW_WIDTH + 205) / 2, WINDOW_HEIGHT / 2);

  // Checking if there is an error regarding all the Text pointer.
  if (!pGame->pNumOne || !pGame->pNumTwo || !pGame->pNumThree || !pGame->pBoardInfo || !pGame->pBackText || !pGame->pMessName || 
      !pGame->pMessIP || !pGame->pContinueText || !pGame->pSelectText || !pGame->pStartText || !pGame->pStartDark || 
      !pGame->pWaitingText || !pGame->pTitleBigText || !pGame->pTitleSmallText || !pGame->pQuitText || !pGame->pQuitDark) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  return 1;

}

/* 
*  Create all snakes (players) in the array.
*  Checking for errors.
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

/* Creates Items and creates their image */
int init_Items(Game *pGame) {

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

/* Initiate image load */
int init_image(Game *pGame) {

  pGame->pSpaceSurface = IMG_Load("../lib/resources/space.png");
  pGame->pEnterSurface = IMG_Load("../lib/resources/enter.png");
  pGame->pEscSurface = IMG_Load("../lib/resources/esc.png");
  if (!pGame->pSpaceSurface || !pGame->pEnterSurface || !pGame->pEscSurface) {
    SDL_FreeSurface(pGame->pSpaceSurface);
    SDL_FreeSurface(pGame->pEnterSurface);
    SDL_FreeSurface(pGame->pEscSurface);
    close(pGame);
    return 0;
  }

  pGame->pSpaceTexture = SDL_CreateTextureFromSurface(pGame->pRenderer, pGame->pSpaceSurface);
  pGame->pEnterTexture = SDL_CreateTextureFromSurface(pGame->pRenderer, pGame->pEnterSurface);
  pGame->pEscTexture = SDL_CreateTextureFromSurface(pGame->pRenderer, pGame->pEscSurface);
  if (!pGame->pSpaceTexture || !pGame->pEnterTexture || !pGame->pEscTexture) {
    close(pGame);
    return 0;
  }
  SDL_FreeSurface(pGame->pSpaceSurface);
  SDL_FreeSurface(pGame->pEnterSurface);
  SDL_FreeSurface(pGame->pEscSurface);

  return 1;

}

/* Initializes audios/sounds & checks for error */
int init_Music(Game *pGame) {

  if (Mix_Init(MIX_INIT_MP3) != MIX_INIT_MP3) {
    printf("Failed to initialize SDL_mixer: %s\n", Mix_GetError());
    return 0;
  }
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("Failed to open audio device: %s\n", Mix_GetError());
    Mix_Quit();
    SDL_Quit();
    return 0;
  }
  pGame->pClickSound = Mix_LoadMUS("../lib/resources/click_sound.wav");
  pGame->pSelectSound = Mix_LoadMUS("../lib/resources/select_sound.wav");
  pGame->pMusic = Mix_LoadMUS("../lib/resources/game_music.mp3");

  if(!pGame->pMusic || !pGame->pClickSound || !pGame->pSelectSound) {
    printf("Failed to load music: %s\n", Mix_GetError());
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
    return 0;
  }

}

/* Destoryes various SDL libraries and snakes. Safe way when exiting game. */
void close(Game *pGame) {

  for (int i = 0; i < MAX_SNKES; i++) 
    if (pGame->pSnke[i]) destroy_snake(pGame->pSnke[i]);

  if (pGame->pSpaceTexture) SDL_DestroyTexture(pGame->pSpaceTexture);
  if (pGame->pEnterTexture) SDL_DestroyTexture(pGame->pEnterTexture);
  if (pGame->pEscTexture) SDL_DestroyTexture(pGame->pEscTexture);

  if (pGame->pRenderer) SDL_DestroyRenderer(pGame->pRenderer);
  if (pGame->pWindow) SDL_DestroyWindow(pGame->pWindow);

  // Destroy text
  if (pGame->pTitleSmallText) destroy_text(pGame->pTitleSmallText);
  if (pGame->pTitleBigText) destroy_text(pGame->pTitleBigText);
  if (pGame->pInGameTitle) destroy_text(pGame->pInGameTitle);  
  if (pGame->pWaitingText) destroy_text(pGame->pWaitingText);
  if (pGame->pStartText) destroy_text(pGame->pStartText);  
  if (pGame->pStartDark) destroy_text(pGame->pStartDark);
  if (pGame->pQuitDark) destroy_text(pGame->pQuitDark);
  if (pGame->pQuitText) destroy_text(pGame->pQuitText);
  if (pGame->pBackText) destroy_text(pGame->pBackText);
  if (pGame->pMessName) destroy_text(pGame->pMessName);
  if (pGame->pMessIP) destroy_text(pGame->pMessIP);
  if (pGame->pSnkeName) destroy_text(pGame->pSnkeName);
  if (pGame->pBoardText) destroy_text(pGame->pBoardText);
  if (pGame->pBoardInfo) destroy_text(pGame->pBoardInfo);
  if (pGame->pNumThree) destroy_text(pGame->pNumThree);
  if (pGame->pNumTwo) destroy_text(pGame->pNumTwo);
  if (pGame->pNumOne) destroy_text(pGame->pNumOne);

  for (int i = 0; i < MAX_ITEMS; i++) {
    if (pGame->pItems[i]) destroyItem(pGame->pItems[i]);
    if (pGame->pItemImage[i]) destroyItemImage(pGame->pItemImage[i]);
  }

  // Close fonts
  if (pGame->pTitleSmallFont) TTF_CloseFont(pGame->pTitleSmallFont);
  if (pGame->pTitleBigFont) TTF_CloseFont(pGame->pTitleBigFont);
  if (pGame->pStrdFont) TTF_CloseFont(pGame->pStrdFont);
  if (pGame->pNameFont) TTF_CloseFont(pGame->pNameFont);
  if (pGame->pNumberFont) TTF_CloseFont(pGame->pNumberFont);

  // Free music
  Mix_FreeMusic(pGame->pClickSound); 
  Mix_FreeMusic(pGame->pSelectSound);
  Mix_FreeMusic(pGame->pMusic);
  Mix_CloseAudio();
  Mix_Quit();

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}