#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ilomusi.h"
#include "gameObject.h"

#define GAME_CHARACTER_INITIAL_Y GRAPHIC_PIXEL_HEIGHT-CHARACTER_HEIGHT
#define GAME_CHARACTER_Y16_ACCEL 3
#define GAME_SCENE_FALL_RATE 1
#define GAME_SCENE_FALL_INTERVAL_IN_FRAMES 2
#define CHARACTER_JUMP_VELOCITY -40
#define CHARACTER_MOVEMENT_SPEED_16 32
#define CLOUD_SPAWN_MIN_INTERVAL_IN_FRAMES 60
#define CLOUD_SPAWN_MAX_INTERVAL_IN_FRAMES 100
#define MIN_HEIGHT_OF_COLLIDIBLE_OBJECT CLOUD_HEIGHT
#define COIN_SPAWN_CHARACTER_DISTANCE 8

#define GAME_OVER_TEXT "SCORE:%i HI:%i"
#define GAME_OVER_TEXT_X 0
#define GAME_OVER_TEXT_Y GRAPHIC_PIXEL_HEIGHT-8

#define FRAME_RATE_DIFFICULTY_INCREASE_TEXT "GAME SPEED UP"
#define BULLETS_PROJECTILE_DIFFICULTY_INCREASE_TEXT "MORE BULLETS"
#define MINES_PROJECTILE_RATE_DIFFICULTY_INCREASE_TEXT "MORE MINES"
#define PLATFORM_DIFFICULTY_INCREASE_TEXT "MORE CONVEYORS"
#define ENEMY_RATE_DIFFICULTY_INCREASE_TEXT "MORE ENEMIES"
#define MESSAGE_DURATION 3000

#define BULLET_MIN_SPEED 1
#define BULLET_MAX_SPEED 3
#define MINE_MIN_SPEED 2
#define MINE_MAX_SPEED 5
#define CONVEYOR_MIN_SPEED 10
#define CONVEYOR_MAX_SPEED 30
#define ENEMY_SPEED 1
#define ENEMY_SPEED_DIVIDER 3
#define LASER_SPEED 3
#define LASER_Y_OFFSET 4

#define STORAGE_HIGHSCORE_RESOURCE 255 //The resource for storing the game highscore
#define STORAGE_HIGHSCORE_MAGIC_NUMBER 0x55 //Used for checking if the saved game highscore data is initialized

int characterX16, characterX16ConveyorVel, characterY16, characterYVel16, characterYAccel16;
bool characterLanded, characterFacingRight;
bool gameOver;
uint32_t gameFallCounter;
uint32_t gameEnemyMovementCounter;
uint32_t eightFramesImageCounter;
uint32_t twoFramesImageCounter;
uint32_t nextCloudSpawnTick;
uint32_t nextBulletSpawnTick;
uint32_t nextMineSpawnTick;
uint32_t nextEnemySpawnTick;
uint32_t messageEndTick;
char message[GRAPHIC_PIXEL_WIDTH/6+1];

//Game difficulty tables
int FRAME_RATE_TABLE[] = {100, 90, 81, 72, 65, 59, 53, 47, 43, 38, 34, 31, 28, 25, 22, 20, 18, 16, 15, 13}; //100*0.9**i
int BULLET_SPAWN_RATE_TABLE[] = {-1, 1700, 1444, 1228, 1044, 887, 754, 641, 544, 463, 393, 334, 284, 241, 205, 174, 148, 126, 107, 91}; //1000*0.85**i
int MINE_SPAWN_RATE_TABLE[] = {-1, 180, 162, 145, 131, 118, 106, 95, 86, 77, 69, 62, 56, 50, 45, 41, 37, 33, 30, 27}; //200*0.9**i
int CONVEYOR_RATE_TABLE[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95}; //i*20
int ENEMY_SPAWN_RATE_TABLE[] = {-1, 900, 810, 729, 656, 590, 531, 478, 430, 387, 348, 313, 282, 254, 228, 205, 185, 166, 150, 135}; //1000*0.9**i

//Game difficulty variables
unsigned int frameRateLevel;
unsigned int bulletSpawnRateLevel;
unsigned int mineSpawnRateLevel;
unsigned int platformDifficultyLevel;
unsigned int enemySpawnRateLevel;
#define MAX_LEVEL_EACH 20
#define MAX_TOTAL_LEVEL MAX_LEVEL_EACH*5

struct GameObject *character;
struct GameObject *laser;

#define CHARACTER_GRAPHIC_RESOURCE 1
#define CLOUD_GRAPHIC_RESOURCE 2
#define COIN_GRAPHIC_RESOURCE 3
#define BULLET_GRAPHIC_RESOURCE 4
#define MINE_GRAPHIC_RESOURCE 5
#define ENEMY_GRAPHIC_RESOURCE 6

#define CHARACTER_GRAPHIC_BUFFER_SIZE 9
#define CLOUD_OR_CONVERYOR_GRAPHIC_BUFFER_SIZE 34
#define COIN_GRAPHIC_BUFFER_SIZE 10
#define BULLET_GRAPHIC_BUFFER_SIZE 12
#define MINE_GRAPHIC_BUFFER_SIZE 7
#define ENEMY_GRAPHIC_BUFFER_SIZE 26

struct GraphicImage characterImageL[2];
struct GraphicImage characterImageR[2];
struct GraphicImage cloudImage;
struct GraphicImage conveyorImageL;
struct GraphicImage conveyorImageR;
struct GraphicImage coinImage[8];
struct GraphicImage bulletImageL;
struct GraphicImage bulletImageR;
struct GraphicImage mineImage[2];
struct GraphicImage enemyImage[2];

uint8_t characterGraphicBuffer[CHARACTER_GRAPHIC_BUFFER_SIZE*4];
uint8_t cloudAndConveyorGraphicBuffer[CLOUD_OR_CONVERYOR_GRAPHIC_BUFFER_SIZE*3];
uint8_t coinGraphicBuffer[COIN_GRAPHIC_BUFFER_SIZE*8];
uint8_t bulletGraphicBuffer[BULLET_GRAPHIC_BUFFER_SIZE*2];
uint8_t mineGraphicBuffer[MINE_GRAPHIC_BUFFER_SIZE*2];
uint8_t enemyGraphicBuffer[ENEMY_GRAPHIC_BUFFER_SIZE*2];

uint8_t lcdDmaBuffer[GRAPHIC_WIDTH][GRAPHIC_HEIGHT];

#define AABB(ax,ay,aw,ah,bx,by,bw,bh) (ax+aw > bx && ax < bx+bw && ay+ah > by && ay < by+bh)

void loadGraphic(void){
	//Let's assume that the graphic will be loaded successfully. That'd make our code much simpler

	//Load image data from SD card to buffer
	storageRead(CHARACTER_GRAPHIC_RESOURCE, 0, characterGraphicBuffer, sizeof(characterGraphicBuffer));
	storageRead(CLOUD_GRAPHIC_RESOURCE, 0, cloudAndConveyorGraphicBuffer, sizeof(cloudAndConveyorGraphicBuffer));
	storageRead(COIN_GRAPHIC_RESOURCE, 0, coinGraphicBuffer, sizeof(coinGraphicBuffer));
	storageRead(BULLET_GRAPHIC_RESOURCE, 0, bulletGraphicBuffer, sizeof(bulletGraphicBuffer));
	storageRead(MINE_GRAPHIC_RESOURCE, 0, mineGraphicBuffer, sizeof(mineGraphicBuffer));
	storageRead(ENEMY_GRAPHIC_RESOURCE, 0, enemyGraphicBuffer, sizeof(enemyGraphicBuffer));

	//Construct struct GraphicImage based on the data loaded into the buffer
	graphicLoadImage(&characterGraphicBuffer[CHARACTER_GRAPHIC_BUFFER_SIZE*0], &characterImageL[0]);
	graphicLoadImage(&characterGraphicBuffer[CHARACTER_GRAPHIC_BUFFER_SIZE*1], &characterImageL[1]);
	graphicLoadImage(&characterGraphicBuffer[CHARACTER_GRAPHIC_BUFFER_SIZE*2], &characterImageR[0]);
	graphicLoadImage(&characterGraphicBuffer[CHARACTER_GRAPHIC_BUFFER_SIZE*3], &characterImageR[1]);

	graphicLoadImage(&cloudAndConveyorGraphicBuffer[CLOUD_OR_CONVERYOR_GRAPHIC_BUFFER_SIZE*0], &cloudImage);
	graphicLoadImage(&cloudAndConveyorGraphicBuffer[CLOUD_OR_CONVERYOR_GRAPHIC_BUFFER_SIZE*1], &conveyorImageL);
	graphicLoadImage(&cloudAndConveyorGraphicBuffer[CLOUD_OR_CONVERYOR_GRAPHIC_BUFFER_SIZE*2], &conveyorImageR);

	for(size_t i=0; i<8; i++)
		graphicLoadImage(&coinGraphicBuffer[COIN_GRAPHIC_BUFFER_SIZE*i], &coinImage[i]);

	graphicLoadImage(&bulletGraphicBuffer[BULLET_GRAPHIC_BUFFER_SIZE*0], &bulletImageL);
	graphicLoadImage(&bulletGraphicBuffer[BULLET_GRAPHIC_BUFFER_SIZE*1], &bulletImageR);

	graphicLoadImage(&mineGraphicBuffer[MINE_GRAPHIC_BUFFER_SIZE*0], &mineImage[0]);
	graphicLoadImage(&mineGraphicBuffer[MINE_GRAPHIC_BUFFER_SIZE*1], &mineImage[1]);

	graphicLoadImage(&enemyGraphicBuffer[ENEMY_GRAPHIC_BUFFER_SIZE*0], &enemyImage[0]);
	graphicLoadImage(&enemyGraphicBuffer[ENEMY_GRAPHIC_BUFFER_SIZE*1], &enemyImage[1]);
}


void gameInit(void){
	srand(systemGetTick());
	gameObjectInit();
	character = NULL;
	laser = NULL;

	characterX16ConveyorVel = 0;
	characterYVel16 = 0;
	characterLanded = false;
	characterFacingRight = true;

	gameFallCounter = 0;
	gameEnemyMovementCounter = 0;
	eightFramesImageCounter = 0;
	twoFramesImageCounter = 0;
	nextCloudSpawnTick = 0;
	nextBulletSpawnTick = UINT32_MAX;
	nextMineSpawnTick = UINT32_MAX;
	nextEnemySpawnTick = UINT32_MAX;

	frameRateLevel = 0;
	bulletSpawnRateLevel = 0;
	mineSpawnRateLevel = 0;
	platformDifficultyLevel = 0;
	enemySpawnRateLevel = 0;
	messageEndTick = 0;

	gameOver = false;

}

void spawnCoin(void){
	int x, y;
	do{ //Keep randomly picking a position until we find the one that isn't close with the character
		x = rand()%(GRAPHIC_PIXEL_WIDTH-COIN_WIDTH);
		//Double the -COIN_HEIGHT to prevent the coin from getting to bottom, making it difficult to get collected
		y = rand()%(GRAPHIC_PIXEL_HEIGHT-COIN_HEIGHT*2);
	}while(AABB(x,y,COIN_WIDTH,COIN_HEIGHT,
		character->x-COIN_SPAWN_CHARACTER_DISTANCE, character->y-COIN_SPAWN_CHARACTER_DISTANCE,
		CHARACTER_WIDTH+COIN_SPAWN_CHARACTER_DISTANCE*2, CHARACTER_HEIGHT+COIN_SPAWN_CHARACTER_DISTANCE*2));
	gameObjectNew(GAME_OBJECT_COIN, x, y);
}

void spawnBullet(void){
	int y = rand()%(GRAPHIC_PIXEL_HEIGHT-BULLET_HEIGHT);
	int8_t speed = rand()%(BULLET_MAX_SPEED-BULLET_MIN_SPEED)+BULLET_MIN_SPEED;
	if(rand()%2)
		speed = -speed;
	struct GameObject *bullet;
	if(speed<0)
		bullet = gameObjectNew(GAME_OBJECT_BULLET, GRAPHIC_PIXEL_WIDTH-1, y); //GRAPHIC_PIXEL_WIDTH=128 would be an overflow
	else
		bullet = gameObjectNew(GAME_OBJECT_BULLET, -BULLET_WIDTH, y);
	bullet->extra = speed;
	//Set the time tick to spawn the next cloud
	nextBulletSpawnTick = systemGetTick()
							+(rand()%(BULLET_SPAWN_RATE_TABLE[bulletSpawnRateLevel]/2)
							+BULLET_SPAWN_RATE_TABLE[bulletSpawnRateLevel]/2)*FRAME_RATE_TABLE[frameRateLevel];
}

void spawnMine(void){
	int x = rand()%(GRAPHIC_PIXEL_WIDTH-MINE_WIDTH);
	int8_t speed = rand()%(MINE_MAX_SPEED-MINE_MIN_SPEED)+MINE_MIN_SPEED;
	struct GameObject *mine;
	mine = gameObjectNew(GAME_OBJECT_MINE, x, GRAPHIC_PIXEL_HEIGHT);
	mine->extra = speed;
	//Set the time tick to spawn the next cloud
	nextMineSpawnTick = systemGetTick()
							+(rand()%(MINE_SPAWN_RATE_TABLE[mineSpawnRateLevel]/2)
							+MINE_SPAWN_RATE_TABLE[mineSpawnRateLevel]/2)*FRAME_RATE_TABLE[frameRateLevel];
}

void spawnEnemy(void){
	if(rand()%2)
		gameObjectNew(GAME_OBJECT_ENEMY, GRAPHIC_PIXEL_WIDTH-1, 0); //The y will be calaculated every frame
	else
		gameObjectNew(GAME_OBJECT_ENEMY, -ENEMY_WIDTH, 0); //The y will be calaculated every frame

	//Set the time tick to spawn the next cloud
	nextEnemySpawnTick = systemGetTick()
							+(rand()%(ENEMY_SPAWN_RATE_TABLE[enemySpawnRateLevel]/2)
							+ENEMY_SPAWN_RATE_TABLE[enemySpawnRateLevel]/2)*FRAME_RATE_TABLE[frameRateLevel];
}

void spawnLaser(void){
	if(laser == NULL){
		if(characterFacingRight){
			laser = gameObjectNew(GAME_OBJECT_LASER, character->x+CHARACTER_WIDTH-LASER_WIDTH, character->y+LASER_Y_OFFSET);
			laser->extra = LASER_SPEED;
		}else{
			laser = gameObjectNew(GAME_OBJECT_LASER, character->x+LASER_WIDTH, character->y+LASER_Y_OFFSET);
			laser->extra = -LASER_SPEED;
		}
	}
}

int getTotalLevel(){
	return frameRateLevel+bulletSpawnRateLevel+mineSpawnRateLevel+platformDifficultyLevel+enemySpawnRateLevel;
}

void increasesDifficulty(void){
	bool difficultyIncreased = false;
	do{
		int type = rand()%5;
		if(type==0 && frameRateLevel<MAX_LEVEL_EACH){
			frameRateLevel++;
			strcpy(message, FRAME_RATE_DIFFICULTY_INCREASE_TEXT);
			difficultyIncreased = true;
		}else if(type==1 && bulletSpawnRateLevel<MAX_LEVEL_EACH){
			bulletSpawnRateLevel++;
			spawnBullet();
			strcpy(message, BULLETS_PROJECTILE_DIFFICULTY_INCREASE_TEXT);
			difficultyIncreased = true;
		}else if(type==2 && mineSpawnRateLevel<MAX_LEVEL_EACH){
			mineSpawnRateLevel++;
			spawnMine();
			strcpy(message, MINES_PROJECTILE_RATE_DIFFICULTY_INCREASE_TEXT);
			difficultyIncreased = true;
		}else if(type==3 && platformDifficultyLevel<MAX_LEVEL_EACH){
			platformDifficultyLevel++;
			strcpy(message, PLATFORM_DIFFICULTY_INCREASE_TEXT);
			difficultyIncreased = true;
		}else if(type==4 && enemySpawnRateLevel<MAX_LEVEL_EACH){
			enemySpawnRateLevel++;
			spawnEnemy();
			strcpy(message, ENEMY_RATE_DIFFICULTY_INCREASE_TEXT);
			difficultyIncreased = true;
		}
	}while(!difficultyIncreased);
	messageEndTick = systemGetTick()+MESSAGE_DURATION;
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
		if((state&KEYS_1)!=0 && (justChanged&KEYS_1)!=0)
			spawnLaser();

		//Check which side the character is facing
		if((state&KEYS_RIGHT)!=0 && (state&KEYS_LEFT)==0)
			characterFacingRight = true;
		else if((state&KEYS_LEFT)!=0 && (state&KEYS_RIGHT)==0)
			characterFacingRight = false;
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
		struct GameObject *cloud = gameObjectNew(GAME_OBJECT_CLOUD, cloudX, 64);
		//Set the time tick to spawn the next cloud
		nextCloudSpawnTick = systemGetTick()
								+(rand()%(CLOUD_SPAWN_MAX_INTERVAL_IN_FRAMES-CLOUD_SPAWN_MIN_INTERVAL_IN_FRAMES)
								+CLOUD_SPAWN_MIN_INTERVAL_IN_FRAMES)*FRAME_RATE_TABLE[frameRateLevel];

		if(rand()%100 < CONVEYOR_RATE_TABLE[platformDifficultyLevel]){
			int8_t speed = rand()%(CONVEYOR_MAX_SPEED-CONVEYOR_MIN_SPEED)+CONVEYOR_MIN_SPEED;
			if(rand()%2)
				speed = -speed;
			cloud->extra = speed;
		}else{
			cloud->extra = 0;
		}
		//Also spawn the character at the beginning of the game
		if(character==NULL){
			character = gameObjectNew(GAME_OBJECT_CHARACTER,
										cloudX+CLOUD_WIDTH/2-CHARACTER_WIDTH/2,
										GAME_CHARACTER_INITIAL_Y);
			characterX16 = character->x*16;
			characterY16 = character->y*16;
			spawnCoin();
		}
	}
	
	if(systemGetTick()>=nextBulletSpawnTick)
		spawnBullet();

	if(systemGetTick()>=nextMineSpawnTick)
		spawnMine();

	if(systemGetTick()>=nextEnemySpawnTick)
		spawnEnemy();

	if(gameFallCounter>=GAME_SCENE_FALL_INTERVAL_IN_FRAMES)
		gameFallCounter = 0;


	if(gameEnemyMovementCounter>=ENEMY_SPEED_DIVIDER)
		gameEnemyMovementCounter = 0;

	for(size_t i=0; i<GAME_OBJECT_NUM; i++){
		switch(gameObjectArray[i].type){
			//Process cloud movement
			case GAME_OBJECT_CLOUD:
				if(gameFallCounter == 0){
					//Move the each cloud up
					gameObjectArray[i].y -= GAME_SCENE_FALL_RATE;
					//If the cloud had rised to too high and disappeared from the screen, delete it.
					if(gameObjectArray[i].y+CLOUD_HEIGHT<0)
						gameObjectDelete(&gameObjectArray[i]);
					gameFallCounter = 0;
				}
			break;
			//Process bullet movement
			case GAME_OBJECT_BULLET:
				gameObjectArray[i].x += gameObjectArray[i].extra;
				if(gameObjectArray[i].x+BULLET_WIDTH<0 || gameObjectArray[i].x >= GRAPHIC_PIXEL_WIDTH)
					gameObjectDelete(&gameObjectArray[i]);
			break;
			//Process mine movement
			case GAME_OBJECT_MINE:
				gameObjectArray[i].y -= gameObjectArray[i].extra;
				if(gameObjectArray[i].y+MINE_HEIGHT<0)
					gameObjectDelete(&gameObjectArray[i]);
			break;
			//Process enemy movement
			case GAME_OBJECT_ENEMY:
			{
				int diff = (gameObjectArray[i].x+ENEMY_WIDTH/2)-(character->x+CHARACTER_WIDTH/2);
				if(gameEnemyMovementCounter==0){
					if(diff < 0){ //Enemy on the left
						gameObjectArray[i].x += ENEMY_SPEED;
						gameObjectArray[i].extra = true;
					}else{ //Enemy on the right
						gameObjectArray[i].x -= ENEMY_SPEED;
						gameObjectArray[i].extra = false;
					}
				}
				if(abs(diff)>CHARACTER_WIDTH)
					gameObjectArray[i].y = character->y-ENEMY_HEIGHT;
				else
					gameObjectArray[i].y = character->y-ENEMY_HEIGHT+(CHARACTER_WIDTH-abs(diff));
			}
			break;
			//Process laser movement
			case GAME_OBJECT_LASER:
				gameObjectArray[i].x += gameObjectArray[i].extra;
				if(gameObjectArray[i].x+LASER_WIDTH<0 || gameObjectArray[i].x >= GRAPHIC_PIXEL_WIDTH){
					gameObjectDelete(&gameObjectArray[i]);
					laser = NULL;
				}
			default:
			break;
		}
	}
	gameFallCounter++;
	gameEnemyMovementCounter++;

	//Process character movement: check if the character is landed
	if(characterYVel16>=GAME_CHARACTER_Y16_ACCEL*2)
		characterLanded = false;
	if(!characterLanded)
		characterX16ConveyorVel = 0;
	////Process character movement: calculate X and Y position and velocity
	characterX16 += characterX16ConveyorVel;
	character->x = (characterX16+8)/16;
	characterYVel16 += GAME_CHARACTER_Y16_ACCEL;
	if(characterYVel16>(MIN_HEIGHT_OF_COLLIDIBLE_OBJECT-2)*16) //Limits the max fall velocity to avoid breaking collision detection
		characterYVel16 = (MIN_HEIGHT_OF_COLLIDIBLE_OBJECT-2)*16;
	characterY16 += characterYVel16;
	character->y = (characterY16+8)/16;

	//Process collision handling
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
					characterX16ConveyorVel = cloud->extra;
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
			case GAME_OBJECT_BULLET:
				if(AABB(character->x, character->y, CHARACTER_WIDTH, CHARACTER_HEIGHT,
				gameObjectArray[i].x, gameObjectArray[i].y, BULLET_WIDTH, BULLET_HEIGHT)){
					gameOver = true;
				}
			break;
			case GAME_OBJECT_MINE:
				if(AABB(character->x, character->y, CHARACTER_WIDTH, CHARACTER_HEIGHT,
				gameObjectArray[i].x, gameObjectArray[i].y, MINE_WIDTH, MINE_HEIGHT)){
					gameOver = true;
				}
			break;
			case GAME_OBJECT_ENEMY:
				if(AABB(character->x, character->y, CHARACTER_WIDTH, CHARACTER_HEIGHT,
				gameObjectArray[i].x, gameObjectArray[i].y, ENEMY_WIDTH, ENEMY_HEIGHT)){
					gameOver = true;
				}

				if(laser != NULL &&
				AABB(laser->x, laser->y, LASER_WIDTH, LASER_HEIGHT,
				gameObjectArray[i].x, gameObjectArray[i].y, ENEMY_WIDTH, ENEMY_HEIGHT)){
					gameObjectDelete(&gameObjectArray[i]);
					gameObjectDelete(laser);
					laser = NULL;
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
	eightFramesImageCounter++;
	twoFramesImageCounter++;
	if(eightFramesImageCounter>=8)
		eightFramesImageCounter = 0;
	if(twoFramesImageCounter>=2)
		twoFramesImageCounter = 0;
	for(size_t i=0; i<GAME_OBJECT_NUM; i++){
		switch(gameObjectArray[i].type){
			case GAME_OBJECT_CHARACTER:
				if(characterFacingRight)
					graphicDrawImage(&characterImageR[twoFramesImageCounter],
										gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
				else
					graphicDrawImage(&characterImageL[twoFramesImageCounter],
										gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
			break;
			case GAME_OBJECT_CLOUD:
				if(gameObjectArray[i].extra>0){ // >>>>>> right conveyor
					graphicDrawImage(&conveyorImageR, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
				}else if(gameObjectArray[i].extra<0){ // <<<<<< left conveyor
					graphicDrawImage(&conveyorImageL, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
				}else{ //stationary cloud
					graphicDrawImage(&cloudImage, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
				}
			break;
			case GAME_OBJECT_COIN:
				graphicDrawImage(&coinImage[eightFramesImageCounter],
									gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
			break;
			case GAME_OBJECT_BULLET:
				if(gameObjectArray[i].extra>0)
					graphicDrawImage(&bulletImageR, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
				else
					graphicDrawImage(&bulletImageL, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
			break;
			case GAME_OBJECT_MINE:
				graphicDrawImage(&mineImage[twoFramesImageCounter],
									gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
			break;
			case GAME_OBJECT_ENEMY:
				graphicDrawImage(&enemyImage[twoFramesImageCounter],
									gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_FOREGROUND_OR);
			break;
			case GAME_OBJECT_LASER:
			{
				struct GraphicImage graphicImage = {
					.image = GRAPHIC_SHAPE_RECTANGLE,
					.width = LASER_WIDTH,
					.height = LASER_HEIGHT,
				};
				graphicDrawImage(&graphicImage, gameObjectArray[i].x, gameObjectArray[i].y, GRAPHIC_MODE_BACKGROUND_OR);
			}
			break;
			default:
			break;
		}
	}
	if(gameOver){
		int score = getTotalLevel();
		int highScore = 0;
		uint8_t magicNumber = 0;

		//Read the highscore. If the current highscore is larger than the old one, also save it.
		{
			char buf[sizeof(highScore)+sizeof(magicNumber)];
			int len = storageRead(STORAGE_HIGHSCORE_RESOURCE, 0, buf, sizeof(buf));
			if(len==sizeof(buf)){
				memcpy(&magicNumber, &buf[4], sizeof(magicNumber));
				if(magicNumber==STORAGE_HIGHSCORE_MAGIC_NUMBER)
					memcpy(&highScore, &buf[0], sizeof(highScore));
				if(score>highScore){ //Highscore achieved. Saving.
					magicNumber = STORAGE_HIGHSCORE_MAGIC_NUMBER;
					memcpy(&buf[0], &score, sizeof(highScore)); //Save the current score instead of the previous highscore
					memcpy(&buf[4], &magicNumber, sizeof(magicNumber));
					storageWrite(STORAGE_HIGHSCORE_RESOURCE, 0, buf, sizeof(buf)); //We just assume that the save is a success :p
				}
			}
		}

		char buf[GRAPHIC_PIXEL_WIDTH/6+1];
		sprintf(buf, GAME_OVER_TEXT, score, highScore);
		graphicDrawText(buf, 0, GAME_OVER_TEXT_X, GAME_OVER_TEXT_Y, GRAPHIC_PIXEL_WIDTH, 8, GRAPHIC_MODE_FOREGROUND_AND_NOT|GRAPHIC_MODE_BACKGROUND_OR);
	}else{
		if(systemGetTick()<messageEndTick){
			graphicDrawText(message, 0, GRAPHIC_PIXEL_WIDTH-strlen(message)*6, GAME_OVER_TEXT_Y, GRAPHIC_PIXEL_WIDTH, 8, GRAPHIC_MODE_FOREGROUND_OR);
		}
	}
}

int main(void){
	//Game initialization
	loadGraphic();

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
