#ifndef __STATE_H__
#define __STATE_H__

#include "convo.hpp"
#include "visuals.hpp"
#include "action.hpp"
#include "inventory.hpp"
#include <map>

/* optional to use for client */
#define VIEW_PORT_FULL_SCREEN {0, 0, SCREEN_W, SCREEN_H}
#define VIEW_PORT_DEFAULT_CONVO_BAR {0, SCREEN_H - SCREEN_H/6, SCREEN_W, SCREEN_H /6}
#define VIEW_PORT_DEFAULT_STATUS_BAR {0, 0, SCREEN_W, SCREEN_H/9}

class Scene
{
	public:
		Scene(sceneType_t type, std::string name, sceneId_t id, Inventory* inventory);
		~Scene();
		void load(Visuals* visuals, ChangeSceneAction* args);
		void reset();
		std::string getName();
		sceneId_t getId();
		//TODO: make adder?
		static std::unordered_map<sceneId_t, Scene*> scenes;
		void handleInput(Visuals* visuals);
		void live(Visuals* visuals, int& frameCounter);
		/* public so game can set convo texts and actions etc */
		Convo* convo = NULL;
		/* answer player is highlighting */
		int selectedAnswer;
		std::vector<Action*> triggeredActions;

		void setConvoEntryPoint(int convo, int entryPoint);
		void setPromptText(std::string text, Visuals* visuals);
		void resetPromptText(Visuals* visuals);
		void setBackground(std::string path);
		
		void startConvo(int id, Visuals* v, SDL_Rect viewPort);
		void endConvo(Visuals* v);
		void setActiveConvoText(int id, Visuals* v);

		void advanceTime(int amount = 0);
	private:
		void changeSelectedConvoAnswer(int& selectedAnswer, const std::map<int, std::string> answers, const Direction dir);
		void clearSprites();
		sceneType_t type;

		/* interface to overload */
		virtual void loadSceneSpecifics(Visuals* visuals) = 0;
		virtual void loadSceneDefault(Visuals* visuals) = 0;
		virtual void specificLive(Visuals* visuals, int& frameCounter) = 0;
		virtual void handleSpecificInput(SDL_Event& e, Visuals* visuals, std::vector<Action*>& actions) = 0;
		virtual void setArgsSpecific(ChangeSceneAction* args) = 0;
		virtual bool isGameOver() = 0;
		virtual void useItem() = 0;
		virtual void addInventoryRenderer(Visuals* visuals) = 0;	//So children set their own overloaded version of inventoryRenderer
	protected:
		bool disableInventory = false;
		std::string name;
		sceneId_t id;
		bool inventoryOpen = false;
		static std::string promptText;
		std::unordered_map<int, int> convoEntryPoints;
		/* allows scene to continue when party is dead, useful for intro or gameover cutscenes */
		bool skipGameOverCheck = false;
		std::vector<Sprite*> sprites;
		Background background;
		Inventory* inventory = {};
		/* wether item is selected in inventory */
		bool itemIsSelected = false;

		void openInventory(Visuals* visuals);
		void closeInventory(Visuals* visuals);

		void handleInventoryInput(const SDL_Event* const e, Visuals* visuals);
		std::vector<Action*> handleConvoInput(const SDL_Event* const e, Visuals* visuals);
		bool inventoryIsOpen();
};

#endif