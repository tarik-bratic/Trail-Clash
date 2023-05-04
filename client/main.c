#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include "../lib/include/data.h"
#include "../lib/include/text.h"
#include "../lib/include/snake.h"
#include "../lib/include/init.h"
#include "../lib/include/item.h"

/* Client Game struct (Snake, UI, Network) */
typedef struct game
{

  // SNAKE
  SDL_Window *pWindow;
  SDL_Renderer *pRenderer;
  Snake *pSnke[MAX_SNKES];
  int num_of_snkes;
  int snkeID;

  // UI
  TTF_Font *pStrdFont, *pTitleFont, *pNetFont;
  Text *pTitleText, *pStartText, *pQuitText, *pWaitingText, *pEnterIpAddrs;

  // NETWORK
  UDPsocket pSocket;
  UDPpacket *pPacket;
  IPaddress serverAdd;

  // ITEM
  ItemImage *pItemImage[MAX_ITEMS];
  Item *pItems[MAX_ITEMS];

  // TIMER
  int startTime;

  GameState state;

} Game;

int init_conn(Game *pGame);
int init_structure(Game *pGame);
int init_allSnakes(Game *pGame);
int init_Items(Game *pGame);

void run(Game *pGame);
void close(Game *pGame);
void render_snake(Game *pGame);
void update_server_data(Game *pGame);
void input_handler(Game *pGame, SDL_Event *pEvent);
int spawnItem(Game *pGame, int nrOfItems);

int main(int argv, char **args)
{

  Game g = {0};

  if (!init_structure(&g))
    return 1;
  run(&g);
  close(&g);

  return 0;
}

/* Initialize structe of the game with SDL libraries and other attributes */
int init_structure(Game *pGame)
{

  srand(time(NULL));

  pGame->state = START;
  pGame->num_of_snkes = MAX_SNKES;

  if (!init_sdl_libraries())
    return 0;

  pGame->pWindow = main_wind("Trail Clash - client", pGame->pWindow);
  if (!pGame->pWindow)
    close(pGame);

  pGame->pRenderer = create_render(pGame->pRenderer, pGame->pWindow);
  if (!pGame->pRenderer)
    close(pGame);

  pGame->pTitleFont = create_font(pGame->pTitleFont, "../lib/resources/chrustyrock.ttf", 100);
  if (!pGame->pTitleFont)
    close(pGame);

  pGame->pStrdFont = create_font(pGame->pStrdFont, "../lib/resources/XBItalic.ttf", 20);
  if (!pGame->pStrdFont)
    close(pGame);

  pGame->pNetFont = create_font(pGame->pNetFont, "../lib/resources/Sigmar-Regular.ttf", 20);
  if (!pGame->pNetFont)
    close(pGame);

  // Create own text with create_text function. Value is stored in a Text pointer.
  pGame->pTitleText = create_text(pGame->pRenderer, 70, 168, 65, pGame->pTitleFont,
                                  "Trail Clash", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

  pGame->pStartText = create_text(pGame->pRenderer, 70, 168, 65, pGame->pStrdFont,
                                  "[SPACE] to join", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 50);

  pGame->pQuitText = create_text(pGame->pRenderer, 70, 168, 65, pGame->pStrdFont,
                                 "Q: Quit Game", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 150);

  pGame->pWaitingText = create_text(pGame->pRenderer, 238, 168, 65, pGame->pStrdFont,
                                    "Waiting for server ...", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100);

  // Checking if there is an error regarding the Text pointer.
  if (!pGame->pStartText || !pGame->pWaitingText || !pGame->pTitleText)
  {
    printf("Error: %s\n", SDL_GetError());
    close(pGame);
    return 0;
  }

  init_allSnakes(pGame);

  init_conn(pGame);

  init_Items(pGame); // seems to be working now?

  return 1;
}

/* Main loop of the game. Updates the snakes attributes. Looking for input. Sends and recives new data. */
void run(Game *pGame)
{

  SDL_Event event;
  ClientData cData;
  int joining = 0;
  int closeRequest = 0;
  int boostKey = 0;
  int nrOfItems = 0;
  int replace;

  while (!closeRequest)
  {

    switch (pGame->state)
    {
    // The game is running
    case RUNNING:
      // Update new recived data
      while (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket))
      {
        update_server_data(pGame);
      }

      // Looking if there is an input
      if (SDL_PollEvent(&event))
      {
        if (event.type == SDL_QUIT)
        {
          closeRequest = 1;
        }
        else
          input_handler(pGame, &event);
      }

    /* Kommentar av Adil: Allt verkar fungera upp till run-funktionen
    behöver endast få allt att funka i run-funktionen i main.c för både 
    client o server */
    
     /* nrOfItems = spawnItem(pGame, nrOfItems);
      //update_snake(pGame->pSnke[i], boostKey); //Behöver man?

      for (int i = 0; i < MAX_ITEMS; i++)
      {
        if (collideSnake(pGame->pSnke[i], getRectItem(pGame->pItems[i])))
        {
          boostKey = 1;
          pGame->startTime = 0;
          updateItem(pGame->pItems[i]);
          nrOfItems--;
          replace = i;
        }
      }

      if (boostKey > 0)
      {
        pGame->startTime++;
        if (pGame->startTime == 200)
        {
          boostKey = 0;
        }
      } */

      // Update snake cord, send data
      for (int i = 0; i < MAX_SNKES; i++)
      {
        // Create an array of pointers to other snakes
        Snake *otherSnakes[MAX_SNKES - 1];
        int otherSnakesIndex = 0;
        // Looping through all the other snakes to add them to the array
        for (int j = 0; j < MAX_SNKES; j++)
        {
          if (j != i)
          {
            otherSnakes[otherSnakesIndex++] = pGame->pSnke[j];
          }
        }

        update_snake(pGame->pSnke[i], otherSnakes, MAX_SNKES - 1, boostKey);
      }

      // Render snake to the window
      render_snake(pGame);
      break;
    // Main Menu
    case START:
      // If you haven't joined display start text if not display waiting text
      if (!joining)
      {
        draw_text(pGame->pTitleText);
        draw_text(pGame->pStartText);
        draw_text(pGame->pQuitText);
      }
      else
      {
        SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
        SDL_RenderClear(pGame->pRenderer);
        SDL_SetRenderDrawColor(pGame->pRenderer, 230, 230, 230, 255);
        draw_text(pGame->pWaitingText);
      }

      // Update screen with new render
      SDL_RenderPresent(pGame->pRenderer);

      // Looking if player has pressed spacebar
      if (SDL_PollEvent(&event))
      {
        if (event.type == SDL_QUIT)
          closeRequest = 1;

        if (!joining && event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_SPACE)
        {
          // Change client data and copy to packet (Space is pressed)
          joining = 1;
          cData.command = READY;
          cData.snkeNumber = -1;
          memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
          pGame->pPacket->len = sizeof(ClientData);
        }
        else if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_Q)
        {
          /* Kommentar av Adil: försökt stänga spelet sådär om man trycker Q,
          det funkar men kommer fram segmentation fault på Msys även om det fungerar som tänkt */
          closeRequest = 1;
          close(pGame);
        }
      }

      // Send client data packet
      if (joining)
      {
        if (!SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket))
        {
          printf("Error (UDP_Send): %s", SDLNet_GetError());
        }
      }

      // Update new recived data
      if (SDLNet_UDP_Recv(pGame->pSocket, pGame->pPacket))
      {
        update_server_data(pGame);
        if (pGame->state == RUNNING)
          joining = 0;
      }
      break;
    }

    // SDL_Delay(ONE_MS); Kanske kan användas senare. Låt stå.
  }

  // When player has closed window or left the game
  cData.command = DISC;
  memcpy(pGame->pPacket->data, &cData, sizeof(ClientData));
  pGame->pPacket->len = sizeof(ClientData);

  if (!SDLNet_UDP_Send(pGame->pSocket, -1, pGame->pPacket))
  {
    printf("Error (UDP_Send): %s", SDLNet_GetError());
  }
}

/* Manage various inputs (keypress, mouse) */
void input_handler(Game *pGame, SDL_Event *pEvent)
{

  if (pEvent->type == SDL_KEYDOWN)
  {

    ClientData cData;
    cData.snkeNumber = pGame->snkeID;

    switch (pEvent->key.keysym.scancode)
    {
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
int init_conn(Game *pGame)
{

  // Enable text input handling
  SDL_StartTextInput();

  char mess[30] = "Enter a server IP address";
  char ipAddr[INPUT_BUFFER_SIZE] = "";
  int input_cursor_position = 0;

  int closeRequest = 1;
  while (closeRequest)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
        closeRequest = 0;
        break;
      case SDL_TEXTINPUT:
        if (input_cursor_position < 15)
        {
          strcat(ipAddr, event.text.text);
          input_cursor_position += strlen(event.text.text);
        }
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_RETURN)
          closeRequest = 0;
        if (event.key.keysym.sym == SDLK_BACKSPACE && input_cursor_position > 0)
        {
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
        input_rect.x + 15, // Rectangle x cord
        input_rect.y + 15, // Rectangle y cord
        input_rect.w - 30, // Height of the rectangle
        input_rect.h - 30  // Width of the rectangle
    };

    SDL_Rect message = {
        text_rect.x = 300, // Rectangle x cord
        text_rect.y = 150, // Rectangle y cord
        text_rect.w = 300, // Height of the rectangle
        text_rect.h = 100  // Width of the rectangle
    };

    SDL_Color text_color = {
        255, 255, 255, 255};

    SDL_Surface *input_text_surface = TTF_RenderText_Solid(pGame->pNetFont, ipAddr, text_color);
    SDL_Surface *message_surface = TTF_RenderText_Solid(pGame->pNetFont, mess, text_color);
    SDL_Texture *input_text_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, input_text_surface);
    SDL_Texture *message_texture = SDL_CreateTextureFromSurface(pGame->pRenderer, message_surface);
    SDL_RenderCopy(pGame->pRenderer, input_text_texture, NULL, &input_text_rect);
    SDL_RenderCopy(pGame->pRenderer, message_texture, NULL, &message);
    SDL_RenderPresent(pGame->pRenderer);
  }

  // Disable text input handling, clear screen
  SDL_StopTextInput();
  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);

  // Establish client to server
  if (!(pGame->pSocket = SDLNet_UDP_Open(0)))
  {
    printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
    return 0;
  }

  if (SDLNet_ResolveHost(&(pGame->serverAdd), ipAddr, 2000))
  {
    printf("SDLNet_ResolveHost (%s: 2000): %s\n", ipAddr, SDLNet_GetError());
    return 0;
  }

  if (!(pGame->pPacket = SDLNet_AllocPacket(512)))
  {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    return 0;
  }

  pGame->pPacket->address.host = pGame->serverAdd.host;
  pGame->pPacket->address.port = pGame->serverAdd.port;

  return 1;
}

/* Copy new data to server data and updates snake data */
void update_server_data(Game *pGame)
{

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
int init_allSnakes(Game *pGame)
{

  for (int i = 0; i < MAX_SNKES; i++)
    pGame->pSnke[i] = create_snake(i, pGame->pRenderer, WINDOW_WIDTH, WINDOW_HEIGHT);

  for (int i = 0; i < MAX_SNKES; i++)
  {
    if (!pGame->pSnke[i])
    {
      printf("Error: %s", SDL_GetError());
      close(pGame);
      return 0;
    }
  }
}

int init_Items(Game *pGame)
{
  // creates Items and creates their image

  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer, 230, 230, 230, 255);

  for (int i = 0; i < MAX_ITEMS; i++)
  {
    pGame->pItemImage[i] = createItemImage(pGame->pRenderer);
    pGame->pItems[i] = createItem(pGame->pItemImage[i], WINDOW_WIDTH, WINDOW_HEIGHT, 1);
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

/* Render and presents a snake to the window */
void render_snake(Game *pGame)
{

  SDL_SetRenderDrawColor(pGame->pRenderer, 0, 0, 0, 255);
  SDL_RenderClear(pGame->pRenderer);
  SDL_SetRenderDrawColor(pGame->pRenderer, 230, 230, 230, 255);
  for (int i = 0; i < MAX_ITEMS; i++)
    drawItem(pGame->pItems[i]);
  for (int i = 0; i < MAX_SNKES; i++)
  {
    draw_snake(pGame->pSnke[i]);
    draw_trail(pGame->pSnke[i]);
  }

  SDL_RenderPresent(pGame->pRenderer);
}

/* Destoryes various SDL libraries and snakes. Safe way when exiting game. */
void close(Game *pGame)
{

  for (int i = 0; i < MAX_SNKES; i++)
    if (pGame->pSnke[i])
      destroy_snake(pGame->pSnke[i]);

  if (pGame->pRenderer)
    SDL_DestroyRenderer(pGame->pRenderer);

  if (pGame->pWindow)
    SDL_DestroyWindow(pGame->pWindow);

  if (pGame->pWaitingText)
    destroy_text(pGame->pWaitingText);

  if (pGame->pTitleText)
    destroy_text(pGame->pTitleText);

  if (pGame->pStartText)
    destroy_text(pGame->pStartText);

  if (pGame->pTitleFont)
    TTF_CloseFont(pGame->pTitleFont);

  if (pGame->pStrdFont)
    TTF_CloseFont(pGame->pStrdFont);

  if (pGame->pNetFont)
    TTF_CloseFont(pGame->pNetFont);

  SDLNet_Quit();
  TTF_Quit();
  SDL_Quit();
}

int spawnItem(Game *pGame, int NrOfItems)
{
  int spawn = rand() % 500;
  if (spawn == 0)
  {
    if (NrOfItems == MAX_ITEMS)
    {
    }
    else
    {
      pGame->pItemImage[NrOfItems] = createItemImage(pGame->pRenderer);
      pGame->pItems[NrOfItems] = createItem(pGame->pItemImage[NrOfItems], WINDOW_WIDTH, WINDOW_HEIGHT, 0);
      NrOfItems++;
    }
  }
  return NrOfItems;
}