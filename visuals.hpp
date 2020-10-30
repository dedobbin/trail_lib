#ifndef __SDL_H__
#define __SDL_H__

#include <string>
#include <vector>
#include <unordered_map>
#include <SDL2/SDL.h> 
#include <SDL2/SDL_ttf.h>
#include "inventory.hpp"
#include<map>

#include "action.hpp"
#include "type.hpp"
#include "character.hpp"
#include "location.hpp"

//Controls how fast speed draws
#define DEFAULT_TEXT_SPEED_DIVIDER 1

#define COLOR_DEFAULT {255, 255, 255}
#define COLOR_DANGER_1 {0xFF, 0x80, 0x80}
#define COLOR_DANGER_2 {255, 0, 0}
#define COLOR_SELECTION {255, 0, 255}

#define DEFAULT_TEXT_PADDING 10

inline bool operator==(const SDL_Rect& a, const SDL_Rect& b)
{
    return a.x==b.x && a.y==b.y && a.w==b.w && a.h==b.h;
}

enum Direction{
	UP,
	RIGHT,
	DOWN,
	LEFT
};

typedef struct Sprite{
	SDL_Rect src;
	SDL_Rect dst;
	std::string spriteSheetPath;
	SDL_RendererFlip flip = SDL_FLIP_NONE;
} Sprite;


class Background{
	public:
		Background(std::string path, SDL_Rect src);
		Background(Background* background);
		const std::string path;
		const SDL_Rect src;
		SDL_Rect dst;
		/* if solidColor is true, will ignore texture and draw color */
		bool solidColor = false;
		SDL_Color color = {255, 0, 255, 255};
};

class Renderer
{
	public:
		/* bg is double ptr, so the list entry its pointing to can later be inserted */
		/* same for sprites. this way they can be added after init is done */
		Renderer(
			rendererType_t type,
			SDL_Rect viewPort, 
			std::vector<Sprite*>* sprites = NULL
		);
		~Renderer();
		
		/* Should be overwritten by children */
		virtual void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		
		void loadBackgroundTexture(SDL_Renderer* r);
		void renderBackground(SDL_Renderer* r);
		const void renderSprites(SDL_Renderer* r) const;
		const SDL_Rect viewPort;
		
		void setBackground(Background* background);
		void setBackgroundScroll(int speed, Direction dir = LEFT);

		void DEBUG_setCurPos(SDL_Rect pos, bool print = false);

		const rendererType_t type;

		SDL_Rect DEBUG_prevPos;

	protected:
		/* returns width in pixels of text drawn */
		const SDL_Rect renderText(std::string text, SDL_Color color, int x, int y, SDL_Renderer* r, TTF_Font* f) const;

		const int getRelativeCharW(int charH) const;
		const int getRelativeCharH(int charW) const;
	private:
		static SDL_Texture* surfaceToTexture(SDL_Surface*);
		static SDL_Surface* createSurface(int w, int h);
		Background* background = NULL;
		std::vector<Sprite*>* sprites;
		Direction backgroundScrollDir;
		int backgroundScrollSpeed = 0;
		bool ignoreViewPort = false;
};

class ConvoRenderer : public Renderer
{
	public: 
		ConvoRenderer(int* selectedAnswer, SDL_Rect viewPort, int textPaddingX = DEFAULT_TEXT_PADDING);
		void setText(std::string text, std::map<int, std::string> answers, bool prompt = false, int renderSpeedDivider = DEFAULT_TEXT_SPEED_DIVIDER);
		void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		void setPromptText(std::string text);
		bool entireConvoTextIsRendered() const;

		/* Controls how fast convo text is draw, 1 is fastest, 0 will be used to display text instantly */
		int renderSpeedDivider = DEFAULT_TEXT_SPEED_DIVIDER;
		int textPadding = 5;
		int delayAfterConvoRendered = 10;

	private:
		std::string text;
		std::map<int, std::string> answers;
		std::string promptText = "";
		void renderConvo(SDL_Renderer* r, TTF_Font* _f);
		int* selectedAnswer;
		void resetConvoTextRender();
		bool prompt = false;
};

class InventoryRenderer : public Renderer
{
	public:
	 	InventoryRenderer(Inventory* inventory, int* selectedAnswer, SDL_Rect viewPort);
		void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		int textPaddingY = 0;
		int textPaddingX = 0;
		SDL_Rect submenuPos;
		SDL_Rect infoPos;

	private:
		Inventory* inventory;
		const void renderInventory(SDL_Renderer* r, TTF_Font* _f) const;
		const void renderSubmenu(SDL_Renderer* _r, TTF_Font* _f) const;
		const void renderItemInfo(SDL_Renderer* _r, TTF_Font* _f) const;
	protected:
		int* selectedAnswer;
};

class TransitionRenderer : public Renderer
{
	public:
		/* action to trigger when done */
		TransitionRenderer(ChangeSceneAction* action, SDL_Rect viewPort, int length = 1);
		void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		/* return action if done, NULL if not done */
		ChangeSceneAction* done();
	private:
		int counter = 0;	
		ChangeSceneAction* a;
		const uint8_t length;
};

/**
 * Renders weather effect and time of day
 * When extending and overloading renderSpecifics, 
 * 	should call parent
 */
class AtmosphereRenderer : public Renderer
{
	public:
		AtmosphereRenderer(SDL_Rect viewPort, LocationData* locationData);
		void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		/* returns false if nothing is done, that way children can act if they so please */
		bool renderWeather(SDL_Window* w, SDL_Renderer* r);
		int animationSpeedDivider = 7;
		bool animationPaused = false;
	protected:
		LocationData* locationData = NULL;
		int counter = 0;
		std::vector<SDL_Point> points;
	private:
		void animationStep();
};

class StatusBarRenderer : public Renderer
{
	public:
		StatusBarRenderer(SDL_Rect viewPort, std::string locationName, Character* player = NULL);
		void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		void setBlackout(bool blackout = true);
	private:
		/* should be overloaded by children, but falls back to drawing player status (if player was not set, rip) */
		virtual void renderStatusBarSpecific(SDL_Renderer*_r, TTF_Font* _f, int x = 0, int y = 0);

		void renderStatusBar(SDL_Renderer* r, TTF_Font* _f);
		void renderPlayerStatus(SDL_Renderer*_r, TTF_Font* _f, int x = 0, int y = 0);
		std::string locationName;
		bool blackout = false;
		Character* player = NULL;
};

class FadeoutRenderer : public Renderer
{
	public:
		/* action to trigger when done */
		FadeoutRenderer(ChangeSceneAction* action, SDL_Rect viewPort, uint8_t speed = 1);
		void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		/* return action if done, NULL if not done */
		ChangeSceneAction* done();
	private:
		uint8_t darkness = 7;	//init bump so player knows what happening	
		ChangeSceneAction* a;
		const uint8_t speed;
};

class Visuals
{
	public:
		Visuals(int screenW, int screenH, std::string fontPath, int fontSize);
		~Visuals();
        void setScreenSize(int w, int h);
        std::pair<int, int> getScreenSize();


		/* Client should set this in Game::setDefaultRendererBackgrounds overload, when key of renderer in here, it will always use this bg */
		static std::unordered_map<rendererType_t, Background*> defaultRendererBackgrounds;

		/* When adding, removing, getting of checking for specific renderer, make sure it's in renderPrioMap, should be done in Game::SpecificInit() */
		void addRenderer (std::string rName, Renderer* renderer);
		Renderer* getRenderer(std::string rName);
		bool removeRenderer(std::string rName);
		bool hasRenderer(std::string rName);
		void removeAllRenderers();

		bool render() const;
		
		bool addRenderPrio(std::string rendererName, int prio);

		static std::pair<int, int> imageSize(std::string path);

		static Background* createBackground(std::string path);

		static int renderCircle(SDL_Renderer* renderer, int x, int y, int radius);

		/* lazy loads, gets from backgroundTextureMap, if its not there checks HD, if renderer is passed */
		static SDL_Texture* loadBackgroundTexture(std::string path, SDL_Renderer* _r = NULL);
		/* lazy loads */
		static SDL_Texture* loadSpriteSheetTexture(std::string path, SDL_Renderer* _r = NULL);

		static SDL_Texture* loadTexture(std::string path, SDL_Renderer* r);
		static std::vector<std::string> breakLines(TTF_Font* _f, std::string text, SDL_Rect viewPort, int textPaddingX = 0);


        int screenW;
        int screenH; 
        std::string fontPath;
        int fontSize;

        /* optional to use for client, sizes set in setScreenSize() */
        static SDL_Rect VIEW_PORT_FULL_SCREEN;
        static SDL_Rect VIEW_PORT_DEFAULT_CONVO_BAR;
        static SDL_Rect VIEW_PORT_DEFAULT_STATUS_BAR;
        static SDL_Rect VIEW_PORT_DEFAULT_INVENTORY;
        static SDL_Rect VIEW_PORT_DEFAULT_CONTENT;

	private:
		std::map<sceneType_t, int> renderPrioMap;
		std::map<int, Renderer*> renderers;
		SDL_Window* _w = NULL;
		SDL_Renderer* _r = NULL;
		TTF_Font * _f = NULL;
		bool createWindow(std::string title = "Trail");
		bool initSDL();
		const void renderClear() const;
		void renderPresent() const;
		/* key = path */
		static std::unordered_map<std::string, SDL_Texture*> backgroundTextureMap;
		/* key = path */
		static std::unordered_map<std::string, SDL_Texture*> spriteSheetTextureMap;
};

#endif

