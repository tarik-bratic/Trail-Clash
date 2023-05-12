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

#define HEJADIL 3

/* Client Game struct (Snake, UI, Network) */
typedef struct game {

  // SNAKE
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke[MAX_SNKES];
  int num_of_snkes;
  int snkeID;

  // UI
  TTF_Font *pStrdFont, *pTitleBigFont, *pTitleSmallFont, *pNetFont;
  Text *pTitleBigText, *pTitleSmallText, *pStartText, *pStartDark, *pWaitingText, *pQuitText, *pQuitDark;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress serverAdd;

  // SOUND & AUDIO
  Mix_Music *menuSong, *playSong; 
  Mix_Chunk *hitItem, *choiceSound, *clickButton;

  GameState state;

} Game;

int init_conn(Game *pGame);
int init_structure(Game *pGame);
int init_allSnakes(Game *pGame);

void run(Game *pGame);
void close(Game *pGame);
void render_snake(Game *pGame);
void render_background(Game *pGame);
void update_server_data(Game *pGame);
void input_handler(Game *pGame, SDL_Event *pEvent);

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

  pGame->pWindow = main_wind("Trail Clash - client", pGame->pWindow);
  if ( !pGame->pWindow ) close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if ( !pGame->pRenderer) close(pGame);

  pGame->pTitleBigFont = create_font(pGame->pTitleBigFont, "../lib/resources/ATW.ttf", 120);
  if ( !pGame->pTitleBigFont ) close(pGame);

  pGame->pTitleSmallFont = create_font(pGame->pTitleSmallFont, "../lib/resources/ATW.ttf", 100);
  if ( !pGame->pTitleSmallFont ) close(pGame);

  pGame->pStrdFont = create_font(pGame->pStrdFont, "../lib/resources/ATW.ttf", 25);
  if ( !pGame->pStrdFont ) close(pGame);

  pGame->pNetFont = create_font(pGame->pNetFont, "../lib/resources/ATW.ttf", 20);
  if ( !pGame->pNetFont ) close(pGame);


  // 255, 110, 51
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
    "Quit Game", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  pGame->pQuitDark = create_text(pGame->pRenderer, 255, 255, 255, pGame->pStrdFont,
    "Quit Game", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  pGame->pWaitingText = create_text(pGame->pRenderer, 238, 168, 65,pGame->pStrdFont,
    "Waiting for server ...", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  // Checking if there is an error regarding the Text pointer.
  if (!pGame->pStartText || !pGame->pStartDark || !pGame->pWaitingText || !pGame->pTitleBigText || !pGame->pTitleSmallText) {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  //Initializes audios/sounds & checks for error
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
  pGame->menuSong = Mix_LoadMUS("../lib/resources/main_menu.mp3"); // ta bort
  pGame->playSong = Mix_LoadMUS("../lib/resources/play_game10.mp3");
  pGame->hitItem = Mix_LoadWAV("../lib/resources/boostUp20.wav");
  pGame->choiceSound = Mix_LoadWAV("../lib/resources/choiceSound.wav");
  pGame->clickButton = Mix_LoadWAV("../lib/resources/clickButton.wav");
   if(!pGame->menuSong || !pGame->playSong || !pGame->hitItem || !pGame->choiceSound || !pGame->clickButton)
  {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  } 

  init_allSnakes(pGame);

  // init_conn(pGame);

  Mix_PlayMusic(pGame->menuSong, 0); 

  return 1;

}

/* Main loop of the game. Updates the snakes attributes. Looking for input. Sends and recives new data. */
void run(Game *pGame) {

  SDL_Event event;
  ClientData cData;
  int joining = 0;
  int closeRequest = 0;
  int text_index = 0;

  while(!closeRequest) {

    switch (pGame->state) {
      // The game is running
      case RUNNING:
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

        // Update snake cord, send data
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
      // Main Menu
      case START:
        // If you haven't joined display start text if not display waiting text
        if (!joining) {
          
          render_background(pGame);
          draw_text(pGame->pTitleSmallText);
          draw_text(pGame->pTitleBigText);

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

        // Update screen with new render
        SDL_RenderPresent(pGame->pRenderer);

        // Looking if there is an input
        if (SDL_PollEvent(&event)) {

          // Close window
          if (event.type == SDL_QUIT) closeRequest = 1;

          // Arrow Up
          if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_UP 
          || event.type == SDL_MOUSEMOTION && event.motion.y < WINDOW_HEIGHT / 2 + 70){
             text_index -= 1;
             Mix_PlayChannel(-1, pGame->choiceSound, 0);
             if (text_index < 0) text_index = 0;
          }
          //if (text_index < 0) text_index = 0; //FLYTTADE DENNA INANNFÖR FUNKTIONEN (ADIL)

          // Arrow Down
          if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_DOWN 
          || event.type == SDL_MOUSEMOTION && event.motion.y > WINDOW_HEIGHT / 2 + 70){ 
            text_index += 1;
            Mix_PlayChannel(-1, pGame->choiceSound, 0);
            if (text_index > 1) text_index = 1;
          }
          //if (text_index > 1) text_index = 1; // FLYTTADE DENNA INNANFÖR FUNKTIONEN (ADIL)
          
          if (!joining && event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_SPACE 
          || event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            
            //Play Click Sound
            Mix_PlayChannel(-1,pGame->clickButton, 0);
            
            // START GAME
            if (text_index == 0) {
              init_conn(pGame);

              // Change client data and copy to packet (Space is pressed)
              joining = 1;
              cData.command = READY;
              cData.snkeNumber = -1;
              memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
		          pGame->pPacket->len = sizeof(ClientData);

            //Play Game-Music
              Mix_HaltMusic();
              Mix_PlayMusic(pGame->playSong, 0);
              
            }

            // QUIT
            if (text_index == 1) {
              closeRequest = 1;
            }

          }
        }

        // Send client data packet
        if (joining) {
          if ( !SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket) ) {
            printf("Error (UDP_Send): %s", SDLNet_GetError());
          }
        }

        // Update new recived data
        if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket)) {
          update_server_data(pGame);
          if (pGame->state == RUNNING) joining = 0;
        }
      break;

    }

    //SDL_Delay(ONE_MS); Kanske kan användas senare. Låt stå.

  }

  // When player has closed window or left the game
  cData.command = DISC;
  memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
	pGame->pPacket->len = sizeof(ClientData);

  if ( !SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket) ) {
    printf("Error (UDP_Send): %s", SDLNet_GetError());
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

/**
*  Establish a client to server connection.
*  Text prompt to enter IP address.
*  \param pSocket Open a UDP network socket.
*  \param pPacket Allocate/resize/free a single UDP packet.
*/
int init_conn(Game *pGame) {

  // Enable text input handling
  SDL_StartTextInput();

  char mess[30] = "Enter a server IP address";
  char ipAddr[INPUT_BUFFER_SIZE] = "";
  int input_cursor_position = 0;

  int closeRequest = 1;
  while (closeRequest) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          closeRequest = 0;
        break;
        case SDL_TEXTINPUT:
          if (input_cursor_position < 15) {
            strcat(ipAddr, event.text.text);
            input_cursor_position += strlen(event.text.text);
          }
        break;
        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_RETURN) closeRequest = 0;
          if (event.key.keysym.sym == SDLK_BACKSPACE && input_cursor_position > 0) {
            ipAddr[input_cursor_position - 1] = '\0';
            input_cursor_position--;
          }
        break;
      }
    }

    // Clear the screen
    SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
    SDL_RenderClear(pGame->pRenderer);

    // Render the input field
    SDL_Rect input_rect = { 
      WINDOW_WIDTH / 2 - 100, // Rectangle x cord
      WINDOW_HEIGHT / 2 - 25, // Rectangle y cord
      200,                    // Height of the rectangle
      50                      // Width of the rectangle
    };

    SDL_Rect text_rect = { 
      WINDOW_WIDTH / 2 - 100, // Rectangle x cord
      WINDOW_HEIGHT / 2 - 25, // Rectangle y cord
      200,                    // Height of the rectangle
      50                      // Width of the rectangle
    };

    SDL_SetRenderDrawColor(pGame->pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(pGame->pRenderer, &input_rect);
        
    SDL_Rect input_text_rect = { 
      input_rect.x + 15,        // Rectangle x cord
      input_rect.y + 15,        // Rectangle y cord
      input_rect.w - 30,         // Height of the rectangle
      input_rect.h - 30         // Width of the rectangle
    };

    SDL_Rect message = { 
      text_rect.x = 300,        // Rectangle x cord
      text_rect.y = 150,        // Rectangle y cord
      text_rect.w = 300,        // Height of the rectangle
      text_rect.h = 100         // Width of the rectangle
    };

    SDL_Color text_color = { 
      255, 255, 255, 255 
    };

    SDL_Surface* input_text_surface = TTF_RenderText_Solid(pGame->pNetFont, ipAddr, text_color);
    SDL_Surface* message_surface = TTF_RenderText_Solid(pGame->pNetFont, mess, text_color);
    SDL_Texture* input_text_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, input_text_surface);
    SDL_Texture* message_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, message_surface);
    SDL_RenderCopy(pGame->pRenderer, input_text_texture, NULL, &input_text_rect);
    SDL_RenderCopy(pGame->pRenderer, message_texture, NULL, &message);
    SDL_RenderPresent(pGame->pRenderer);

  }

  // Disable text input handling, clear screen
  SDL_StopTextInput();
  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);

  // Establish client to server
  if ( !(pGame->pSocket = SDLNet_UDP_Open(0)) ) {
    printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
    return 0;
  }

  if (SDLNet_ResolveHost(&(pGame->serverAdd), ipAddr, 2000)) {
    printf("SDLNet_ResolveHost (%s: 2000): %s\n", ipAddr, SDLNet_GetError());
    return 0;
  }

  if ( !(pGame->pPacket = SDLNet_AllocPacket(512)) ) {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    return 0;
  }

  pGame->pPacket->address.host = pGame->serverAdd.host;
  pGame->pPacket->address.port = pGame->serverAdd.port;

  return 1;

}

/* Copy new data to server data and updates snake data */
void update_server_data(Game *pGame) {

    ServerData srvData;

    memcpy(&srvData, pGame->pPacket->data, sizeof(ServerData));
    pGame->snkeID = srvData.snkeNum;
    pGame->state = srvData.gState;

    for (int i = 0; i < MAX_SNKES; i++)
      update_recived_snake_data(pGame->pSnke[i], &(srvData.snakes[i]));

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

/* Render and presents a snake to the window */
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

//Render background for the game
void render_background(Game *pGame) {
  SDL_SetRenderDrawColor(pGame->pRenderer, 9, 66, 100, 255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer, 7, 52, 80, 255);
  for (int i = 0; i <= WINDOW_WIDTH; i += 10) {
    for(int j = 0; j <= WINDOW_HEIGHT; j += 10) {
      SDL_Rect pointRect = { i - POINT_SIZE / 2, j - POINT_SIZE / 2, POINT_SIZE, POINT_SIZE };
      SDL_RenderFillRect(pGame->pRenderer, &pointRect);
    }
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
  Mix_FreeChunk(pGame->hitItem);
  Mix_FreeChunk(pGame->choiceSound);
  Mix_FreeChunk(pGame->clickButton);
  Mix_CloseAudio();

  SDLNet_Quit();
  TTF_Quit(); 
  SDL_Quit();

}