
#include "Bat.h"
#include "App.h"
#include "Render.h"
#include "Player.h"
#include "Pathfinding.h"
#include "Map.h"
#include "Scene.h"
#include "Log.h"
#include "Physics.h"
#include "EntityManager.h"
#include "Textures.h"
#include "Audio.h"	


#include <math.h>
#include <iostream>
using namespace std;

#define DEATH_TIME 25;

Bat::Bat() : Entity(EntityType::BAT) {


	name.Create("Bat");


}

Bat::~Bat() {

}
bool Bat::Awake()
{
	idleAnimation.GenerateAnimation(SDL_Rect({ 0, 0, 216, 25 }), 1, 12);
	idleAnimation.speed = 0.2f;

	flyingLeftAnimation.GenerateAnimation(SDL_Rect({ 0, 25, 322, 26 }), 1, 7);
	flyingLeftAnimation.speed = 0.2f;

	flyingRightAnimation.GenerateAnimation(SDL_Rect({ 0, 51, 322, 26 }), 1, 7);
	flyingRightAnimation.speed = 0.2f;

	questionMarkAnimation.GenerateAnimation(SDL_Rect({ 322, 0, 96, 96 }), 3, 3);
	questionMarkAnimation.speed = 0.2f;

	deathAnimation.GenerateAnimation(SDL_Rect({ 0, 96, 385, 55 }), 1, 7);
	deathAnimation.speed = 0.2f;
	deathAnimation.loop = false;

	texturePath = parameters.attribute("texturepath").as_string();
	Enemyposition.x = parameters.attribute("x").as_int();
	Enemyposition.y = parameters.attribute("y").as_int();
	speed = parameters.attribute("speed").as_int();

	return true;

}
bool Bat::CleanUp()
{
	delete &path;
	return true;
}

bool Bat::Start()
{

	BatTexture = app->tex->Load(texturePath);

	currentAnimation = &idleAnimation;

	lastPlayerPosition.x = -1;
	lastPlayerPosition.y = -1;

	batbody = app->physics->CreateCircle(Enemyposition.x-10 , Enemyposition.y, 10, bodyType::DYNAMIC);

	 //L07 DONE 6: Assign player class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
	batbody->listener = this;

	 //L07 DONE 7: Assign collider type
	batbody->ctype = ColliderType::BAT;


	headBody = app->physics->CreateRectangle(METERS_TO_PIXELS(batbody->body->GetTransform().p.x), METERS_TO_PIXELS(batbody->body->GetTransform().p.y) - 12, 15, 2, bodyType::STATIC);
	headBody->ctype = ColliderType::FLYENEMYHEAD;
	headBody->body->SetFixedRotation(true);
	isDead = false;
	state = State::IDLE;
	timerDeath = DEATH_TIME;

	return true;
}

bool Bat::Update( )
{
	currentAnimation->Update();

	iPoint playerPos;
	playerPos.x = METERS_TO_PIXELS(app->scene->player->pbody->body->GetPosition().x) / app->map->mapData.tileWidth;
	playerPos.y = METERS_TO_PIXELS(app->scene->player->pbody->body->GetPosition().y) / app->map->mapData.tileHeight;

	iPoint gridPos;
	gridPos.x = Enemyposition.x / app->map->mapData.tileWidth;
	gridPos.y = Enemyposition.y / app->map->mapData.tileHeight;

	//printf("Enemyposition X: %f \n", Enemyposition.x);
	//printf("Enemyposition Y: %f \n", Enemyposition.y);

	//printf("gridPos X: %d \n", gridPos.x);
	//printf("gridPos Y: %d \n", gridPos.y);

	//cout << playerPos.x << endl;

	//cout << app->scene->player->pbody->body->GetPosition().x << endl;


	b2Vec2 vel(0,0);
	if (isDead == true) { state = State::DYING; }


	if (playerPos != lastPlayerPosition && playerPos.DistanceTo(gridPos) <= 12 && state != State::DYING && !app->scene->godMode)
	{
		lastPlayerPosition = playerPos;

		int n = app->pathfinding->CreatePath(gridPos, playerPos, false, 0, 12);
		if (n == -1)
		{
			hasPath = false;
			path.Clear();
			path.PushBack(iPoint(gridPos.x, gridPos.y));
			
		}
		else
		{
			hasPath = true;
			const DynArray<iPoint>* newPath = app->pathfinding->GetLastPath();
			int j = 0;
			for (int i = 0; i < path.Count(); i++)
			{
				if (path[i] != (*newPath)[j])
					continue;
				j++;
				break;
			}
			path.Clear();
			for (int i = 0; i < newPath->Count(); i++)
			{
				path.PushBack((*newPath)[i]);
			}
			if (j < path.Count())
				pathIndex = j;
			else
				pathIndex = 0;
			state = State::FLYING;
		}
	}

	fPoint pixelPosition;
	float distance;

	switch (state)
	{
	case State::IDLE:
		currentAnimation = &idleAnimation;
		vel = b2Vec2(0, 0);
		break;
	case State::FLYING:

		currentAnimation = &flyingLeftAnimation;
		vel = b2Vec2(0, 0);
		if (app->scene->godMode)
			break;

		if (pathIndex >= path.Count())
			break;

		pixelPosition.x = path[pathIndex].x * app->map->mapData.tileWidth;
		pixelPosition.y = path[pathIndex].y * app->map->mapData.tileHeight;



		distance = pixelPosition.DistanceTo(Enemyposition);

		if (distance == 0)
		{
			pathIndex++;
			vel.y = 0;
			vel.x = 0;
		}
		else
		{
			float xDiff = pixelPosition.x - Enemyposition.x;
			float yDiff = pixelPosition.y - Enemyposition.y;

			cout << xDiff << endl;
			cout << yDiff << endl;
		
			if (xDiff < 0)
			{
				currentAnimation = &flyingLeftAnimation;
			}
			else if (xDiff >= 0)
			{
				currentAnimation = &flyingRightAnimation;
			}

			if (abs(xDiff) > abs(yDiff))
			{
				int xDir = (xDiff > 0) ? 1 : -1;
				if (abs(xDiff) < abs(xDir * speed ))
				{
					vel.x += xDiff;
					vel.y = 0;
				}
				else 
				{
					vel.x = speed * xDir;
					vel.y = 0;
				}
			}
			else
			{
				int yDir = (yDiff > 0) ? 1 : -1;
				if (abs(yDiff) < abs(yDir * speed))
				{
					vel.y += yDiff;
					vel.x = 0;
				}
				else
				{
					vel.y += yDir * speed;
					vel.x = 0;
			    }
				
			}
		}
		break;

	case State::DYING:
		if (timerDeath >= 0) {
			currentAnimation = &deathAnimation;
			--timerDeath;
		}
		else
		{
			app->entityManager->DestroyEntity(app->scene->bat);
			app->physics->world->DestroyBody(batbody->body);
			app->physics->world->DestroyBody(headBody->body);
			//timerDeath = DEATH_TIME;
		}
		if (currentAnimation->HasFinished())
		{
			pendingToDelete = true;
		}
		break;
	}


	Enemyposition.x = METERS_TO_PIXELS(batbody->body->GetTransform().p.x-10);
	Enemyposition.y = METERS_TO_PIXELS(batbody->body->GetTransform().p.y-10);
	headBodyPos.x = batbody->body->GetTransform().p.x;
	headBodyPos.y = batbody->body->GetTransform().p.y - PIXEL_TO_METERS(12);
	headBody->body->SetTransform({ headBodyPos.x, headBodyPos.y }, 0);

	questionMarkAnimation.Update();
	batbody->body->SetLinearVelocity(vel);
	Draw();

	return true;
}

bool Bat::Draw()
{
	if (state == State::FLYING)
	{
		app->render->DrawTexture(BatTexture, Enemyposition.x - 14, Enemyposition.y, &currentAnimation->GetCurrentFrame());
	}
	else if (state == State::DYING)
	{
		
		
			currentAnimation = &deathAnimation;
		    app->render->DrawTexture(BatTexture, Enemyposition.x - 20, Enemyposition.y - 15, &currentAnimation->GetCurrentFrame());
	     
		
	}
	else
		app->render->DrawTexture(BatTexture, Enemyposition.x, Enemyposition.y, &currentAnimation->GetCurrentFrame());

	if (hasPath)
	{
		if (app->physics->debug)
		{
			if (pathIndex < path.Count())
				app->render->DrawRectangle(SDL_Rect({ path[pathIndex].x * app->map->mapData.tileWidth - 3 + app->map->mapData.tileWidth / 2, path[pathIndex].y * app->map->mapData.tileHeight - 3 + app->map->mapData.tileHeight / 2, 6, 6 }), 255, 0, 0);
			app->pathfinding->DrawPath(&path, 255, 0, 0);
		}
	}
	else
	{
		if (state == State::FLYING)
		{
			app->render->DrawTexture(BatTexture, Enemyposition.x - 6, Enemyposition.y - 32, &questionMarkAnimation.GetCurrentFrame());
		}
	}

	return true;
}

//void Bat::Collision(Collider* other, float dt)
//{
//	if (other == app->entities->GetPlayer()->collider)
//	{
//		iPoint center = iPoint(collider->rect.x + (collider->rect.w / 2), collider->rect.y + (collider->rect.h / 2));
//		iPoint playerCenter = iPoint(other->rect.x + (other->rect.w / 2), other->rect.y + (other->rect.h / 2));
//
//		int xDiff = center.x - playerCenter.x;
//		int yDiff = center.y - playerCenter.y;
//
//		if (abs(yDiff) > abs(xDiff) && yDiff > 0 && app->entities->GetPlayer()->verticalVelocity < 0.0f)
//		{
//			app->ui->score += 5000;
//			app->audio->PlayFx(app->entities->GetPlayer()->doubleJumpFx, 0);
//			state = State::DYING;
//			collider->pendingToDelete = true;
//			app->entities->GetPlayer()->verticalVelocity = app->entities->GetPlayer()->jumpForce;
//		}
//	}
//}

void Bat::OnCollision(PhysBody* physA, PhysBody* physB) {


}

void Bat::Reset()
{
	Enemyposition = initialPosition;
	state = State::IDLE;
}

