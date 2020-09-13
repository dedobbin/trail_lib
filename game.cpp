#include <iostream>
#include <assert.h>
#include "game.hpp"
#include "visuals.hpp"
#include "scene.hpp"
#include "time.hpp"


void Game::loadScene(ChangeSceneAction* args, Visuals* v)
{
	currentScene = Scene::scenes[args->id];
	currentScene->load(v, args);
}

void Game::go()
{
	bool keepGoing = true;
	const int FPS = 60;
	const int SCREEN_TICK_PER_FRAME = 1000 / FPS;

	Visuals* visuals = new Visuals();

	/* init all lib specific renderers */	
	visuals->addRenderPrio("CONTENT", 20);
	visuals->addRenderPrio("CONVO", 40);
	visuals->addRenderPrio("INVENTORY", 60);
	visuals->addRenderPrio("FADE_OUT", 80);
	visuals->addRenderPrio("TRANSITION", 100);


	currentScene = NULL;

	while(keepGoing){
		if (!initialized){
			Time::init(5, Time::MAY, 1849, 11, 23);

			ChangeSceneAction* firstLoad = specificInit(visuals);
			loadScene(firstLoad, visuals);

			countedFrames = 0;
			fpsTimer.start();

			initialized = true;
		}

		capTimer.start();

		currentScene->triggeredActions = {};
		currentScene->handleInput(visuals);
		currentScene->live(visuals, countedFrames);
		keepGoing = handleActions(visuals);
		
		visuals->render();
		float avgFps = countedFrames / ( fpsTimer.getTicks() / 1000.f );
		if( avgFps > 2000000 ){
			avgFps = 0;
		}
		//std::cout << "FPS: " << avgFps << std::endl;
		++countedFrames;
		int frameTicks = capTimer.getTicks();
		if( frameTicks < SCREEN_TICK_PER_FRAME ){
			//Wait remaining time
			SDL_Delay( SCREEN_TICK_PER_FRAME - frameTicks );
		}

	}
	for(auto scene : Scene::scenes){
		delete(scene.second);
	}
	delete(visuals);
	specificDestroy();
	std::cout << "bye" << std::endl;
}

bool Game::handleActions(Visuals* visuals)
{
	bool keepGoing = true;

	for (auto nextAction : currentScene->triggeredActions){
		if (nextAction->actionType == "CHANGE_SCENE") {
			loadScene( (ChangeSceneAction*)nextAction, visuals );
		} else if (nextAction->actionType == "RESET"){
			specificDestroy();
			initialized = false;
		} else if (nextAction->actionType == "ADVANCE_TIME"){
			currentScene->advanceTime( ((AdvanceTimeAction*) nextAction)->amount);
		} else if (nextAction->actionType == "QUIT"){
			keepGoing = false;
		} else if (nextAction->actionType == "FADE_OUT"){
			ChangeSceneAction* nextScene = (ChangeSceneAction*)((FadeOutAction*)nextAction)->nextScene;
			uint8_t speed = ((FadeOutAction*)nextAction)->speed;
			visuals->addRenderer("FADE_OUT", new FadeoutRenderer(nextScene, VIEW_PORT_FULL_SCREEN, speed));
		} else if (nextAction->actionType == "TRANSITION") {
			ChangeSceneAction* nextScene = (ChangeSceneAction*)((TransitionAction*)nextAction)->nextScene;
			int length = ((TransitionAction*)nextAction)->length;
			visuals->addRenderer("TRANSITION", new TransitionRenderer(nextScene, VIEW_PORT_FULL_SCREEN, length));
		
		/* Needs to be handled here (game is last to handle actions) so scene child can add new actions in the handle function */
		} else if (nextAction->actionType == "SET_CONVO_ACTION_TRIGGER"){
			SetConvoActionTrigger* args = (SetConvoActionTrigger*)nextAction;
			currentScene->convo->setAction(args->textId, args->action, args->answerId);
		} else if (nextAction->actionType == "LINK_TEXTS"){
			LinkTexts* args = (LinkTexts*)nextAction;
			currentScene->convo->linkTexts(args->srcText, args->dstText, args->answer);
		}
	}

	handleActionsSpecific(visuals);

	return keepGoing;
}