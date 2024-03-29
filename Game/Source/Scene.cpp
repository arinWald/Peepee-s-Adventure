#include "App.h"
#include "Input.h"
#include "Textures.h"
#include "Audio.h"
#include "Render.h"
#include "Window.h"
#include "Scene.h"
#include "EntityManager.h"
#include "Map.h"
#include "Player.h"
#include "Pathfinding.h"
#include "GuiManager.h"
#include "Item.h"
#include"ModuleUI.h"
#include <iostream>
using namespace std;

#include "Defs.h"
#include "Log.h"
#include "DynArray.h"

Scene::Scene() : Module()
{
	name.Create("scene");
}

// Destructor
Scene::~Scene()
{}

// Called before render is available
bool Scene::Awake(pugi::xml_node& config)
{
	LOG("Loading Scene");
	bool ret = true;

	// iterate all objects in the scene
	// Check https://pugixml.org/docs/quickstart.html#access
	for (pugi::xml_node itemNode = config.child("item"); itemNode; itemNode = itemNode.next_sibling("item"))
	{
		Item* item = (Item*)app->entityManager->CreateEntity(EntityType::ITEM);
		item->parameters = itemNode;
	}

	player = (Player*)app->entityManager->CreateEntity(EntityType::PLAYER);
	player->parameters = config.child("player");

	bat = (Bat*)app->entityManager->CreateEntity(EntityType::BAT);
	bat->parameters = config.child("bat");

	walkEnemy = (WalkEnemy*)app->entityManager->CreateEntity(EntityType::WALKENEMY);
	walkEnemy->parameters = config.child("walkenemy");

	//iterate all enemies
	/*for (pugi::xml_node enemyNode = config.child("walkenemy"); enemyNode; enemyNode = enemyNode.next_sibling("walkenemy"))
	{
		WalkEnemy* enemy = (WalkEnemy*)app->entityManager->CreateEntity(EntityType::WALKENEMY);
		enemy->parameters = enemyNode;
	}*/

	pugi::xml_node titleButtons = config.child("titleButtons");

	titleButtonsPath = titleButtons.attribute("texturepath").as_string();
	titleButtonsPath = "Assets/Textures/Ui/title_buttons.png";
	
	pugi::xml_node logo = config.child("logo");
	logotexturePath = logo.attribute("texturepath").as_string();

	pugi::xml_node intro = config.child("intro");
	introtexturePath = intro.attribute("texturepath").as_string();
	
	pugi::xml_node game_over = config.child("game_over");
	game_over_texturePath = game_over.attribute("texturepath").as_string();

	pugi::xml_node win_screen = config.child("win_screen");
	win_screen_texturePath = win_screen.attribute("texturepath").as_string();

	pugi::xml_node level_1_song = config.child("level_1_song");
	level1SongPath = level_1_song.attribute("audiopath").as_string();

	pugi::xml_node silence_song = config.child("silence_song");
	silenceSongPath = silence_song.attribute("audiopath").as_string();

	pugi::xml_node victory_song = config.child("victory_song");
	victorySongPath = victory_song.attribute("audiopath").as_string();
	return ret;
}

// Called before the first frame
bool Scene::Start()
{

	newGameButtonTex = app->tex->Load(titleButtonsPath);
	exitButtonTex = app->tex->Load(titleButtonsPath);

	newGameButtonAnim.PushBack({ 0,24,101,24 });
	newGameButtonAnim.PushBack({ 101,24,101,24 });

	exitButtonAnim.PushBack({ 0,96,101,24 });
	exitButtonAnim.PushBack({ 101,96,101,24 });

    newGameButtonAnim.speed=exitButtonAnim.speed = 6.0f;

	// L04: DONE 7: Set the window title with map/tileset info
	SString title("Peepee's Adventure - Map:%dx%d Tiles:%dx%d Tilesets:%d",
		app->map->mapData.width,
		app->map->mapData.height,
		app->map->mapData.tileWidth,
		app->map->mapData.tileHeight,
		app->map->mapData.tilesets.Count());

	app->win->SetTitle(title.GetString());

	
	logo = app->tex->Load(logotexturePath);
	intro = app->tex->Load(introtexturePath);
	game_over = app->tex->Load(game_over_texturePath);
	win_screen = app->tex->Load(win_screen_texturePath);

	godMode = false;

	bool retLoad = app->map->Load();

	// L12 Create walkability map
	if (retLoad) {
		int w, h;
		uchar* data = NULL;

		bool retWalkMap = app->map->CreateWalkabilityMap(w, h, &data);
		if (retWalkMap) app->pathfinding->SetMap(w, h, data);

		RELEASE_ARRAY(data);

	}

	//Sets the camera to be centered in isometric map
	if (app->map->mapData.type == MapTypes::MAPTYPE_ISOMETRIC) {
		uint width, height;
		app->win->GetWindowSize(width, height);
		app->render->camera.x = width / 2;

		// Texture to highligh mouse position 
		mouseTileTex = app->tex->Load("Assets/Maps/path.png");

		// Texture to show path origin 
		originTex = app->tex->Load("Assets/Maps/x.png");
	}

	if (app->map->mapData.type == MapTypes::MAPTYPE_ORTHOGONAL) {

		// Texture to highligh mouse position 
		mouseTileTex = app->tex->Load("Assets/Maps/path_square.png");

		// Texture to show path origin 
		originTex = app->tex->Load("Assets/Maps/x_square.png");
	}

	checkPoint = app->physics->CreateRectangleSensor(1382, 256, 32, 64, bodyType::STATIC);

	// L15: DONE 2: Declare a GUI Button and create it using the GuiManager
	uint w, h;
	app->win->GetWindowSize(w, h);
	button1 = (GuiButton*)app->guiManager->CreateGuiControl(GuiControlType::BUTTON, 1, "Button 1", { buttonsPosX, buttonsPosY + 24, 100, 24 }, this);
	button2 = (GuiButton*)app->guiManager->CreateGuiControl(GuiControlType::BUTTON, 2, "Button 2", { buttonsPosX, buttonsPosY + 48, 100, 24 }, this);


	return true;
}

// Called each loop iteration
bool Scene::PreUpdate()
{
	return true;
}

// Called each loop iteration
bool Scene::Update(float dt)
{

	if (app->input->GetKey(SDL_SCANCODE_F8) == KEY_DOWN)
	{
		app->guiManager->showDebug = !app->guiManager->showDebug;
		
	}
	if (app->input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN && gameplayState == LOGO_SCREEN)
	{
		FadeToNewState(TITLE_SCREEN);
	}
	/*if (app->input->GetKey(SDL_SCANCODE_0) == KEY_DOWN && gameplayState == PLAYING)
	{
		FadeToNewState(GAME_OVER_SCREEN);
	}*/

	if (gameplayState == TITLE_SCREEN && (app->input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN ))
	{
		app->scene->FadeToNewState(PLAYING);
		LOG("LOAD REQUESTED");
	}
	/*if (gameplayState == GAME_OVER_SCREEN && app->input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN)
	{
		app->scene->FadeToNewState(TITLE_SCREEN);
	}*/
	//if (gameplayState == WIN_SCREEN && app->input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN)
	//{
	//	app->scene->FadeToNewState(TITLE_SCREEN);
	//}
	if (player->playerlives <= 0 /*|| app->ui->timer<=0*/) {
		app->scene->FadeToNewState(app->scene->GAME_OVER_SCREEN);

	}

	if (player->isWin && gameplayState == PLAYING)
	{
		player->playerlives = 3;
		app->scene->FadeToNewState(app->scene->WIN_SCREEN);
	}

	if (gameplayState == PLAYING) {
		if (app->input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN)
			app->SaveGameRequest();

		if (app->input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
			app->LoadGameRequest();

		//if (app->input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)
		//	app->render->camera.y += 3;

		//if (app->input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
		//	app->render->camera.y -= 3;

		//if (app->input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)
		//	app->render->camera.x += 3;

		//if (app->input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
		//	app->render->camera.x -= 3;


		if (app->input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN)
		{
			player->playerlives = 3;
			player->ChangePosition(player->initialPosX, player->initialPosY);
		}
		/*if (app->input->GetKey(SDL_SCANCODE_F2) == KEY_DOWN)
			player->playerlives = 0;*/
		// Start from the beginning of the current level
		if (app->input->GetKey(SDL_SCANCODE_F3) == KEY_DOWN)
		{
			player->ChangePosition(player->initialPosX, player->initialPosY);
		}
		if (app->input->GetKey(SDL_SCANCODE_F10) == KEY_DOWN)
		{
			if (!godMode) godMode = true;
			else godMode = false;
		}
	}

	if (app->input->GetKey(SDL_SCANCODE_F11) == KEY_DOWN)//caps fps to 30 or 60
	{
		if (app->FPS == 60)
		{
			app->FPS = 30;
		}
		else
		{
			app->FPS = 60;
		}
	}

	if (gameplayState != targetState)
	{
		//Abans era 0.05f
		currentFade += 0.02f;
		if (currentFade >= 0.0f)
		{
			currentFade = 0.0f;
			ChangeGameplayState(targetState);
		}
	}
	else if (currentFade > 0.0f)
	{
		currentFade -= 0.02f;
	}
	else if (currentFade <= 0.0f)
	{
		currentFade = 0.0f;
		fading = false;
	}

	app->render->DrawTexture(game_over, 0, 0);
	
	// Draw map
	if (app->scene->gameplayState == PLAYING)
	{
		app->map->Draw();
	}

	

	//PATHFINDING WALK ENEMY
	if (walkEnemy->currentMoveState == WalkEnemy::MoveState::CHASING)
	{
		origin.x = walkEnemy->pbody->body->GetPosition().x;
		origin.y = walkEnemy->pbody->body->GetPosition().y;
		iPoint destination;
		destination.x = player->pbody->body->GetPosition().x;
		destination.y = player->pbody->body->GetPosition().y;
		app->pathfinding->ClearLastPath();
		app->pathfinding->CreatePath(origin, destination);
		// Get the latest calculated path and draw
		const DynArray<iPoint>* path = app->pathfinding->GetLastPath();
		for (uint i = 0; i < path->Count(); ++i)
		{
			iPoint pos = app->map->MapToWorld(path->At(i)->x, path->At(i)->y);
			if (i == 1)
			{
				walkEnemy->objective.x = PIXEL_TO_METERS(pos.x);
				walkEnemy->objective.y = PIXEL_TO_METERS(pos.y);
			}
			if (app->physics->debug) app->render->DrawTexture(mouseTileTex, pos.x, pos.y);
		}

		// Debug pathfinding
		if (app->physics->debug)
		{
		iPoint originScreen = app->map->MapToWorld(origin.x, origin.y);
		if (app->physics->debug) app->render->DrawTexture(originTex, originScreen.x, originScreen.y);
	    }
	}
	newGameButtonAnim.Update();
	exitButtonAnim.Update();

	return true;
}

// Called each loop iteration
bool Scene::PostUpdate()
{
	

	if (app->input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN || (gameplayState == WIN_SCREEN && app->input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN)
		|| (gameplayState == GAME_OVER_SCREEN && app->input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN))
	{
		ret = false;

	}

	if (gameplayState == LOGO_SCREEN)
	{
		app->render->DrawTexture(logo, 0, 0);
	}

	if (gameplayState == TITLE_SCREEN)
	{
		app->render->DrawTexture(intro, 0, 0);
		
		app->guiManager->Draw();
		
		SDL_Rect newGameRect = newGameButtonAnim.GetCurrentFrame();
		app->render->DrawTexture(newGameButtonTex, buttonsPosX, buttonsPosY + 24, &newGameRect, 0, 0, 0, 0);

		SDL_Rect exitRect = exitButtonAnim.GetCurrentFrame();
		app->render->DrawTexture(exitButtonTex, buttonsPosX, buttonsPosY + 48, &exitRect, 0, 0, 0, 0);
	}

	if (gameplayState == GAME_OVER_SCREEN)
	{
		app->guiManager->Draw();
		app->render->DrawTexture(game_over, 0, 0);

		SDL_Rect exitRect = exitButtonAnim.GetCurrentFrame();
		app->render->DrawTexture(exitButtonTex, buttonsPosX, buttonsPosY + 48, &exitRect, 0, 0, 0, 0);

	}
	if (gameplayState == WIN_SCREEN)
	{
		app->guiManager->Draw();
		app->render->DrawTexture(win_screen, 0, 0);

	}
	return ret;
}


bool Scene::LoadState(pugi::xml_node& data)
{
	player->ChangePosition(data.child("player").attribute("x").as_int() , data.child("player").attribute("y").as_int());

	walkEnemy->ChangePosition(data.child("walkEnemy").attribute("x").as_int(), data.child("walkEnemy").attribute("y").as_int());
	walkEnemy->ChangeMoveState(data.child("walkEnemy").attribute("moveState").as_int());

	return true;
}

bool Scene::SaveState(pugi::xml_node& data)
{
	pugi::xml_node playerNode = data.append_child("player");

	playerNode.append_attribute("x") = player->position.x;
	playerNode.append_attribute("y") = player->position.y;

	pugi::xml_node walkEnemyNode = data.append_child("walkEnemy");
	walkEnemyNode.append_attribute("x") = walkEnemy->position.x;
	walkEnemyNode.append_attribute("y") = walkEnemy->position.y;
	walkEnemyNode.append_attribute("moveState") = (int*)walkEnemy->currentMoveState;

	pugi::xml_node batNode = data.append_child("batEnemy");
	batNode.append_attribute("x") = bat->position.x;
	batNode.append_attribute("y") = bat->position.y;
	batNode.append_attribute("state") = (int*)bat->state;

	return true;
}

void Scene::FadeToNewState(GameplayState newState)
{
	if (gameplayState == newState)
		return;
	if (fading)
		return;
	targetState = newState;
	currentFade = 0.0f;
	fading = true;
}

void Scene::ChangeGameplayState(GameplayState newState)
{
	if (gameplayState == newState) return;

	switch (newState)
	{
	case LOGO_SCREEN:
		gameplayState = LOGO_SCREEN;
		app->render->camera.x = 0;
		app->render->camera.y = 0;
		break;
	case PLAYING:
		gameplayState = PLAYING;
		//app->map->Load();
		app->audio->PlayMusic(level1SongPath, 0);
		player->ChangePosition(player->initialPosX, player->initialPosY);
		break;
	case TITLE_SCREEN:
		gameplayState = TITLE_SCREEN;
		app->map->Load();
		player->isDead = false;
		player->isWin = false;
		/*button1= (GuiButton*)app->guiManager->CreateGuiControl(GuiControlType::BUTTON, 1, 0, SDL_Rect({ buttonsPosX, buttonsPosY, 101, 24 }),this);
		button2=(GuiButton*)app->guiManager->CreateGuiControl(GuiControlType::BUTTON, 2, 0, SDL_Rect({ buttonsPosX, buttonsPosY + 24, 101, 24 }),this);*/
		player->playerlives = 3;
		app->render->camera.x = 0;
		app->render->camera.y = 0;
		break;
	case GAME_OVER_SCREEN:
		gameplayState = GAME_OVER_SCREEN;
		app->audio->PlayMusic(silenceSongPath, 0);
		//app->map->CleanUp();
		app->render->camera.x = 0;
		app->render->camera.y = 0;
		break;
	case WIN_SCREEN:
		app->audio->PlayMusic(victorySongPath, 0);
		gameplayState = WIN_SCREEN;
		//app->map->CleanUp();
		app->render->camera.x = 0;
		app->render->camera.y = 0;
	}
}


// Called before quitting
bool Scene::CleanUp()
{
	LOG("Freeing scene");

	return true;
}

bool Scene::OnGuiMouseClickEvent(GuiControl* control)
{
	 
	// L15: DONE 5: Implement the OnGuiMouseClickEvent method
	LOG("Event by %d ", control->id);
	if (fading)
		return true;

	if (gameplayState == TITLE_SCREEN)
	{
		
		switch (control->type)
		{
		case GuiControlType::BUTTON:
			switch (control->id)
			{
			case 1:
				app->audio->PlayFx(app->guiManager->pressButtonFx);
				FadeToNewState(PLAYING);
				break;


			case 2:
				app->audio->PlayFx(app->guiManager->pressButtonFx);
				 ret = false;
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
	}
	if (gameplayState == GAME_OVER_SCREEN)
	{

		switch (control->type)
		{
		case GuiControlType::BUTTON:
			switch (control->id)
			{
			case 1:
				app->audio->PlayFx(app->guiManager->pressButtonFx);
				FadeToNewState(PLAYING);
				break;


			case 2:
				app->audio->PlayFx(app->guiManager->pressButtonFx);
				ret = false;
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
		if (gameplayState == WIN_SCREEN)
		{

			switch (control->type)
			{
			case GuiControlType::BUTTON:
				switch (control->id)
				{
				case 1:
					app->audio->PlayFx(app->guiManager->pressButtonFx);
					FadeToNewState(PLAYING);
					break;


				case 2:
					app->audio->PlayFx(app->guiManager->pressButtonFx);
					ret = false;
					break;

				default:
					break;
				}
				break;

			default:
				break;
			}
		}
	}
	return ret;
    
	
}