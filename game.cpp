#include <iostream>
#include <assert.h>
#include "game.hpp"
#include "visuals.hpp"
#include "scene.hpp"
#include "time.hpp"

Game::Game(int screenW, int screenH, std::string fontPath, int fontH)
{
    visuals = new Visuals(screenW, screenH, fontPath, fontH);
}

void Game::setActiveScene(ChangeSceneAction* action)
{
	currentScene = Scene::scenes[action->id];
	currentScene->setActive(action->sceneInitArgs);
}

void Game::go()
{
	bool keepGoing = true;
	const int FPS = 10;
	const int SCREEN_TICK_PER_FRAME = 1000 / FPS;

	/* init all lib specific renderers */	
	visuals->addRenderPrio("CONTENT", 20);
	visuals->addRenderPrio("ATMOSPHERE", 40);
	visuals->addRenderPrio("CONVO", 60);
	visuals->addRenderPrio("INVENTORY", 80);
	visuals->addRenderPrio("STATUS", 100);
	visuals->addRenderPrio("FADE_OUT", 120);
	visuals->addRenderPrio("TRANSITION", 140);


	/* call child init functions */
	ChangeSceneAction* firstLoad = specificInit(visuals);
	setupScenes(visuals);
	setDefaultRendererBackgrounds(visuals);
	setSpecificRenderPrio(visuals);

	currentScene = NULL;

	while(keepGoing){
		
		if (!initialized){
			Time::init(5, Time::MAY, 1849, 11, 23);

			setActiveScene(firstLoad);

			countedFrames = 0;
			fpsTimer.start();

			initialized = true;
		}

		capTimer.start();

		currentScene->triggeredActions = {};
		currentScene->handleInput();
		currentScene->live(countedFrames);
		keepGoing = handleActions(visuals);
		
		visuals->render();
		
		float avgFps = countedFrames / ( fpsTimer.getTicks() / 1000.f );
		if( avgFps > 2000000 ){
			avgFps = 0;
		}
		//std::cout << "FPS: " << avgFps << std::endl;
		++countedFrames;
		int frameTicks = capTimer.getTicks();
		while (fpsTimer.getTicks() % 6  != 0);

	}

	specificDestroy();
	delete(visuals);
	for (auto scene : Scene::scenes){
		delete(scene.second);
	}
	std::cout << "bye" << std::endl;
}

bool Game::handleActions(Visuals* visuals)
{
	bool keepGoing = true;

	for (auto action : currentScene->triggeredActions){
		/*** game level actions ***/
		if (action->actionType == "CHANGE_SCENE") {
			/* should always be done as final action, so stash away */
			nextScene = (ChangeSceneAction*)action;
		} else if (action->actionType == "RESET"){
			specificDestroy();
			initialized = false;
		} else if (action->actionType == "QUIT"){
			keepGoing = false;
		} else if (action->actionType == "FADE_OUT"){
			ChangeSceneAction* nextScene = (ChangeSceneAction*)((FadeOutAction*)action)->nextScene;
			uint8_t speed = ((FadeOutAction*)action)->speed;
			visuals->addRenderer("FADE_OUT", new FadeoutRenderer(nextScene, visuals->VIEW_PORT_FULL_SCREEN, speed));
		} else if (action->actionType == "TRANSITION") {
			ChangeSceneAction* nextScene = (ChangeSceneAction*)((TransitionAction*)action)->nextScene;
			int length = ((TransitionAction*)action)->length;
			visuals->addRenderer("TRANSITION", new TransitionRenderer(nextScene, visuals->VIEW_PORT_FULL_SCREEN, length));
		
		/*** current scene level actions ***/
		} else if (action->actionType == "ADVANCE_TIME"){
			currentScene->advanceTime( ((AdvanceTimeAction*) action)->amount);
		}  else if (action->actionType == "OPEN_INVENTORY"){
			currentScene->openInventory();
		} else if (action->actionType == "CLOSE_INVENTORY"){
			currentScene->closeInventory();
		} else if (action->actionType == "START_CONVO"){
			auto args = (StartConvo*) action;
			if (args->convo){
				currentScene->startConvo(args->convo, args->interrupt, args->entryPoint);
			} else {
				currentScene->startConvo(args->convoId);
			}
			if (args->forceCloseInventory){
				currentScene->closeInventory();
			}
		} else if (action->actionType == "END_CONVO"){
			currentScene->endConvo();


		/*** render level actions ***/
		}else if (action->actionType == "CHANGE_CONVO_SPEED"){
			auto renderer = (ConvoRenderer*)visuals->getRenderer("CONVO");
			renderer->renderSpeedDivider = ((ChangeConvoSpeed*)action)->val;
		
		/*** convo level actions TODO: refactor duplicated logic ***/
		} else if (action->actionType == "SET_CONVO_ENTRY_POINT"){
			ConvoEntryPointArgs* args = (ConvoEntryPointArgs*)action;
			auto scene = args->sceneId >= 0 ? Scene::scenes[args->sceneId] : currentScene;
			if (!scene->isLoaded()){
				scene->load();
			}
			auto convoId = args->convoId >= 0 ? args->convoId : scene->getDefaultConvoId();
			scene->setConvoEntryPoint(args->entryPoint, convoId);
		} else if (action->actionType == "SET_TEXT"){
			SetText* args = (SetText*)action;
			auto scene = args->sceneId >= 0 ? Scene::scenes[args->sceneId] : currentScene;
			if (!scene->isLoaded()){
				scene->load();
			}
			auto convo = args->convoId >= 0 ? scene->convos[args->convoId] : scene->convos[currentScene->getDefaultConvoId()];
			convo->addText(args->textId, {args->text});
		} else if (action->actionType == "SET_CONVO_ACTION_TRIGGER"){
			SetConvoActionTrigger* args = (SetConvoActionTrigger*)action;
			auto scene = args->sceneId >= 0 ? Scene::scenes[args->sceneId] : currentScene;
			if (!scene->isLoaded()){
				scene->load();
			}
			auto convo = args->convoId >= 0 ? scene->convos[args->convoId] : scene->convos[currentScene->getDefaultConvoId()];
			convo->setAction(args->textId, args->action, args->answerId);
		} else if (action->actionType == "LINK_TEXTS"){
			LinkTexts* args = (LinkTexts*)action;
			auto scene = args->sceneId >= 0 ? Scene::scenes[args->sceneId] : currentScene;
			if (!scene->isLoaded()){
				scene->load();
			}
			auto convo = args->convoId >= 0 ? scene->convos[args->convoId] : scene->convos[currentScene->getDefaultConvoId()];
			convo->linkTexts(args->srcText, args->dstText, args->answer);
		}
	}

	handleActionsSpecific(visuals);

	if (nextScene){
		setActiveScene( nextScene);
		nextScene = NULL;
	}

	return keepGoing;
}