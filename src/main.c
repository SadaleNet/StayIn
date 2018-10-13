#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ilomusi.h"
#include "gameObject.h"

#define FRAME_PERIOD 100
#define GAME_CHARACTER_INITIAL_X (GRAPHIC_WIDTH/2-CHARACTER_WIDTH/2)
#define GAME_CHARACTER_INITIAL_Y 20
#define GAME_CHARACTER_Y16_ACCEL 3
#define GAME_SCENE_FALL_RATE 1
#define GAME_SCENE_FALL_INTERVAL 100
#define CHARACTER_JUMP_VELOCITY -32

int characterX16, characterY16, characterYVel16, characterYAccel16;
struct GameObject *character;
uint8_t lcdDmaBuffer[GRAPHIC_WIDTH][GRAPHIC_HEIGHT];

void gameInit(void){
	gameObjectInit();
	character = gameObjectNew(GAME_OBJECT_CHARACTER, GAME_CHARACTER_INITIAL_X, GAME_CHARACTER_INITIAL_Y);
	gameObjectNew(GAME_OBJECT_CLOUD, GAME_CHARACTER_INITIAL_X-5, 64);

	characterX16 = GAME_CHARACTER_INITIAL_X*16;
	characterY16 = GAME_CHARACTER_INITIAL_Y*16;
	characterYVel16 = 0;
}

void handleKeyInput(void){
	uint8_t justChanged = keysGetJustChangedState();
	uint8_t state = keysGetPressedState();
	if((state&KEYS_LEFT)!=0)
		characterX16 -= 16;
	if((state&KEYS_RIGHT)!=0)
		characterX16 += 16;
	if((state&KEYS_UP)!=0 && (justChanged&KEYS_UP)!=0)
		characterYVel16 = CHARACTER_JUMP_VELOCITY;
}

void processGameLogic(void){
	static uint32_t previousGameFallTick;
	//Process cloud movement
	if(systemGetTick()-previousGameFallTick>GAME_SCENE_FALL_INTERVAL){
		for(size_t i=0; i<GAME_OBJECT_NUM; i++){
			if(gameObjectArray[i].type==GAME_OBJECT_CLOUD)
				gameObjectArray[i].y -= GAME_SCENE_FALL_RATE;
		}
		previousGameFallTick = systemGetTick();
	}
	//Process character movement
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
			 * higher than the bottom of the object (cloud->y+CLOUD_HEIGHT).
			 */
			if(character->x+CHARACTER_WIDTH > cloud->x && character->x < cloud->x+CLOUD_WIDTH &&
				character->y+CHARACTER_HEIGHT > cloud->y && character->y+CHARACTER_HEIGHT < cloud->y+CLOUD_HEIGHT){
				character->y = cloud->y-CHARACTER_HEIGHT;
				characterY16 = character->y*16;
				characterYVel16 = 0;
			}
		}
	}
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
