#ifndef __FLYINGENEMY_H__
#define __FLYINGENEMY_H__


#include "Entity.h"
#include "Point.h"
#include "Animation.h"
#include "Physics.h"
#include "SDL/include/SDL.h"
#include "Animation.h"

struct SDL_Texture;

struct transformPosition {
	float posX;
	float posY;
	bool turn;

};

class FlyingEnemy : public Entity
{
public:

	FlyingEnemy();

	virtual ~FlyingEnemy();

	bool Awake();

	bool Start();

	bool Update();

	bool CleanUp();

	// L07 DONE 6: Define OnCollision function for the player. Check the virtual function on Entity class
	void OnCollision(PhysBody* physA, PhysBody* physB);

	void FlyingEnemy::ChangePosition(int x, int y);


public:



	int speed;
	int jumpspeed;
	bool ground;
	int jumpsavailable;
	int playerlives;

	// L07 DONE 5: Add physics to the player - declare a Physics body
	PhysBody* pbody;
	PhysBody* groundSensor;

	int timerJump;
	int pickCoinFxId;

	int timerPocho;
	int deathFxId;

	int defeatFxId;

	int timerDeath;
	bool isDead;

	bool isWin;

	int jumpFxId;
	int LastDir;

	transformPosition teleport;

	int initialPosX;
	int initialPosY;

private:



};

#endif