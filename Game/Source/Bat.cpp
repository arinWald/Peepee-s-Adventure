
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

Bat::Bat() : Entity(EntityType::BAT) {


	name.Create("Bat");


}

Bat::~Bat() {

}
bool Bat::Awake()
{
	idleAnimation.GenerateAnimation(SDL_Rect({ 0, 0, 216, 25 }), 1, 12);
	idleAnimation.speed = 6.0f;

	flyingLeftAnimation.GenerateAnimation(SDL_Rect({ 0, 25, 322, 26 }), 1, 7);
	flyingLeftAnimation.speed = 10.0f;

	flyingRightAnimation.GenerateAnimation(SDL_Rect({ 0, 51, 322, 26 }), 1, 7);
	flyingRightAnimation.speed = 10.0f;

	questionMarkAnimation.GenerateAnimation(SDL_Rect({ 322, 0, 96, 96 }), 3, 3);
	questionMarkAnimation.speed = 10.0f;

	deathAnimation.GenerateAnimation(SDL_Rect({ 0, 96, 385, 55 }), 1, 7);
	deathAnimation.speed = 10.0f;
	deathAnimation.loop = false;

	texturePath = parameters.attribute("texturepath").as_string();


}
bool Bat::CleanUp()
{
	delete& path;
	return true;
}

bool Bat::Start()
{

BatTexture = app->tex->Load(texturePath);

currentAnimation = &idleAnimation;

lastPlayerPosition.x = -1;
lastPlayerPosition.y = -1;

batbody = app->physics->CreateCircle(position.x + 16, position.y + 16, 14, bodyType::DYNAMIC);

// L07 DONE 6: Assign player class (using "this") to the listener of the pbody. This makes the Physics module to call the OnCollision method
batbody->listener = this;

// L07 DONE 7: Assign collider type
batbody->ctype = ColliderType::PLAYER;

state = State::IDLE;

	return true;
}

bool Bat::Update(float dt)
{
	currentAnimation->Update(dt);

	iPoint playerPos;
	playerPos.x = app->scene->player->pbody->body->GetPosition().x/ app->map->mapData.tileWidth;
	playerPos.y = app->scene->player->pbody->body->GetPosition().y / app->map->mapData.tileHeight;

	iPoint gridPos;
	gridPos.x = position.x / app->map->mapData.tileWidth;
	gridPos.y = position.y / app->map->mapData.tileHeight;

	b2Vec2 vel;

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

		//currentAnimation = &flyingLeftAnimation;
		if (app->scene->godMode)
			break;

		if (pathIndex >= path.Count())
			break;

		pixelPosition.x = path[pathIndex].x * app->map->mapData.tileWidth;
		pixelPosition.y = path[pathIndex].y * app->map->mapData.tileHeight;

		distance = pixelPosition.DistanceTo(position);

		if (distance == 0)
		{
			pathIndex++;
		}
		else
		{
			float xDiff = pixelPosition.x - position.x;
			float yDiff = pixelPosition.y - position.y;

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
				if (abs(xDiff) < abs(xDir * speed * dt))
				{
					position.x += xDiff;
				}
				else
					position.x += xDir * speed * dt;
			}
			else
			{
				int yDir = (yDiff > 0) ? 1 : -1;
				if (abs(yDiff) < abs(yDir * speed * dt))
				{
					position.y += yDiff;
				}
				else
					position.y += yDir * speed * dt;
			}
		}
		break;

	case State::DYING:
		currentAnimation = &deathAnimation;
		if (currentAnimation->HasFinished())
		{
			pendingToDelete = true;
		}
		break;
	}

	

	questionMarkAnimation.Update(dt);

	return true;
}

bool Bat::Draw()
{
	if (state == State::FLYING)
	{
		app->render->DrawTexture(BatTexture, position.x - 14, position.y, &currentAnimation->GetCurrentFrame());
	}
	else if (state == State::DYING)
	{
		currentAnimation = &deathAnimation;
		app->render->DrawTexture(BatTexture, position.x - 20, position.y - 15, &currentAnimation->GetCurrentFrame());
	}
	else
		app->render->DrawTexture(BatTexture, position.x, position.y, &currentAnimation->GetCurrentFrame());

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
			app->render->DrawTexture(BatTexture, position.x - 6, position.y - 32, &questionMarkAnimation.GetCurrentFrame());
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
	position = initialPosition;
	state = State::IDLE;
}

