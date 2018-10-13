#define GAME_OBJECT_NUM (64U)
#define CHARACTER_WIDTH (8)
#define CHARACTER_HEIGHT (8)
#define COIN_WIDTH (8)
#define COIN_HEIGHT (8)
#define CLOUD_WIDTH (32)
#define CLOUD_HEIGHT (5)
#define BULLET_WIDTH (10)
#define BULLET_HEIGHT (7)
#define MINE_WIDTH (5)
#define MINE_HEIGHT (5)

enum GameObjectType{
	NO_OBJECT,
	GAME_OBJECT_CHARACTER,
	GAME_OBJECT_CLOUD,
	GAME_OBJECT_CONVEYOR,
	GAME_OBJECT_COIN,
	GAME_OBJECT_BULLET,
	GAME_OBJECT_MINE,
	GAME_OBJECT_ENEMY,
};

struct __attribute__((packed)) GameObject{
	enum GameObjectType type;
	int8_t x;
	int8_t y;
	int8_t extra; //cloud: conveyor speed. bullet/mines/enemies: movement speed.
};

extern struct GameObject gameObjectArray[GAME_OBJECT_NUM];

extern void gameObjectInit(void);
extern struct GameObject* gameObjectNew(enum GameObjectType objectType, int8_t x, int8_t y);
extern void gameObjectDelete(struct GameObject* gameObject);
