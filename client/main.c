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

/* Client Game struct */
typedef struct game {

  char ipAddr[INPUT_BUFFER_SIZE];
  char myName[INPUT_BUFFER_SIZE];

  // NAME OF EVERY PLAYER
  char playerNames[MAX_SNKES][INPUT_BUFFER_SIZE];

  // THE SCORE
  int scoreNum[MAX_SNKES];
  char scoreText[MAX_SNKES][2];

  int roundCount;

  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;

  // FLAGS
  int initStart;
  int textIndex;

  // IMAGES
  SDL_Texture *pSpaceTexture, *pEnterTexture, *pEscTexture;
  SDL_Surface *pSpaceSurface, *pEnterSurface, *pEscSurface;

  // SNAKE
  Snake *pSnke[MAX_SNKES];
  int collision;
  int snkeID;

  // SOUND
  Mix_Music *pClickSound, *pSelectSound, *pMusic, *pWinSound;

  // UI
  TTF_Font *pStrdFont, *pTitleBigFont, *pTitleSmallFont, *pNameFont, *pNumberFont;
  Text *pInGameTitle, *pTitleBigText, *pTitleSmallText, *pStartText, *pStartDark, *pQuitText, *pQuitDark,
  *pSelectText, *pContinueText, *pBackText, *pWaitingText, *pMessIP, *pMessName, *pSnkeName, *pBoardText,
  *pBoardInfo, *pNumThree, *pNumTwo, *pNumOne, *pScoreText, *pRoundNum, *pRoundText, *pWinner;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress serverAdd;
  int currentClients;

  GameState state;
  GameScene scene;

} Game;

int init_structure(Game *pGame);
void run(Game *pGame);
void close(Game *pGame);

int init_Music(Game *pGame);
int init_image(Game *pGame);
int init_allFonts(Game *pGame);
int init_allText(Game *pGame);
int init_allSnakes(Game *pGame);
int build_handler(Game *pGame);
int init_connection(Game *pGame);
int render_scene(Game *pGame);

void input_handler(Game *pGame, SDL_Event *pEvent);
void render_game(Game *pGame);
void render_elements(Game *pGame);
void render_background(Game *pGame);
void looby(Game *pGame);
void winner(Game *pGame);
void find_winner(Game *pGame);
void finish_game(Game *pGame);
void render_round(Game *pGame);
void highlight_text(Game *pGame, int index);
void update_ServerData(Game *pGame);
void new_game(Game *pGame);
void draw_interface(Game* pGame);
void collision_counter(Game *pGame);
void clientCommand(Game *pGame, int command);
void draw_snakeName(Game *pGame, int i);
void update_Names(Game *pGame);
void create_snakePointers(Game *pGame);
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

  // Default values
  pGame->state = MENU;
  pGame->scene = MENU_SCENE;

  pGame->initStart = 0;
  pGame->textIndex = 0;
  pGame->roundCount = 0;
  pGame->currentClients = 0;

  for (int i = 0; i < MAX_SNKES; i++)
    pGame->scoreNum[i] = 0;

  // Init everthing needed
  if ( !init_sdl_libraries() ) return 0;

  pGame->pWindow = client_wind("Trail Clash", pGame->pWindow);
  if ( !pGame->pWindow ) close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if ( !pGame->pRenderer) close(pGame);

  if ( !init_Music(pGame) ) return 0;

  if ( !init_image(pGame) ) return 0;

  if ( !init_allSnakes(pGame) ) return 0;

  if ( !init_allFonts(pGame) ) return 0;

  if ( !init_allText(pGame) ) return 0;

  return 1;

}

/** 
 * Main loop of the game. 
 * Updates the snakes attributes. 
 * Looking for input. 
 * Sends and recives new data. 
*/
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;

  int closeRequest = 0;
  while(!closeRequest) {
    switch (pGame->state) {
      // The game is running
      case RUNNING:

        create_snakePointers(pGame);

        // Initiates only one time.
        if (!pGame->initStart) {
          update_Names(pGame);
          render_game(pGame);
          countDown(pGame);
          pGame->initStart++;
        }

        // Update new recived data
        while(SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
          update_ServerData(pGame);
        }

        render_game(pGame);

        // Side note: This one leads to a segmentation fault
        if (!Mix_PlayingMusic()) Mix_PlayMusic(pGame->pMusic, -1);

        // Ifall one player is alive, new round
        if (pGame->collision == 1) new_game(pGame);

        // Looking if there is an input
        if (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT) {
            closeRequest = 1;
            clientCommand(pGame, 0);
          }
          else input_handler(pGame, &event);
        }

        //Check if one Snake left
        collision_counter(pGame);

      break;
      // Main Menu
      case MENU:

        // Render a choosen scene
        if ( !render_scene(pGame) ) closeRequest = 1;
        
        // Looking if there is an input
        if (SDL_PollEvent(&event)) {

          if (event.type == SDL_QUIT) closeRequest = 1;

          if (pGame->scene == MENU_SCENE && event.type == SDL_KEYDOWN) {
            // Arrow up, W
            if (event.key.keysym.scancode == SDL_SCANCODE_UP || event.key.keysym.scancode == SDL_SCANCODE_W) {
              Mix_PlayMusic(pGame->pClickSound, 0);
              pGame->textIndex -= 1;
              if (pGame->textIndex < 0) pGame->textIndex = 0;
            }

            // Arrow down, S
            if (event.key.keysym.scancode == SDL_SCANCODE_DOWN || event.key.keysym.scancode == SDL_SCANCODE_S) {
              Mix_PlayMusic(pGame->pClickSound, 0);
              pGame->textIndex += 1;
              if (pGame->textIndex > 1) pGame->textIndex = 1;
            }

            // Select content (Space)
            if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
              Mix_PlayMusic(pGame->pSelectSound, 0);
              // START GAME
              if (pGame->textIndex == 0) pGame->scene = BUILD_SCENE;

              // QUIT
              if (pGame->textIndex == 1) closeRequest = 1;
            }

          }

        }

        // Update new recived data
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) update_ServerData(pGame);

      break;
    }
  }

}

/* Count down from 3 */
void countDown(Game *pGame) {

  // Three
  draw_text(pGame->pNumThree);
  SDL_RenderPresent(pGame->pRenderer);
  Mix_PlayMusic(pGame->pSelectSound, 0);
  SDL_Delay(1000);

  // Two
  render_game(pGame);
  draw_text(pGame->pNumTwo);
  SDL_RenderPresent(pGame->pRenderer);
  Mix_PlayMusic(pGame->pSelectSound, 0);
  SDL_Delay(1000);

  // One
  render_game(pGame);
  draw_text(pGame->pNumOne);
  SDL_RenderPresent(pGame->pRenderer);
  Mix_PlayMusic(pGame->pSelectSound, 0);
  SDL_Delay(1000);

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

/* Show the selected scene */
int render_scene(Game *pGame) {

  switch (pGame->scene) {
    case MENU_SCENE:
      // Title, background
      render_elements(pGame);
      draw_text(pGame->pTitleSmallText);
      draw_text(pGame->pTitleBigText);
      // Highligt specific content
      highlight_text(pGame, pGame->textIndex);
    break;
    case BUILD_SCENE:
      if ( !build_handler(pGame) ) return 0;
    break;
    case LOBBY_SCENE:
      looby(pGame);
    break;
    case WINNER_SCENE:
      winner(pGame);
    break;

  }

  // Update screen
  SDL_RenderPresent(pGame->pRenderer);

  return 1;

}

/* Render who won the game */
void winner(Game *pGame) {

  Mix_PlayMusic(pGame->pWinSound, 0);
  draw_interface(pGame);
  find_winner(pGame);
  SDL_RenderPresent(pGame->pRenderer);

  SDL_Delay(5000);

  pGame->scene = MENU_SCENE;
  pGame->state = MENU;

}

/* Find who the winner is, based on the highest score */
void find_winner(Game *pGame) {

  // Find who has the highest score
  int maxNum = 0, minNum = 0, player = 0;
  for(int i = 0; i < MAX_SNKES; i++) {

    for(int j = 0; j < MAX_SNKES; j++) {

      if (minNum < pGame->scoreNum[i]) {
        minNum = pGame->scoreNum[i];
        player = i;
      }

      if (maxNum < minNum) {
        maxNum = minNum;
        player = player;
      }

    }

  }

  // Render the winner
  char Winner[INPUT_BUFFER_SIZE] = "";
  strcat(Winner, "The Winner is ");
  strcat(Winner, pGame->playerNames[player]);

  pGame->pWinner = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    Winner, (WINDOW_WIDTH + 205) / 2, WINDOW_HEIGHT / 2);

  if (!pGame->pWinner) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
  }

  draw_text(pGame->pWinner);

}

/* Render the background of the game */
void render_background(Game *pGame) {

  SDL_SetRenderDrawColor(pGame->pRenderer, 9, 66, 100, 255); // Dark Blue
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer, 7, 52, 80, 255);  // Darker color then the previous

  // Creates small rect in a checkboard pattern
  for (int i = 0; i <= WINDOW_WIDTH; i += 10) {
    for(int j = 0; j <= WINDOW_HEIGHT; j += 10) {
      SDL_Rect pointRect = { i - POINT_SIZE / 2, j - POINT_SIZE / 2, POINT_SIZE, POINT_SIZE };
      SDL_RenderFillRect(pGame->pRenderer, &pointRect);
    }
  }

}

/* Render a element for the game */
void render_elements(Game *pGame) {

  // Render the background
  render_background(pGame);

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

/* Render snakes, trail and names for the game to the window */
void render_game(Game *pGame) {

  draw_interface(pGame);

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
    pGame->playerNames[i], pGame->pSnke[i]->xCord + 3, pGame->pSnke[i]->yCord - 10);

  if (!pGame->pSnkeName) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
  }

  draw_text(pGame->pSnkeName);

}

/* Function to highligt a specific content */
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
 * Build scene and text handler.
 * Text handler to be able to type input to window with SDL_StartTextInput()
 * It returns two values, ipAddr and playerName, to struct Game (pGame)
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
  int inputIndex = 0;

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
          if (inputIndex == 0) {
            if (clientName_pos < INPUT_BUFFER_SIZE) {
              strcat(clientName, event.text.text);
              clientName_pos += strlen(event.text.text);
            }
          }

          // Enter Ip Address
          if (inputIndex == 1) {
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
            ESC = 1;
            closeRequest = 1;
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
            inputIndex -= 1;
            if (inputIndex < 0) inputIndex = 0;
          }

          if (event.key.keysym.sym == SDLK_DOWN) {
            Mix_PlayMusic(pGame->pClickSound, 0);
            inputIndex += 1;
            if (inputIndex > 1) inputIndex = 1;
          }

          // Erase a char clientName (0) or ipAddr (1)
          if (inputIndex == 0) {
            if (event.key.keysym.sym == SDLK_BACKSPACE && clientName_pos > 0) {
              clientName[clientName_pos - 1] = '\0';
              clientName_pos--;
            }
          }

          if (inputIndex == 1) {
            if (event.key.keysym.sym == SDLK_BACKSPACE && ipAddr_pos > 0) {
              ipAddr[ipAddr_pos - 1] = '\0';
              ipAddr_pos--;
            }
          }

        break;
      }
    }

    // Erase a space character when typing for the first time
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

    render_elements(pGame);

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

    // Highlight the chosen inputIndex
    if (inputIndex == 0) {
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

    if (inputIndex == 1) {
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

  // Copy string to pGame for future use
  strcpy(pGame->ipAddr, ipAddr);
  strcpy(pGame->myName, clientName);

  // Has not exited build_handler function
  if (!ESC) {
    if ( !init_connection(pGame) ) return 0;
    clientCommand(pGame, 1);
  }

  return 1;

}

/**
 * Establish a client to server connection.
 * Text prompt to enter IP address.
 * \param pSocket Open a UDP network socket.
 * \param pPacket Allocate/resize/free a single UDP packet.
*/
int init_connection(Game *pGame) {

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

/**
 * Client is ready to play 
 * \param command If 0 client is disconnecting, otherwise ready.
*/
void clientCommand(Game *pGame, int command) {

  ClientData cData;

  cData.command = READY;
  if (!command) cData.command = DISC;
  cData.snkeNumber = -1;
  strcpy(cData.clientName, pGame->myName);
  memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
	pGame->pPacket->len = sizeof(ClientData);

  SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket);

}

/* Update the names of every client in the game */
void update_Names(Game *pGame) {

  ServerData sData;

  memcpy(&sData, pGame->pPacket->data, sizeof(ServerData));
  for (int i = 0; i < MAX_SNKES; i++) {
    strcpy(pGame->playerNames[i], sData.playerName[i]);
    strcpy(pGame->scoreText[i], "0");
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
    sprintf(pGame->scoreText[i], "%d", sData.died[i]);
    pGame->scoreNum[i] = sData.died[i];
  }

}

/* Draw the interface the user sees when playing */
void draw_interface(Game* pGame) {

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

  if (pGame->scene != WINNER_SCENE) render_round(pGame);

  // Render "Player - P"
  draw_text(pGame->pBoardInfo);

  // Render the leaderboard
  for(int i = 0; i < MAX_SNKES; i++) {

    SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(pGame->pRenderer, 13, 61 + txtY, 185, 61 + txtY);
    
    char strInfo[INPUT_BUFFER_SIZE] = "";

    strcat(strInfo, pGame->playerNames[i]);
    strcat(strInfo, " - ");
    strcat(strInfo, pGame->scoreText[i]);

    pGame->pBoardText = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
      strInfo, (WINDOW_WIDTH - 695) / 2, 85 + txtY);

    if (!pGame->pBoardText) {
      printf("Error: %s\n", SDL_GetError());
      close(pGame);
    }

    draw_text(pGame->pBoardText);
    txtY += 45;

  }
  
}

/* Render what current round the players are playing */
void render_round(Game *pGame) {

  char round[10] = "";
  sprintf(round, "%d", pGame->roundCount + 1);
  strcat(round, "/3");

  pGame->pRoundText = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "Round", (WINDOW_WIDTH - 695) / 2, WINDOW_HEIGHT- 120);

  pGame->pRoundNum = create_text(pGame->pRenderer, 255, 255, 255, pGame->pNumberFont,
    round, (WINDOW_WIDTH - 695) / 2, WINDOW_HEIGHT - 70);

  if (!pGame->pRoundNum || !pGame->pRoundText) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
  }

  draw_text(pGame->pRoundText);
  draw_text(pGame->pRoundNum);

}

/* Check if there is one snake left alive to trigger flag collision to 1 */
void collision_counter(Game *pGame) {

  int numOfCollision = 0;
  for (int i = 0; i < MAX_SNKES; i++)
    if (pGame->pSnke[i]->snakeCollided == 1) numOfCollision++;

  if (numOfCollision >= MAX_SNKES - 1) pGame->collision = 1;

}

/* Function to display the winner and reset values to default */
void finish_game(Game *pGame) {

  pGame->initStart = 0;
  pGame->textIndex = 0;
  pGame->textIndex = 0;
  pGame->roundCount = 0;
  pGame->currentClients = 0;
  pGame->scene = WINNER_SCENE;
  render_scene(pGame);

}

/* Check if a new round should start or to display the winner */
void new_game(Game *pGame) {

  pGame->collision = 0;
  pGame->roundCount += 1;
  Mix_PauseMusic();
  
  for (int i = 0; i < MAX_SNKES; i++)
    reset_snake(pGame->pSnke[i], i);

  if (pGame->roundCount == MAX_ROUNDS) {
    finish_game(pGame);
  }
  else countDown(pGame);
  
}

/* Create own font with create_font function. Value is stored in a Text pointer. */
int init_allFonts(Game *pGame) {

  pGame->pTitleBigFont = create_font(pGame->pTitleBigFont, "../lib/resources/fonts/ATW.ttf", 150);
  pGame->pTitleSmallFont = create_font(pGame->pTitleSmallFont, "../lib/resources/fonts/ATW.ttf", 110);
  pGame->pNumberFont = create_font(pGame->pNumberFont, "../lib/resources/fonts/PixeloidSansBold-PKnYd.ttf", 50);
  pGame->pStrdFont = create_font(pGame->pStrdFont, "../lib/resources/fonts/PixeloidSansBold-PKnYd.ttf", 25);
  pGame->pNameFont = create_font(pGame->pNameFont, "../lib/resources/fonts/PixeloidSansBold-PKnYd.ttf", 15);

  if (!pGame->pNumberFont || !pGame->pNameFont || !pGame->pTitleBigFont || !pGame->pTitleSmallFont || !pGame->pStrdFont) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  return 1;

}

/* Create own text with create_text function. Value is stored in a Text pointer. */
int init_allText(Game *pGame) {

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
    "Player - P", (WINDOW_WIDTH - 695) / 2, 35);

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
 * Create all snakes (players) in the array.
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

/* Initiate image load */
int init_image(Game *pGame) {

  pGame->pSpaceSurface = IMG_Load("../lib/resources/pictures/space.png");
  pGame->pEnterSurface = IMG_Load("../lib/resources/pictures/enter.png");
  pGame->pEscSurface = IMG_Load("../lib/resources/pictures/esc.png");
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

  pGame->pClickSound = Mix_LoadMUS("../lib/resources/sounds/click_sound.wav");
  pGame->pSelectSound = Mix_LoadMUS("../lib/resources/sounds/select_sound.wav");
  pGame->pWinSound = Mix_LoadMUS("../lib/resources/sounds/win_sound.wav");
  pGame->pMusic = Mix_LoadMUS("../lib/resources/sounds/game_music.mp3");

  if(!pGame->pMusic || !pGame->pClickSound || !pGame->pSelectSound || !pGame->pWinSound) {
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
  if (pGame->pRoundText) destroy_text(pGame->pRoundText);
  if (pGame->pRoundNum) destroy_text(pGame->pRoundNum);
  if (pGame->pSelectText) destroy_text(pGame->pSelectText);
  if (pGame->pContinueText) destroy_text(pGame->pContinueText);
  if (pGame->pScoreText) destroy_text(pGame->pScoreText);
  if (pGame->pWinner) destroy_text(pGame->pWinner);

  // Close fonts
  if (pGame->pTitleSmallFont) TTF_CloseFont(pGame->pTitleSmallFont);
  if (pGame->pTitleBigFont) TTF_CloseFont(pGame->pTitleBigFont);
  if (pGame->pStrdFont) TTF_CloseFont(pGame->pStrdFont);
  if (pGame->pNameFont) TTF_CloseFont(pGame->pNameFont);
  if (pGame->pNumberFont) TTF_CloseFont(pGame->pNumberFont);

  // Free music
  Mix_FreeMusic(pGame->pClickSound); 
  Mix_FreeMusic(pGame->pSelectSound);
  Mix_FreeMusic(pGame->pWinSound);
  Mix_FreeMusic(pGame->pMusic);
  Mix_CloseAudio();
  Mix_Quit();

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}