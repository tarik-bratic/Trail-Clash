#ifndef item_h
#define item_h

typedef struct itemImage ItemImage;
typedef struct item Item;

ItemImage *createItemImage(SDL_Renderer *pRenderer);
Item *createItem(ItemImage *pItemImage, int window_width, int window_height, int spawn, int xcoords, int ycoords);
void updateItem(Item *pItem);
void drawItem(Item *pItem);
void destroyItem(Item *pItem);
SDL_Rect getRectItem(Item *pItem);
void destroyItemImage(ItemImage *pItemImage);
int getXcoords(int xcoords, int socx);
int getYcoords(int ycoords, int socy);

#endif