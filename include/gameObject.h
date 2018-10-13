#define GAME_OBJECT_NUM (256U)
#define CHARACTER_WIDTH (8)
#define CHARACTER_HEIGHT (8)
#define CLOUD_WIDTH (32)
#define CLOUD_HEIGHT (5)

enum GameObjectType{
	NO_OBJECT,
	GAME_OBJECT_CHARACTER,
	GAME_OBJECT_CLOUD,
};

struct __attribute__((packed)) GameObject{
	enum GameObjectType type;
	int8_t x;
	int8_t y;
};

extern struct GameObject gameObjectArray[GAME_OBJECT_NUM];

extern void gameObjectInit(void);
extern struct GameObject* gameObjectNew(enum GameObjectType objectType, int8_t x, int8_t y);
extern void gameObjectDelete(struct GameObject* gameObject);
