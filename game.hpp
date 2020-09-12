#ifndef __GAME_H__
#define __GAME_H__

#include <unordered_map>
#include "sdl_utils.hpp"
#include "scene.hpp"

class Game{
	public:
		void go();
	private:
		LazyFooTimer capTimer;
		LazyFooTimer fpsTimer;

		/* Called when game starts, or restarts from game over, returns first scene to load */
		virtual ChangeSceneAction* specificInit(Visuals* visuals) = 0;
		/* Should return false to end game */
		virtual bool handleActions(Visuals* visuals);

		virtual void handleActionsSpecific(Visuals* visuals) = 0;
		virtual void specificDestroy() = 0;
	protected:
		bool initialized = false;
		Scene* currentScene = NULL;
		int countedFrames = 0;
		void loadScene(ChangeSceneAction* args, Visuals* v);
};

#endif