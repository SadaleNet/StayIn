#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ilomusi.h"
#include "gameObject.h"

#define GAME_CHARACTER_INITIAL_Y GRAPHIC_PIXEL_HEIGHT-CHARACTER_HEIGHT
#define GAME_CHARACTER_Y16_ACCEL 3
#define GAME_SCENE_FALL_RATE 1
#define GAME_SCENE_FALL_INTERVAL_IN_FRAMES 2
#define CHARACTER_JUMP_VELOCITY -40
#define CHARACTER_MOVEMENT_SPEED_16 32
#define CLOUD_SPAWN_MIN_INTERVAL_IN_FRAMES 50
#define CLOUD_SPAWN_MAX_INTERVAL_IN_FRAMES 100
#define MIN_HEIGHT_OF_COLLIDIBLE_OBJECT CLOUD_HEIGHT
#define COIN_SPAWN_CHARACTER_DISTANCE 8

#define GAME_OVER_TEXT "SCORE:%i HI:%i"
#define GAME_OVER_TEXT_X 0
#define GAME_OVER_TEXT_Y GRAPHIC_PIXEL_HEIGHT-8


int characterX16, characterY16, characterYVel16, characterYAccel16;
bool characterLanded;
bool gameOver;
uint32_t gameFallCounter;
uint32_t nextCloudSpawnTick;

//Game difficulty tables
int FRAME_RATE_TABLE[] = {100, 90, 81, 72, 65, 59, 53, 47, 43, 38, 34, 31, 28, 25, 22, 20, 18, 16, 15, 13};

//Game difficulty variables
unsigned int frameRateLevel;
unsigned int horizontalProjectileLevel;
unsigned int verticalProjectileLevel;
unsigned int platformXAxisMovementLevel;
unsigned int enemySpawnRateLevel;
#define MAX_LEVEL_EACH 1
#define MAX_TOTAL_LEVEL MAX_LEVEL_EACH*5

struct GameObject *character;
uint8_t lcdDmaBuffer[GRAPHIC_WIDTH][GRAPHIC_HEIGHT];

#define AABB(ax,ay,aw,ah,bx,by,bw,bh) (ax+aw > bx && ax < bx+bw && ay+ah > by && ay < by+bh)

void spawnCoin(void);

void gameInit(void){
	srand(systemGetTick());
	gameObjectInit();
	character = NULL;

	characterYVel16 = 0;
	characterLanded = false;

	gameFallCounter = 0;
	nextCloudSpawnTick = 0;

	frameRateLevel = 0;
	horizontalProjectileLevel = 0;
	verticalProjectileLevel = 0;
	platformXAxisMovementLevel = 0;
	enemySpawnRateLevel = 0;

	gameOver = false;

}

void spawnCoin(void){
	int x, y;
	do{ //Keep randomly picking a position until we find the one that isn't close with the character
		x = rand()%(GRAPHIC_PIXEL_WIDTH-COIN_WIDTH);
		y = rand()%(GRAPHIC_PIXEL_HEIGHT-COIN_HEIGHT);
	}while(AABB(x,y,COIN_WIDTH,COIN_HEIGHT,
		character->x-COIN_SPAWN_CHARACTER_DISTANCE, character->y-COIN_SPAWN_CHARACTER_DISTANCE,
		CHARACTER_WIDTH+COIN_SPAWN_CHARACTER_DISTANCE*2, CHARACTER_HEIGHT+COIN_SPAWN_CHARACTER_DISTANCE*2));
	gameObjectNew(GAME_OBJECT_COIN, x, y);
}

int getTotalLevel(){
	return frameRateLevel+horizontalProjectileLevel+verticalProjectileLevel+platformXAxisMovementLevel+enemySpawnRateLevel;
}

void increasesDifficulty(void){
	bool difficultyIncreased = false;
	do{
		int type = rand()%5;
		if(type==0 && frameRateLevel<MAX_LEVEL_EACH){
			frameRateLevel++;
			difficultyIncreased = true;
		}else if(type==1 && horizontalProjectileLevel<MAX_LEVEL_EACH){
			horizontalProjectileLevel++;
			difficultyIncreased = true;
		}else if(type==2 && verticalProjectileLevel<MAX_LEVEL_EACH){
			verticalProjectileLevel++;
			difficultyIncreased = true;
		}else if(type==3 && platformXAxisMovementLevel<MAX_LEVEL_EACH){
			platformXAxisMovementLevel++;
			difficultyIncreased = true;
		}else if(type==4 && enemySpawnRateLevel<MAX_LEVEL_EACH){
			enemySpawnRateLevel++;
			difficultyIncreased = true;
		}
	}while(!difficultyIncreased);
}

void handleKeyInput(void){
	uint8_t justChanged = keysGetJustChangedState();
	uint8_t state = keysGetPressedState();
	if(!gameOver){
		if((state&KEYS_LEFT)!=0)
			characterX16 -= CHARACTER_MOVEMENT_SPEED_16;
		if((state&KEYS_RIGHT)!=0)
			characterX16 += CHARACTER_MOVEMENT_SPEED_16;
		if((state&KEYS_UP)!=0 && (justChanged&KEYS_UP)!=0){
			if(characterLanded){
				characterYVel16 = CHARACTER_JUMP_VELOCITY;
				characterLanded = false;
			}
		}
	}else{
		//Button to restart the game
		if((state&KEYS_2)!=0 && (justChanged&KEYS_2)!=0)
			gameInit();
	}
}

void processGameLogic(void){
	//Spawn new cloud
	if(systemGetTick()>=nextCloudSpawnTick){
		int cloudX = rand()%(GRAPHIC_PIXEL_WIDTH-CLOUD_WIDTH);
		gameObjectNew(GAME_OBJECT_CLOUD, cloudX, 64);
		//Set the time tick to spawn the next cloud
		nextCloudSpawnTick = systemGetTick()
								+(rand()%(CLOUD_SPAWN_MAX_INTERVAL_IN_FRAMES-CLOUD_SPAWN_MIN_INTERVAL_IN_FRAMES)
								+CLOUD_SPAWN_MIN_INTERVAL_IN_FRAMES)*FRAME_RATE_TABLE[frameRateLevel];
		if(character==NULL){
			character = gameObjectNew(GAME_OBJECT_CHARACTER,
										cloudX+CLOUD_WIDTH/2-CHARACTER_WIDTH/2,
										GAME_CHARACTER_INITIAL_Y);
			characterX16 = character->x*16;
			characterY16 = character->y*16;
			spawnCoin();
		}
	}


	//Process cloud movement
	if(gameFallCounter>=GAME_SCENE_FALL_INTERVAL_IN_FRAMES){
		for(size_t i=0; i<GAME_OBJECT_NUM; i++){
			if(gameObjectArray[i].type==GAME_OBJECT_CLOUD){
				//Move the each cloud up
				gameObjectArray[i].y -= GAME_SCENE_FALL_RATE;
				//If the cloud had rised to too high and disappeared from the screen, delete it.
				if(gameObjectArray[i].y+CLOUD_HEIGHT<0)
					gameObjectDelete(&gameObjectArray[i]);
			}
		}
		gameFallCounter = 0;
	}
	gameFallCounter++;
	//Process character movement
	if(characterYVel16>=GAME_CHARACTER_Y16_ACCEL)
		characterLanded = false;
	character->x = (characterX16+8)/16;
	characterYVel16 += GAME_CHARACTER_Y16_ACCEL;
	if(characterYVel16>(MIN_HEIGHT_OF_COLLIDIBLE_OBJECT-1)*16) //Limits the max fall velocity to avoid breaking collision detection
		characterYVel16 = (MIN_HEIGHT_OF_COLLIDIBLE_OBJECT-1)*16;
	characterY16 += characterYVel16;
	character->y = (characterY16+8)/16;
	
	for(size_t i=0; i<GAME_OBJECT_NUM; i++){
		switch(gameObjectArray[i].type){
			case GAME_OBJECT_CLOUD:
			{
				struct GameObject *cloud = &gameObjectArray[i];
				/* This is a modified AABB algorithm.
				 * The X-axis collision detection is same as AABB algorithm
				 * However, for a Y-axis, it's designed in a way that the collision is
				 * detected only if the bottom of the character (character->y+CHARACTER_HEIGHT) is
				 * higher than the bottom of the cloud (cloud->y+CLOUD_HEIGHT).
				 *
				 * Reasoning: If the feet of the character is higher than the bottom of the cloud,
				 * the character would climb the cloud and reaches its top.
				 * However, if its feet is below the bottom of the cloud, there's no way that he
				 * could get to the top of the cloud. Hence the fall and there's no "collision" detected.
				 */
				if(character->x+CHARACTER_WIDTH > cloud->x && character->x < cloud->x+CLOUD_WIDTH &&
					character->y+CHARACTER_HEIGHT > cloud->y && character->y+CHARACTER_HEIGHT < cloud->y+CLOUD_HEIGHT){
					character->y = cloud->y-CHARACTER_HEIGHT;
					characterY16 = character->y*16;
					characterYVel16 = 0;
					characterLanded = true;
				}
			}
			break;
			case GAME_OBJECT_COIN:
				if(AABB(character->x, character->y, CHARACTER_WIDTH, CHARACTER_HEIGHT,
				gameObjectArray[i].x, gameObjectArray[i].y, COIN_WIDTH, COIN_HEIGHT)){
					gameObjectDelete(&gameObjectArray[i]);
					increasesDifficulty();
					if(getTotalLevel()<MAX_TOTAL_LEVEL){
						spawnCoin();
					}
				}
			break;
			default:
			break;
		}
	}

	if(character->x+CHARACTER_WIDTH<0 || character->x>GRAPHIC_PIXEL_WIDTH ||
		character->y+CHARACTER_HEIGHT<0 || character->y>=GRAPHIC_PIXEL_HEIGHT)
		gameOver = true;
}

void renderGameObjects(void){
	for(size_t i=0; i<GAME_OBJECT_NUM; i++){
		switch(gameObjectArray[i].type){
			case GAME_OBJECT_CHARACTER:
			{
				struct GraphicImage graphicImage = {
					.image = GRAPHIC_SHAPE_RECTANGLE,
					.width = CHARACTER_WIDTH,
					.height = CHARACTER_HEIGHT,
				};
				graphicDrawImage(&graphicImage, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_BACKGROUND_OR);
			}
			break;
			case GAME_OBJECT_CLOUD:
			{
				struct GraphicImage graphicImage = {
					.image = GRAPHIC_SHAPE_RECTANGLE,
					.width = CLOUD_WIDTH,
					.height = CLOUD_HEIGHT,
				};
				graphicDrawImage(&graphicImage, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
			}
			break;
			case GAME_OBJECT_COIN:
			{
				struct GraphicImage graphicImage = {
					.image = GRAPHIC_SHAPE_RECTANGLE,
					.width = COIN_WIDTH,
					.height = COIN_HEIGHT,
				};
				graphicDrawImage(&graphicImage, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
			}
			break;
			default:
			break;
		}
	}
	if(gameOver){
		int highScore = 0; //TODO: load highScore from the saved data

		char buf[GRAPHIC_PIXEL_WIDTH/6+1];
		sprintf(buf, GAME_OVER_TEXT, getTotalLevel(), highScore);
		graphicDrawText(buf, 0, GAME_OVER_TEXT_X, GAME_OVER_TEXT_Y, GRAPHIC_PIXEL_WIDTH, 8, GRAPHIC_MODE_FOREGROUND_AND_NOT|GRAPHIC_MODE_BACKGROUND_OR);
	}
}

int main(void){
	//Game Objects Initialization
	gameInit();

	//Initialize display
	graphicSetDrawBuffer(lcdDmaBuffer);

	uint32_t previousTick = systemGetTick();
	while(true){
		//wait for completion of the previous rendering
		while(!graphicIsDisplayReady()){} //Do not start drawing until the rendering is completed
		graphicClearDisplay(false);

		//Perform these functions every frame
		handleKeyInput();
		if(!gameOver)
			processGameLogic();
		renderGameObjects();
		graphicDisplay(lcdDmaBuffer);

		//Frame limiting
		int32_t timeElapsed = (systemGetTick()-previousTick);
		if(timeElapsed<FRAME_RATE_TABLE[frameRateLevel])
			systemSleep(FRAME_RATE_TABLE[frameRateLevel]-timeElapsed, false);
		previousTick = systemGetTick();
	}
	return 0;
}
