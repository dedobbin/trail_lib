#include <unordered_map>
#include <assert.h>
#include <iostream>
#include <dump_lib.hpp>
#include <algorithm>
#include "action.hpp"
#include "scene.hpp"

/* to set back after item was used from submenu*/
static int selectedAnswerBeforeOpeningInventorySubMenu = 0;

static bool caps = false;

/* Stores text typed by playing if current convo text is prompt */
std::string Scene::promptText = "";

Scene::Scene(std::string _type, std::string name, int id, Inventory* inventory, Visuals* visuals, Character* player)
: id(id), name(name), selectedAnswer(0), inventory(inventory), visuals(visuals), player(player), type(_type)
{
    convoViewPort = visuals->VIEW_PORT_DEFAULT_CONVO_BAR;
    statusViewPort = visuals->VIEW_PORT_DEFAULT_STATUS_BAR;
    contentViewPort = visuals->VIEW_PORT_DEFAULT_CONTENT;
    inventoryViewPort = visuals->VIEW_PORT_DEFAULT_INVENTORY;

}

bool Scene::isLoaded()
{
	return loaded;
}

std::string Scene::getName()
{
	return name;
}

sceneId_t Scene::getId()
{
	return id;
}

sceneType_t Scene::getType()
{
	return type;
}

void Scene::openInventory()
{
	//std::cout << "DEBUG: open inventory" << std::endl;
	if (inventoryOpen)	return;
	selectedAnswer = 0;
	inventoryOpen = true;

	InventoryRenderer* renderer = new InventoryRenderer(inventory, &selectedAnswer, inventoryViewPort);
	visuals->addRenderer("INVENTORY", renderer);
}

void Scene::closeInventory()
{
	if (!inventoryOpen) return;

	selectedAnswer = 0;
	inventoryOpen = false;
	visuals->removeRenderer("INVENTORY");
}

bool Scene::inventoryIsOpen()
{
	return inventoryOpen;
}

bool Scene::connectScene(Scene* scene, bool connectBack)
{
	if (connectedScenes.count(scene->getId()> 0)){
		return false;
	} else {
		connectedScenes[scene->getId()] = scene;
		if (connectBack){
			scene->connectScene(this, false);
		}
		//std::cout  << "DEBUG: connected " << getName() << " to " << scene->getName() << std::endl;
		return true;
	}
}


void Scene::handleInput()
{
	SDL_Event e;
	while( SDL_PollEvent( &e ) != 0 ){
		if (e.type == SDL_QUIT){
			triggeredActions.push_back(new Action("QUIT"));
		} else {
			if (!visuals->hasRenderer("FADE_OUT")){ 
				if (!inventoryIsOpen() 
					&& !disableInventory
					&& e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_TAB
				) {
					//triggeredActions.push_back(new OpenInventoryAction());
					openInventory();
				} 
				else if (inventoryIsOpen()){
					handleInventoryInput(&e);
				} else if (currentConvo){
					auto inputConvoActions = handleConvoInput(&e);
					triggeredActions.insert(triggeredActions.end(), inputConvoActions.begin(), inputConvoActions.end());

					for (auto  a : triggeredActions){
						if (a->actionType == "CHANGE_SELECTED_ANSWER"){
							selectedAnswer = *((SetSelectedAnswer*)a)->answerId;
						}
					}
				}
				handleSpecificInput(e);
			}
		}
	}
}

void Scene::advanceTime(int amount)
{
	Time::advanceMinute(amount);
}

void Scene::load()
{
	if (loaded){
		std::cerr << "Tried to load already loaded scene " << type << " " << id << std::endl;
		return;
	}

	loadSceneDefault();
	loadSceneSpecifics();
	loaded = true;
}

void Scene::setActive(SceneInitArgs_t _args)
{
	if (!loaded){
		load();
	}

	visuals->removeAllRenderers();
	interruptedConvos = {};

	args = _args;
	addRenderers();

	/* The background called "CONTENT" always draws scenes background */
	if (backgroundPath != "" && visuals->hasRenderer("CONTENT")){
		visuals->getRenderer("CONTENT")->setBackground(Visuals::createBackground(backgroundPath));
	}

	locationData.discovered = true;
	if (locationData.isLocation){
		//locationData.weather = locationData.possibleWeatherTypes.at(dump_lib::random_int_range(0, locationData.possibleWeatherTypes.size()-1));
		locationData.weather = "NEUTRAL";
		locationData.weatherSeverity = dump_lib::random_int_range(1, 10) + 4;
		
		visuals->addRenderer("ATMOSPHERE", new AtmosphereRenderer(visuals->VIEW_PORT_FULL_SCREEN, &locationData));
	}

	if (defaultConvo >= 0){
		startConvo(defaultConvo);
	}

	setActiveSpecific();
}

bool Scene::isDiscoveredLocation()
{
	return locationData.isLocation && locationData.discovered;
}

void Scene::setWeather(weatherType_t type, int severity)
{
	locationData.weather = type;
	locationData.weatherSeverity = severity;
}



void Scene::clearSprites()
{
	for (auto sprite : sprites){
		delete(sprite);
	}
	sprites = {};
}

Scene::~Scene()
{
	for (auto convo : convos){
		delete(convo.second);
	}

	clearSprites();
}

void Scene::live(int& frameCounter)
{
	if (visuals->hasRenderer("TRANSITION")){
		Action* transitionDoneChangeState = ((TransitionRenderer*)visuals->getRenderer("TRANSITION"))->done();
		if (transitionDoneChangeState){
			triggeredActions.push_back(transitionDoneChangeState);
		}
	}else if (visuals->hasRenderer("FADE_OUT")){
		/* return NULL when not done yet */
		Action* transitionDoneChangeState = ((FadeoutRenderer*)visuals->getRenderer("FADE_OUT"))->done();
		if (transitionDoneChangeState){
			triggeredActions.push_back(transitionDoneChangeState);
		}
	} else if (isGameOver()){
		auto args = new ChangeSceneAction("CUT_SCENE", 6);
		triggeredActions.push_back(new FadeOutAction(args));
	}

	specificLive(frameCounter);
}

void Scene::handleInventoryInput(const SDL_Event* const e)
{
	InventoryRenderer* renderer = (InventoryRenderer*)visuals->getRenderer("INVENTORY");

	int nItems  = inventory->submenuIsOpen() 
		? inventory->itemWithSubMenuOpen->getInventorySubmenuData()->getAnswers().size() 
		: inventory->getItems().size();

	switch(e->type){
		case SDL_KEYUP:
			switch(e->key.keysym.sym){
				case SDLK_DOWN:
					selectedAnswer = selectedAnswer < nItems - 1 ? selectedAnswer + 1 % nItems : 0;
				break;
				case SDLK_UP:
					selectedAnswer = nItems > 0 ? selectedAnswer <= 0 ? nItems - 1 : selectedAnswer - 1 : 0;
				break;

				case SDLK_TAB:
					if (inventory->submenuIsOpen()){
						inventory->itemWithSubMenuOpen = NULL;
					} else {
						closeInventory();
					}
					selectedAnswer = 0;
				break;
				
				case SDLK_SPACE: case SDLK_RETURN:
					int j = 0;
					auto renderer = (InventoryRenderer*)visuals->getRenderer("INVENTORY");
					if (!inventory->itemWithSubMenuOpen){ /* select item from menu */
						for (auto item : inventory->getItems()){
							if (selectedAnswer == j){
								inventory->itemWithSubMenuOpen = item.second.first;
								selectedAnswerBeforeOpeningInventorySubMenu = selectedAnswer;
								selectedAnswer = 0;
								//std::cout << "DEBUG: item selected of type "<< item.first << std::endl;
								
								auto submenuData = inventory->itemWithSubMenuOpen->getInventorySubmenuData();
								
								/* for now, all items that don't have submenu will be used instantly when selected from inventory */
								if (!submenuData){
									UseItemParams* itemParams = constructItemParamsFromInventoryMenuSelection();
									if (itemParams){
										auto a = inventory->useItem(inventory->itemWithSubMenuOpen->type, *itemParams);
										triggeredActions.insert(triggeredActions.end(), a.begin(), a.end());
										delete(itemParams);
									}
								} 
							}
							j++;
						}
					} else { /* use item from submenu */
						UseItemParams* itemParams = constructItemParamsFromInventoryMenuSelection();
						if (itemParams){
							auto a = inventory->useItem(inventory->itemWithSubMenuOpen->type, *itemParams);
							triggeredActions.insert(triggeredActions.end(), a.begin(), a.end());
							delete(itemParams);
						} else {
							//std::cout << "DEBUG: inventorysubmenu closed without using item " << std::endl;
						}
						inventory->itemWithSubMenuOpen = NULL;
						/* when ran out of item and is last item, jump to item before */
						selectedAnswer = selectedAnswerBeforeOpeningInventorySubMenu >= inventory->size()
							? inventory->size() - 1
							: selectedAnswerBeforeOpeningInventorySubMenu;
					}
				
				break;
			}
		break;
	}
}

UseItemParams* Scene::constructItemParamsFromInventoryMenuSelection()
{
	/* check if cancel option was selected to close submenu without using item */
	if (inventory->submenuIsOpen()){
		auto answers = inventory->itemWithSubMenuOpen->getInventorySubmenuData()->getAnswers();
		if (answers[selectedAnswer].first == CANCEL_INVENTORY_SUBMENU){
			return NULL;
		}
	}

	auto useItemParams = constructItemParamsFromInventoryMenuSelectionSpecific();
	if (!useItemParams){
		if (inventory->submenuIsOpen()){
			auto answers = inventory->itemWithSubMenuOpen->getInventorySubmenuData()->getAnswers();
			if (inventory->itemWithSubMenuOpen->getInventorySubmenuData()->type == "CONFIRM"){
				if (!player){
					std::cout << "DEBUG: Scene::player == NULL, if there is no main player, lib cannot use a default confirm inventorysubmenu" 
						<< ",client can return own version in constructItemParamsFromInventoryMenuSelectionSpecific " << std::endl;
					exit(1);
				}
				
				if (answers[selectedAnswer].first == 0){
					useItemParams = new UseItemParams({player});
				} 
			} else if (inventory->itemWithSubMenuOpen->getInventorySubmenuData()->type == "MAP"){
				auto sceneId = answers[selectedAnswer].first;
				if (scenes.count(sceneId) == 0){
					std::cerr << "submenuData of map type, selected answer int does not match a scene id" << std::endl;
					exit(1);
				}

				useItemParams = new UseItemParams();
				useItemParams->targetLocation = scenes[sceneId];

			}
		} else { /* use item directly there is no submenu*/
			if (!player){
				std::cout << "DEBUG: Scene::player == NULL, if there is no main player, lib cannot use a default logic for using item directly from menu" 
					<< ",client can return own version in constructItemParamsFromInventoryMenuSelectionSpecific " << std::endl;
				exit(1);
			}
			useItemParams = new UseItemParams({player});
		}
	}
	if (!useItemParams){
		std::cerr << "Scene::constructItemParamsFromInventoryMenuSelection failed to construct valid useItemParams" 
		<< " for type: "<< inventory->itemWithSubMenuOpen->getInventorySubmenuData()->type;
		if (inventory->submenuIsOpen()){
			auto answers = inventory->itemWithSubMenuOpen->getInventorySubmenuData()->getAnswers();
			std::cerr << ", submenu selection: (" << answers[selectedAnswer].first << ") " << answers[selectedAnswer].second;
		}
		std::cerr << std::endl;
		exit(1);
	}
	return useItemParams;
}

void Scene::endConvo()
{
  	auto it = std::find_if (convos.begin(), convos.end(), [this](auto convo){
		  return convo.second == currentConvo;
	});
	if (it == convos.end()){
		std::cout << "DEBUG: ended detached convo, free-ing it.." << std::endl;
		delete(currentConvo);
	}
	if (interruptedConvos.size() == 0){
		currentConvo = NULL;
		visuals->removeRenderer("CONVO");	
	} else {
		auto interruptedConvo = interruptedConvos.top();
		interruptedConvos.pop();
		startConvo(interruptedConvo);
	}

}

void Scene::startConvo(Convo* convo, bool interrupt, int entryTextId)
{

	/* find first available key */
	// int i = 0; 
	// for (auto it = convos.cbegin(), end = convos.cend();
    // it != end && i == it->first; ++it, ++i){}
	// convos[i] = convo;
	// convoEntryPoints[i] = entryTextId;
	// startConvo(i);
	
	if (currentConvo && interrupt){
		interruptedConvos.push(currentConvo);
	} 

	currentConvo = convo;
	currentConvo->setActiveText(entryTextId);

	ConvoRenderer* r = NULL;
	if (!visuals->hasRenderer("CONVO")){
		r = new ConvoRenderer(&selectedAnswer, convoViewPort, 10);
		//std::cout << "DEBUG: Started convo: " << id << std::endl;
		visuals->addRenderer("CONVO", r);
	} else {
		r = (ConvoRenderer*)visuals->getRenderer("CONVO");
	}
	
	r->setText(currentConvo->current().second.text, currentConvo->current().second.answers, currentConvo->current().second.prompt);
}


void Scene::startConvo(int id, bool interrupt)
{
	if (convos.count(id) == 0){
		throw std::runtime_error("Tried starting convo with non-existing id " + id);
	}

	if (currentConvo && interrupt){
		interruptedConvos.push(currentConvo);
	} 

	currentConvo = convos[id];

	currentConvo->setActiveText(currentConvo->entryPoint);


	ConvoRenderer* r = NULL;
	if (!visuals->hasRenderer("CONVO")){
		r = new ConvoRenderer(&selectedAnswer, convoViewPort, 10);
		//std::cout << "DEBUG: Started convo: " << id << std::endl;
		visuals->addRenderer("CONVO", r);
	} else {
		r = (ConvoRenderer*)visuals->getRenderer("CONVO");
	}
	
	r->setText(currentConvo->current().second.text, currentConvo->current().second.answers, currentConvo->current().second.prompt);
}

void Scene::setActiveConvoText(int id)
{
	currentConvo->setActiveText(id);
	ConvoRenderer* r = (ConvoRenderer*)visuals->getRenderer("CONVO");
	r->setText(currentConvo->current().second.text, currentConvo->current().second.answers, currentConvo->current().second.prompt);
}

void Scene::setPromptText(std::string _text)
{
	ConvoRenderer* renderer = (ConvoRenderer*)visuals->getRenderer("CONVO");
	if (renderer->entireConvoTextIsRendered()){
		promptText = _text;
		renderer->setPromptText(promptText);
	}
}

void Scene::resetPromptText()
{
	promptText = "";
	ConvoRenderer* renderer = (ConvoRenderer*)visuals->getRenderer("CONVO");
	renderer->setPromptText("");
}

int Scene::getDefaultConvoId()
{
	return defaultConvo;
}


std::vector<Action*> Scene::handleConvoInput(const SDL_Event* const e)
{
	/* TODO: shift is tracked in 'caps' var when its pressed, when is done with more buttons should create a map */
	std::vector<Action*> triggeredActions = {};

	auto current = currentConvo->current().second;

	ConvoRenderer* renderer = (ConvoRenderer*)visuals->getRenderer("CONVO");
	switch( e->type ){
		case SDL_KEYUP:{
			int nAnswers = currentConvo->current().second.answers.size();
			
			if (!currentConvo->current().second.prompt){
				if (e->key.keysym.sym == SDLK_RIGHT){
					changeSelectedConvoAnswer(selectedAnswer, current.answers, RIGHT);
				}else if (e->key.keysym.sym == SDLK_LEFT){
					changeSelectedConvoAnswer(selectedAnswer, current.answers, LEFT);
				}
				//std::cout << "DEBUG: selected answer " << selectedAnswer << std::endl; 
			} else {
				if (e->key.keysym.sym >= SDLK_SPACE && e->key.keysym.sym <= SDLK_z){
					char newChar = (char)e->key.keysym.sym;
					if (caps){
						newChar = toupper(newChar);
					}
					setPromptText(promptText + newChar);
				} else if (e->key.keysym.sym == SDLK_BACKSPACE){
					setPromptText(promptText.substr(0, promptText.size() - 1));
				}
			}
			
			if (e->key.keysym.sym == SDLK_RETURN){
				auto renderer = (ConvoRenderer*)visuals->getRenderer("CONVO");
				if (renderer->entireConvoTextIsRendered()){
					auto actions = currentConvo->progress(selectedAnswer);

					if (currentConvo->current().second.prompt){
						resetPromptText();
					}

					triggeredActions.insert(triggeredActions.begin(), actions.begin(), actions.end());
					
					/* if progressing convo ended convo remove convo renderer*/
					if (!currentConvo){
						visuals->removeRenderer("CONVO");
					} else { /* other wise display new text */
						auto text = currentConvo->current().second;
						renderer->setText(text.text, text.answers, text.prompt);
					}
					break;
				} else{
					auto text = currentConvo->current().second;
					renderer->setText(text.text, text.answers, text.prompt, 0);
				}
			} else if (e->key.keysym.sym == SDLK_CAPSLOCK){
				caps = !caps;
			} else if (e->key.keysym.sym == SDLK_LSHIFT || e->key.keysym.sym == SDLK_RSHIFT){
				caps = false;
			}
		}break;
		case (SDL_KEYDOWN):
			if (e->key.keysym.sym == SDLK_LSHIFT || e->key.keysym.sym == SDLK_RSHIFT){
				caps = true;
			}
		break;
	}
	return triggeredActions;
}

/* Because answers is map with holes, these indices need to be skipped */
void Scene::changeSelectedConvoAnswer(int& selectedAnswer, const std::map<int, std::string> answers, const Direction dir)
{	
	if (answers.size() == 0) return;

	int lastAnswerIndex = (*answers.end()).first;
	int firstAnswerIndex = (*answers.begin()).first;
	
	int newSelection = selectedAnswer;

	if (dir == RIGHT){
		while (answers.find(++newSelection) == answers.end()){
			if (newSelection > lastAnswerIndex){
				//if want to wrap arround, assign selectedAnswer here
				return;
			};

		}
	} else if (dir == LEFT){
		while (answers.find(--newSelection) == answers.end()){
		if (newSelection < firstAnswerIndex){
				//if want to wrap arround, assign selectedAnswer here
				return;
			};
		}
	}

	selectedAnswer = newSelection;
}

void Scene::setConvoEntryPoint(int entryPoint, int convoId)
{
	Convo* convo = NULL;
	if (convoId < 0){
		if (!currentConvo){
			std::cerr << "setConvoEntryPoint was called with convoId < 0, so using currentConvo, but it's not set.."
			<< " (" << type << " " << id << ")" << std::endl;
			return;
		}
		convo = currentConvo;
	} else {
		if (convos.count(convoId) == 0){
			std::cerr << "setConvoEntryPoint tried to set starting point of non-existing convo " << convoId
			<< " (" << type << " " << id << ")" << std::endl;
			return;
		}
		convo = convos[convoId];
	}
	//std::cout << "DEBUG: set entry point for convo " << convoId << " to " << entryPoint << ", " << type << " " << id << std::endl; 
	convo->entryPoint = entryPoint;
}

Item* Scene::createItemFromType(itemType_t itemType)
{
	Item* item = createItemFromTypeSpecific(itemType);
	if (item){
		return item;
	}

	if (itemType == "JONKO"){
		return new Jonko();
	} else {
		std::cerr << "Scene::createItemFromType, unknown item type: " << itemType << std::endl;
		return NULL;
	}
}

std::unordered_map<sceneId_t, Scene*> Scene::scenes;