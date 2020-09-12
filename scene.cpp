#include <unordered_map>
#include <iostream>
#include "scene.hpp"

static bool caps = false;

/* Stores text typed by playing if current convo text is prompt */
std::string Scene::promptText = "";

Scene::Scene(std::string _type, std::string name, int id, Inventory* inventory)
: id(id), name(name), selectedAnswer(0), inventory(inventory)
{ 
	type = _type;
}

std::string Scene::getName()
{
	return name;
}

sceneId_t Scene::getId()
{
	return id;
}

void Scene::openInventory(Visuals* visuals)
{
	std::cout << "DEBUG: open inventory" << std::endl;
	if (inventoryOpen)	return;
	selectedAnswer = 0;
	inventoryOpen = true;
	
	addInventoryRenderer(visuals);
}

void Scene::closeInventory(Visuals* visuals)
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


void Scene::handleInput(Visuals* visuals)
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
					openInventory(visuals);
				} 
				else if (inventoryIsOpen()){
					handleInventoryInput(&e, visuals);
				} else if (convo->active()){
					auto inputConvoActions = handleConvoInput(&e, visuals);
					triggeredActions.insert(triggeredActions.end(), inputConvoActions.begin(), inputConvoActions.end());

					for (auto  a : triggeredActions){
						if (a->actionType == "CHANGE_SELECTED_ANSWER"){
							selectedAnswer = *((SetSelectedAnswer*)a)->answerId;
						}
					}
				}
				handleSpecificInput(e, visuals, triggeredActions);
			}
		}
	}
}

void Scene::advanceTime(int amount)
{
	Time::advanceMinute(amount);
}

void Scene::load(Visuals* visuals, ChangeSceneAction* args)
{
	visuals->removeAllRenderers();
	reset();
	
	skipGameOverCheck = args->skipGameOverCheck;
	setArgsSpecific(args);
	
	loadSceneDefault(visuals);
	loadSceneSpecifics(visuals);
}

void Scene::clearSprites()
{
	for (auto sprite : sprites){
		delete(sprite);
	}
	sprites = {};
}

void Scene::reset()
{
	if (convo){
		delete(convo);
		convo = new Convo();
	}
	clearSprites();
}

Scene::~Scene()
{
	delete(convo);
	clearSprites();
}

void Scene::live(Visuals* v, int& frameCounter)
{
	if (v->hasRenderer("TRANSITION")){
		Action* transitionDoneChangeState = ((TransitionRenderer*)v->getRenderer("TRANSITION"))->done();
		if (transitionDoneChangeState){
			triggeredActions.push_back(transitionDoneChangeState);
		}
	}else if (v->hasRenderer("FADE_OUT")){
		/* return NULL when not done yet */
		Action* transitionDoneChangeState = ((FadeoutRenderer*)v->getRenderer("FADE_OUT"))->done();
		if (transitionDoneChangeState){
			triggeredActions.push_back(transitionDoneChangeState);
		}
	} else if (isGameOver()){
		auto args = new ChangeSceneAction("CUT_SCENE", 6);
		args->skipGameOverCheck = true;
		triggeredActions.push_back(new FadeOutAction(args));
	}

	specificLive(v, frameCounter);
}

void Scene::handleInventoryInput(const SDL_Event* const e, Visuals* visuals)
{
	//TODO: assert renderer is convo renderer
	InventoryRenderer* renderer = (InventoryRenderer*)visuals->getRenderer("INVENTORY");
 
	int nItems  = inventory->getItems().size();
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
					if (itemIsSelected){
						itemIsSelected = false;
					} else {
						closeInventory(visuals);
					}
					selectedAnswer = 0;
				break;
				
				case SDLK_SPACE: case SDLK_RETURN:
					int j = 0;
					if (!itemIsSelected){
						for (auto item : inventory->getItems()){
							if (selectedAnswer == j){
								inventory->selectedItem = item.second.first;
								itemIsSelected = true;
								selectedAnswer = 0;
								std::cout << "DEBUG: item selected of type "<< item.first << std::endl;
							}
							j++;
						}
					} else {
						useItem();
					}
				
				break;
			}
		break;
	}
}

void Scene::startConvo(int id, Visuals* v, SDL_Rect viewPort)
{
	if (convoEntryPoints.find(id) == convoEntryPoints.end()){
		throw std::runtime_error("Tried starting non-existing convo: " + id);
	}
	ConvoRenderer* r = new ConvoRenderer(&selectedAnswer, viewPort);
	int entryPoint = convoEntryPoints.at(id);
	convo->setActiveText(entryPoint);
	r->setText(convo->current().second.text, convo->current().second.answers, convo->current().second.prompt);
	//std::cout << "DEBUG: Started convo: " << id << std::endl;
	v->addRenderer("CONVO", r);
}

void Scene::setActiveConvoText(int id, Visuals* v)
{
	convo->setActiveText(id);
	ConvoRenderer* r = (ConvoRenderer*)v->getRenderer("CONVO");
	r->setText(convo->current().second.text, convo->current().second.answers, convo->current().second.prompt);
}

void Scene::endConvo(Visuals* v)
{
	convo->end();
	v->removeRenderer("CONVO");
}


void Scene::setBackground(std::string path)
{
	auto size = Visuals::imageSize(path);
	//TODO: now distorts image if dimension are unlike those of screen, should cut off instead?
	background.path = path;
	background.src = {0, 0, size.first, size.second};
	//background = {path, {0, 0, size.first, size.second}, viewPort};
}

void Scene::setPromptText(std::string _text, Visuals* visuals)
{
	if (visuals->entireConvoTextIsRendered()){
		promptText = _text;
		ConvoRenderer* renderer = (ConvoRenderer*)visuals->getRenderer("CONVO");
		renderer->setPromptText(promptText);
	}
}

void Scene::resetPromptText(Visuals* visuals)
{
	promptText = "";
	ConvoRenderer* renderer = (ConvoRenderer*)visuals->getRenderer("CONVO");
	renderer->setPromptText("");
}

std::vector<Action*> Scene::handleConvoInput(const SDL_Event* const e, Visuals* visuals)
{
	/* TODO: shift is tracked in 'caps' var when its pressed, when is done with more buttons should create a map */
	std::vector<Action*> triggeredActions = {};

	auto current = convo->current().second;

	ConvoRenderer* renderer = (ConvoRenderer*)visuals->getRenderer("CONVO");
	switch( e->type ){
		case SDL_KEYUP:{
			int nAnswers = convo->current().second.answers.size();
			
			if (!convo->current().second.prompt){
				if (e->key.keysym.sym == SDLK_RIGHT){
					changeSelectedConvoAnswer(selectedAnswer, current.answers, RIGHT);
				}else if (e->key.keysym.sym == SDLK_LEFT){
					changeSelectedConvoAnswer(selectedAnswer, current.answers, LEFT);
				}
			} else {
				if (e->key.keysym.sym >= SDLK_SPACE && e->key.keysym.sym <= SDLK_z){
					char newChar = (char)e->key.keysym.sym;
					if (caps){
						newChar = toupper(newChar);
					}
					setPromptText(promptText + newChar, visuals);
				} else if (e->key.keysym.sym == SDLK_BACKSPACE){
					setPromptText(promptText.substr(0, promptText.size() - 1), visuals);
				}
			}
			
			if (e->key.keysym.sym == SDLK_SPACE || e->key.keysym.sym == SDLK_RETURN){
				if (visuals->entireConvoTextIsRendered()){
					auto actions = convo->progress(selectedAnswer);

					if (convo->current().second.prompt){
						resetPromptText(visuals);
					}

					triggeredActions.insert(triggeredActions.begin(), actions.begin(), actions.end());
					
					/* if progressing convo ended convo remove convo renderer*/
					if (!convo->active()){
						visuals->removeRenderer("CONVO");
					} else { /* other wise display new text */
						auto text = convo->current().second;
						renderer->setText(text.text, text.answers, text.prompt);
					}
					selectedAnswer = 0;
					for (auto it = triggeredActions.begin(); it != triggeredActions.end(); it++){
						Action* a = *it;
						if (a->actionType == "OPEN_INVENTORY"){
							openInventory(visuals);
						} else if (a->actionType == "SET_CONVO_ENTRY_POINT"){
							ConvoEntryPointArgs* args = (ConvoEntryPointArgs*)a;
							setConvoEntryPoint(args->convo, args->entryPoint);
						} else if (a->actionType == "SET_TEXT"){
							SetText* args = (SetText*)a;
							convo->addText(args->textId, {args->text});
						} 
						//Return so child can handle child specific actions, and execute extra logic for handled actions
					}
					break;
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

void Scene::setConvoEntryPoint(int convo, int entryPoint)
{
	convoEntryPoints[convo] = entryPoint;
}


std::unordered_map<sceneId_t, Scene*> Scene::scenes;