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

#define SCREEN_W 1024
#define SCREEN_H 981

#define FONT_CHAR_W_H_RATIO 2
#define DEFAULT_FONT_CHARACTER_W SCREEN_W / 75
#define DEFAULT_FONT_H DEFAULT_FONT_CHARACTER_W * FONT_CHAR_W_H_RATIO

//Controls how fast speed draws
#define DEFAULT_TEXT_SPEED_DIVIDER 1

#define COLOR_DEFAULT {255, 255, 255}
#define COLOR_DANGER_1 {0xFF, 0x80, 0x80}
#define COLOR_DANGER_2 {255, 0, 0}
#define COLOR_SELECTION {255, 0, 255}

#define DEFAULT_TEXT_PADDING 10

/* optional to use for client */
#define VIEW_PORT_FULL_SCREEN {0, 0, SCREEN_W, SCREEN_H}
#define VIEW_PORT_DEFAULT_CONVO_BAR {0, SCREEN_H - SCREEN_H/6, SCREEN_W, SCREEN_H /6}
#define VIEW_PORT_DEFAULT_STATUS_BAR {0, 0, SCREEN_W, SCREEN_H/9}

enum Direction{
	UP,
	RIGHT,
	DOWN,
	LEFT
};

typedef struct Sprite{
	SDL_Rect src;
	SDL_Rect dst;
	std::string spriteSheet;
	SDL_RendererFlip flip = SDL_FLIP_NONE;
} Sprite;

typedef struct Background{
	std::string path;
	SDL_Rect src = {0, 0, SCREEN_W, SCREEN_H};
	SDL_Rect dst = {0, 0, SCREEN_W, SCREEN_H};
} Background;


class Renderer
{
	public:
		Renderer(
			SDL_Rect viewPort = VIEW_PORT_FULL_SCREEN, 
			Background* bg = NULL, 
			std::vector<Sprite*>* sprites = NULL, 
			std::unordered_map<std::string, SDL_Texture*>* spriteSheets = NULL
		);
		~Renderer();
		
		/* Should be overwritten by children */
		virtual void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		
		void renderBackground(SDL_Renderer* r);
		const void renderSprites(SDL_Renderer* r) const;
		void scrollBackground(Direction direction, int n);
		static SDL_Texture* loadTexture(std::string path, SDL_Renderer* r);
		const SDL_Rect viewPort = VIEW_PORT_FULL_SCREEN;
	protected:
		/* returns width in pixels of text drawn */
		const int renderText(std::string text, SDL_Color color, int x, int y, int h, SDL_Renderer* r, TTF_Font* f, bool ignoreViewPort = false) const;

		const int getRelativeCharW(int charH) const;
		const int getRelativeCharH(int charW) const;
		Background* background = NULL;
	private:
		void loadBackgroundTexture(SDL_Renderer* r);
		static SDL_Texture* surfaceToTexture(SDL_Surface*);
		static SDL_Surface* createSurface(int w, int h);
		SDL_Texture* backgroundTexture = NULL;
		std::vector<Sprite*>* sprites;
		std::unordered_map<std::string, SDL_Texture*>* spriteSheets;
};

class ConvoRenderer : public Renderer
{
	public: 
		ConvoRenderer(int* selectedAnswer, SDL_Rect viewPort, int textPadding = DEFAULT_TEXT_PADDING);
		void setText(std::string text, std::map<int, std::string> answers, bool prompt = false, int renderSpeedDivider = DEFAULT_TEXT_SPEED_DIVIDER);
		void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
		void setPromptText(std::string text);
		const int textPadding;
	private:
		int fontH;
		int characterW;
		int convoBoxH;
		SDL_Rect convoBox;

		std::string text;
		std::map<int, std::string> answers;
		std::string promptText = "";
		void renderConvo(SDL_Renderer* r, TTF_Font* _f);
		int* selectedAnswer;
		void resetConvoTextRender();
		/* Controls how fast convo text is draw, 1 is fastest, 0 will be used to display text instantly */
		int renderSpeedDivider = DEFAULT_TEXT_SPEED_DIVIDER;
		bool prompt = false;
};

class InventoryRenderer : public Renderer
{
	public:
	 	InventoryRenderer(Inventory* inventory, int* selectedAnswer, bool* itemIsSelected, SDL_Rect viewPort, int TextPadding = DEFAULT_TEXT_PADDING);
		void renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f);
	private:
		Inventory* inventory;
		const void renderInventory(SDL_Renderer* r, TTF_Font* _f) const;
		/* Needs to be overloaded so a client can decide if is player, party, or whatever,this determines what is done when item is selected in inventory */
		virtual const void renderItemIsSelected(SDL_Renderer* _r, TTF_Font* _f) const = 0;
	protected:
		int* selectedAnswer;
		bool* itemIsSelected;
		int textPadding = 0;
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
		Visuals();
		~Visuals();

		/* When adding, removing, getting of checking for specific renderer, make sure it's in renderPrioMap, should be done in Game::SpecificInit() */
		void addRenderer (std::string rName, Renderer* renderer);
		Renderer* getRenderer(std::string rName);
		bool removeRenderer(std::string rName);
		bool hasRenderer(std::string rName);
		void removeAllRenderers();

		bool render() const;
		void loadSpriteSheet(std::string name, std::string path);
		std::unordered_map<std::string, SDL_Texture*> spriteSheets;
		bool entireConvoTextIsRendered() const;
		
		bool addRenderPrio(std::string rendererName, int prio);

		static std::pair<int, int> imageSize(std::string path);
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
};

#endif

