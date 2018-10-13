#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ilomusi.h"
#include "gameObject.h"

#define FRAME_PERIOD 100
#define GAME_CHARACTER_INITIAL_Y 20
#define GAME_CHARACTER_Y16_ACCEL 3
#define GAME_SCENE_FALL_RATE 1
#define GAME_SCENE_FALL_INTERVAL_IN_FRAMES 2
#define CHARACTER_JUMP_VELOCITY -32
#define CHARACTER_MOVEMENT_SPEED_16 16
#define CLOUD_SPAWN_MIN_INTERVAL_IN_FRAMES 50
#define CLOUD_SPAWN_MAX_INTERVAL_IN_FRAMES 100

#define GAME_OVER_TEXT "SCORE:%i HI:%i"
#define GAME_OVER_TEXT_X 0
#define GAME_OVER_TEXT_Y GRAPHIC_PIXEL_HEIGHT-8


int characterX16, characterY16, characterYVel16, characterYAccel16;
bool characterLanded;
int score;
bool gameOver;
uint32_t gameFallCounter;
uint32_t nextCloudSpawnTick;
struct GameObject *character;
uint8_t lcdDmaBuffer[GRAPHIC_WIDTH][GRAPHIC_HEIGHT];

void gameInit(void){
	srand(systemGetTick());
	gameObjectInit();
	character = NULL;

	characterYVel16 = 0;
	characterLanded = false;

	gameFallCounter = 0;
	nextCloudSpawnTick = 0;

	score = 0;
	gameOver = false;
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
								+CLOUD_SPAWN_MIN_INTERVAL_IN_FRAMES)*FRAME_PERIOD;
		if(character==NULL){
			character = gameObjectNew(GAME_OBJECT_CHARACTER,
										cloudX+CLOUD_WIDTH/2-CHARACTER_WIDTH/2,
										GAME_CHARACTER_INITIAL_Y);
			characterX16 = character->x*16;
			characterY16 = character->y*16;
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
	characterY16 += characterYVel16;
	character->y = (characterY16+8)/16;
	
	for(size_t i=0; i<GAME_OBJECT_NUM; i++){
		if(gameObjectArray[i].type==GAME_OBJECT_CLOUD){
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
			default:
			break;
		}
	}
	if(gameOver){
		int highScore = 0; //TODO: load highScore from the saved data

		char buf[GRAPHIC_PIXEL_WIDTH/6+1];
		sprintf(buf, GAME_OVER_TEXT, score, highScore);
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
		if(timeElapsed<FRAME_PERIOD)
			systemSleep(FRAME_PERIOD-timeElapsed, false);
		previousTick = systemGetTick();
	}
	return 0;
}
