#ifndef __STATE_H__
#define __STATE_H__

#include "convo.hpp"
#include "visuals.hpp"
#include "action.hpp"
#include "character.hpp"
#include "inventory.hpp"
#include "type.hpp"
#include "location.hpp"
#include <stack>
#include <map>
#include <unordered_map>

class Scene
{
	public:
		Scene(sceneType_t type, std::string name, sceneId_t id, Inventory* inventory, Visuals* visuals, Character* player = NULL);
		~Scene();
		void load();
		void setActive(SceneInitArgs_t args);
		bool connectScene(Scene* scene, bool connectBack = false);
		Item* createItemFromType(itemType_t itemType);
		bool isLoaded();
		std::string getName();
		sceneType_t getType();
		sceneId_t getId();

		//TODO: make adder?
		static std::unordered_map<sceneId_t, Scene*> scenes;
		void handleInput();
		void live(int& frameCounter);
		/* answer player is highlighting */
		int selectedAnswer;
		std::vector<Action*> triggeredActions;
		/* When called with convoId < 0, will use currentConvo */
		void setConvoEntryPoint(int entryPoint, int convoId = -1);

		void setPromptText(std::string text);
		void resetPromptText();
		
		/* When interrupt is true, currentConvo will be pushed to a stack, then when this convo has ended, will continue that one */
		void startConvo(int id, bool interrupt = false);
		void startConvo(Convo* convo, bool interrupt = false, int entryPoint = 0);
		void endConvo();
		void setActiveConvoText(int id);

		void advanceTime(int amount = 0);

		void openInventory();
		void closeInventory();

		bool isDiscoveredLocation();
		void setWeather(weatherType_t type, int severity);

		UseItemParams* constructItemParamsFromInventoryMenuSelection();
		
		int getDefaultConvoId();

		Convo* currentConvo = NULL;
		std::map<int, Convo*> convos;

	private:
		void changeSelectedConvoAnswer(int& selectedAnswer, const std::map<int, std::string> answers, const Direction dir);
		void clearSprites();
		sceneType_t type;
		Character* player = NULL;
		bool loaded = false;
		/* when convo is started with interrupt true, current convo is pushed to this stack. When next convo ends, will pop from here to continue.*/
		std::stack<Convo*> interruptedConvos;

		/**** interface to implement ****/
		/* can be used to set up general stuff, unrelated to a specific scene ID */
		virtual void loadSceneSpecifics() = 0;
		/* use to init convos, sprites, background specific based on ID of scene */
		virtual void loadSceneDefault() = 0;
		/* logic to be executed when scene is set active, example set flip of sprites based on direction of a journey etc. */
		virtual void setActiveSpecific() = 0;
		/* called each frame, can be used for timeprogression etc */
		virtual void specificLive(int& frameCounter) = 0;
		/* called each frame, can be used  */
		virtual void handleSpecificInput(SDL_Event& e) = 0;
		virtual void addRenderers() = 0;
		/* hook for gameover check, can be convient to put in client base scene */
		virtual bool isGameOver() = 0;
		
		/** functions that should/could be implemented in client base scene **/
		/* Hook for logic when item is selected from inventory, set giver and receiver of item, based on selection (use, cancel, etc) */
		/* If said item has no inventory submenu, function is called when item is selected */
		/* If said item has inventory submenu, is called when selection is made from submenu */
		/* should be overloaded in client base scene */
		virtual UseItemParams* constructItemParamsFromInventoryMenuSelectionSpecific() = 0;
		/* called from createItemFromType 
		/* Implement to create items and their submenu etc data, 
		/* Can also overload items from lib to give custom submenu etc.
		/* Should return NULL if itemtype is not known to client, so parent function can do it's logic
		 */
		virtual Item* createItemFromTypeSpecific(itemType_t) = 0;
	protected:
		Visuals* const visuals;
		SceneInitArgs_t args = {};
		bool disableInventory = false;
		std::string name;
		sceneId_t id;
		LocationData locationData;
		std::unordered_map<sceneId_t, Scene*> connectedScenes;
		
		bool inventoryOpen = false;
		/* allows scene to continue when party is dead, useful for intro or gameover cutscenes */
		bool skipGameOverCheck = false;
		std::vector<Sprite*> sprites;
		Inventory* inventory = NULL;

		void handleInventoryInput(const SDL_Event* const e);
		std::vector<Action*> handleConvoInput(const SDL_Event* const e);
		bool inventoryIsOpen();

		static std::string promptText;

		/* When this is > 1, convo with this ID will be started after scene loads */
		int defaultConvo = -1;

		std::string backgroundPath = "";

		SDL_Rect convoViewPort;
		SDL_Rect statusViewPort;
		SDL_Rect contentViewPort;
		SDL_Rect inventoryViewPort;
};

#endif