#ifndef __GAME_H__
#define __GAME_H__

#include <unordered_map>
#include "sdl_utils.hpp"
#include "scene.hpp"

class Game{
	public:
        Game(int screenW = 1280, int screenH = 720, std::string fontPath ="assets/font.ttf", int fontH = 24);
		void go();
	private:
		/*** interface ***/
		/* Called when game starts, or restarts from game over, returns first scene to load */
		virtual ChangeSceneAction* specificInit(Visuals* visuals) = 0;
		virtual void handleActionsSpecific(Visuals* visuals) = 0;
		virtual void setupScenes(Visuals* visuals) = 0;
		virtual void setDefaultRendererBackgrounds(Visuals* visuals) = 0;
		virtual void setSpecificRenderPrio(Visuals* visuals) = 0;
		virtual void specificDestroy() = 0;

		/* When set, next scene will be loaded after all actions are handled */
		ChangeSceneAction* nextScene = NULL;

		LazyFooTimer capTimer;
		LazyFooTimer fpsTimer;

		/* Should return false to end game */
		virtual bool handleActions(Visuals* visuals);

        Visuals* visuals = NULL;

	protected:
		bool initialized = false;
		Scene* currentScene = NULL;
		int countedFrames = 0;
		void setActiveScene(ChangeSceneAction* action);
};

#endif