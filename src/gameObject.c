#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "gameObject.h"

struct GameObject gameObjectArray[GAME_OBJECT_NUM];

void gameObjectInit(void){
	for(size_t i=0; i<GAME_OBJECT_NUM; i++)
		gameObjectArray[i].type = NO_OBJECT;
}

struct GameObject* gameObjectNew(enum GameObjectType objectType, int8_t x, int8_t y){
	for(size_t i=0; i<GAME_OBJECT_NUM; i++){
		if(gameObjectArray[i].type == NO_OBJECT){
			gameObjectArray[i].type = objectType;
			gameObjectArray[i].x = x;
			gameObjectArray[i].y = y;
			return &gameObjectArray[i];
		}
	}
	return NULL;
}

void gameObjectDelete(struct GameObject* gameObject){
	gameObject->type = NO_OBJECT;
}
