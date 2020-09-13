#include <iostream>
#include <unordered_map>
#include <sstream>
#include <string>
#include <SDL2/SDL_image.h>
#include <sys/stat.h>
#include <exception>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <arpa/inet.h> //ntohl
#include <dump_lib.hpp>

#include "visuals.hpp"
#include "time.hpp"

#define BLINK_DIVIDER 20
static bool renderBlink = false;

//TODO: maybe wrap in struct, together with convobox
static int counter = 0;
static int currentRenderedConvoChars = 0;
static std::vector<std::string> convoLines;
bool entireConvoTextRendered = false;

/* after all convo text is rendered, some delay before being able to continue */
static int ticksAfterAllConvoRendered = 0;
#define DELAY_AFTER_CONVO_RENDERERED 10

std::pair<int, int> Visuals::imageSize(std::string path)
{
	//TODO: assert png
	std::ifstream img(path);
	int w, h;
	img.seekg(16);
 	img.read((char *)&w, 4);
    img.read((char *)&h, 4);

	return {ntohl(w), ntohl(h)};
}

Renderer::Renderer(SDL_Rect viewPort, Background* _background, std::vector<Sprite*>* sprites, std::unordered_map<std::string, SDL_Texture*>* spriteSheets)
:viewPort(viewPort), sprites(sprites), spriteSheets(spriteSheets)
{
	if (_background){
		_background->dst = {0, 0, viewPort.w, viewPort.h};
		background = _background;
	}
}

Renderer::~Renderer()
{
	/* bg is owned by scene, don't free here */
	// if (background){
	// 	delete(background);
	// 	background = NULL;
	// }
	
	if (backgroundTexture){
		SDL_DestroyTexture(backgroundTexture);
	}
	
	/* sprites are owned by scene, don't free here */
	// if (sprites){
	// 	for (auto sprite : *sprites){
	// 		delete(sprite);
	// 	}
	// 	sprites = NULL;
	// }	
}

void Renderer::loadBackgroundTexture(SDL_Renderer* _r)
{
	if (!_r) throw std::runtime_error("Trying to set bg texture but SDL_Renderer is not set");
	struct stat buffer;
	bool fileExists =  (stat (background->path.c_str(), &buffer) == 0);
	if (!fileExists){
		std::cerr << "Unable to load background from: " << background->path << std::endl;
		exit(1);
	} else {
		if (backgroundTexture)	SDL_DestroyTexture(backgroundTexture);

		backgroundTexture = Renderer::loadTexture(background->path, _r);
	}
}

const void InventoryRenderer::renderInventory(SDL_Renderer* _r, TTF_Font* _f) const
{
	//TODO: make transparent optional
	SDL_SetRenderDrawColor( _r, 0, 0, 0, 255 );
	SDL_RenderFillRect( _r, &viewPort );

	int y = textPadding;
	int x = textPadding;
	SDL_Color c;
	int fontH = DEFAULT_FONT_H; 
	int j = 0;
	for (auto it : inventory->getItems()){
		Item* item = it.second.first;
		int amount = it.second.second;
		c = COLOR_DEFAULT;
		if (!*itemIsSelected && *selectedAnswer == j){
			c = COLOR_SELECTION;
		}
		renderText(item->type, c, x, y, fontH, _r, _f);
		renderText(std::to_string(amount), COLOR_DEFAULT, x + 180, y, fontH, _r, _f);
		y += fontH;
		j++;
	}
	if (*itemIsSelected){
		renderItemIsSelected(_r, _f);
	}
}

const int Renderer::renderText(std::string text, SDL_Color color, int x, int y, int h, SDL_Renderer* _r, TTF_Font* _f, bool ignoreViewPort) const
{
	if (!ignoreViewPort){
		x+= viewPort.x;
		y+= viewPort.y;
	}

	int textW = text.length() * getRelativeCharW(h);
	SDL_Rect textRect = {x, y, textW, h};
	if (x < viewPort.x 
		|| textRect.x + textRect.w > viewPort.x + viewPort.w
		|| y < viewPort.y 
		|| textRect.y + textRect.h > viewPort.y + viewPort.h

	){
		std::cerr << "renderText: outside of viewport" << std::endl;
		return 0;
	}

	SDL_Surface* textSurface = TTF_RenderText_Solid(_f, text.c_str(), color);
	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(_r, textSurface);

	SDL_RenderCopy(_r, textTexture, NULL, &textRect); 
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);
	return textW;		
}

void Renderer::scrollBackground(Direction direction, int n)
{	
	/* renderBackground will take care of tiling + reseting pos when needed, so don't worry about it here */
	switch (direction){
		case (RIGHT):
			background->dst.x += n;
			break;
		case (DOWN):
			background->dst.y += n;
			break;
		case (LEFT):
			background->dst.x -= n;
			break;
		case (UP):
			background->dst.y -= n;
			break;
		default:
			std::cerr << "moving background unknown direction " << std::endl;
	}
}

const void Renderer::renderSprites(SDL_Renderer* _r) const
{
	if (!sprites)	return;
	
	for (auto& it: *sprites){

		/* calc offset based on viewport */
		SDL_Rect dst = {
			it->dst.x + viewPort.x, 
			it->dst.y + viewPort.y,
			it->dst.w,
			it->dst.h
		};

		SDL_Rect src = it->src;

		/* Don't render parts of sprites outside of viewport*/
		SDL_Rect intersect;
		if (SDL_IntersectRect(&viewPort, &dst, &intersect)){
			dst = intersect;
			if (intersect.w < it->dst.w){
				float intersectFrag = (float)intersect.w / (float)it->dst.w;
				float nonIntersectFrag = 1.0 - intersectFrag;
				
				if (it->dst.x > 0){//right side of port
					//std::cout << "DEBUG: sprite partial outside of viewport: RIGHT" << std::endl;
					src.x += (float)src.w * nonIntersectFrag;
				}
				else {//left side of port
					//std::cout << "DEBUG: sprite partial outside of viewport: LEFT" << std::endl;
					src.w *= intersectFrag;
					src.x *= nonIntersectFrag;
				}
			}
			if (intersect.h < it->dst.h){
				float intersectFrag = (float)intersect.h / (float)it->dst.h;
				float nonIntersectFrag = 1.0 - intersectFrag;
				
				if (it->dst.y > 0){//bottom of port
					//std::cout << "DEBUG: sprite partial outside of viewport: BOTTOM" << std::endl;
					src.h *= intersectFrag;
					src.y *= nonIntersectFrag;
				}
				else {//top of port
					//std::cout << "DEBUG: sprite partial outside of viewport: TOP" << std::endl;
					src.y += (float)src.h * nonIntersectFrag;
				}
			}
			
			SDL_Texture* texture = (*spriteSheets)[it->spriteSheet];
			if (SDL_RenderCopyEx( _r, texture, &src, &dst , NULL, NULL, it->flip) < 0){
				std::cerr << "Tried to render sprite from non-existing spritesheet: '" <<it->spriteSheet << "'" << std::endl;
			}	
		}
	}
}

bool Visuals::entireConvoTextIsRendered() const
{
	return entireConvoTextRendered;
}

void ConvoRenderer::renderConvo(SDL_Renderer* _r, TTF_Font* _f)
{
	/* endless spaghetti in this function */
	SDL_SetRenderDrawColor( _r, 0, 0, 0, 255 );
	SDL_RenderFillRect( _r, &viewPort );

	/* for stuff like waiting for text promp and arrow when text without answers is done drawing */
	if (counter % BLINK_DIVIDER == 0) renderBlink = !renderBlink;

	/* char for char */
	if (!entireConvoTextRendered)
	{
		/* when divider is set to 0 draw instantly*/
		if (renderSpeedDivider == 0){
			currentRenderedConvoChars = text.size();
			entireConvoTextRendered = true;
		} else if (counter % renderSpeedDivider == 0){
			currentRenderedConvoChars ++;
			
			if (currentRenderedConvoChars >= text.size()){
				ticksAfterAllConvoRendered ++;
				if (ticksAfterAllConvoRendered > DELAY_AFTER_CONVO_RENDERERED){
					entireConvoTextRendered = true;
					ticksAfterAllConvoRendered = 0;
				} 
			}
		}
	} 
	std::string textSeg = text.substr(0, currentRenderedConvoChars);
	
	//TODO: don't do every frame
	int charsPerLine = (viewPort.w - textPadding) / (fontH / FONT_CHAR_W_H_RATIO);
	int nLines = textSeg.size() / charsPerLine + 1;

	if (convoLines.empty()){
		convoBox = {textPadding, textPadding, viewPort.w - textPadding, viewPort.h - textPadding};
		auto convoWords = dump_lib::str_split(text, ' ');
		int j = 0;
		int curLen = 0;
		int curLine = 0;
		convoLines.push_back("");
		int lineOffset = 0;
		for(auto word: convoWords){
			std::string toRender = "";
			if (word != "<br/>"){
				toRender = word + " ";
				curLen += toRender.size();
				convoLines[curLine] += toRender;
			} 
			
			/* breakline, + 1 for space */
			if (word =="<br/>" 
			|| convoWords.size() - 1 > j && (curLen - lineOffset + convoWords[j +1].size() + 1)* (fontH / FONT_CHAR_W_H_RATIO) > viewPort.w - textPadding
			){
				//std::cout << "DEBUG: break at " << convoWords[j] << std::endl;
				lineOffset += convoLines[curLine].size();
				convoLines.push_back("");
				curLine++;
			}
			j++;
		}
	}

	int i = 0;
	int lineOffset = 0;
	for(auto line : convoLines){
		int len = renderText(
			line.substr(0, currentRenderedConvoChars - lineOffset), 
			COLOR_DEFAULT, 
			convoBox.x,
			convoBox.y + i * fontH, 
			fontH,
			_r, _f
		);
		
		if (!prompt){
			// TODO: can fall off screen now 
			if (renderBlink && entireConvoTextRendered && answers.empty() && i == convoLines.size()-1){
				len = renderText(
					" >", COLOR_DEFAULT, 
					convoBox.x + len,
					convoBox.y + i * fontH, 
					fontH,
					_r, _f
				);
			}
		}
		
		i++;
		lineOffset += line.size();
		if (lineOffset > currentRenderedConvoChars)
			break;
	}

	if (entireConvoTextRendered){
		int i = 0;
		int y = viewPort.h - fontH - textPadding;
		int x = textPadding;
		for (auto answer : answers){
			SDL_Color answerColor = COLOR_DEFAULT;
			if (answer.first == *selectedAnswer){
				answerColor = COLOR_SELECTION;
			} 
			renderText(answer.second, answerColor, 
				x, 
				y, 
				fontH,
				_r, _f
			);
			x+= convoBox.w / answers.size();
		}
		if (prompt) {
			if (promptText == ""){
				if (renderBlink){
					renderText("_", COLOR_DEFAULT,
						convoBox.x + 25, 
						y, 
						fontH,
						_r, _f
					);
				}
			} else {
				renderText(promptText, COLOR_DEFAULT,
					convoBox.x + 25, 
					y, 
					fontH,
					_r, _f
				);
			}
		}
	}

	counter ++;
}

void Renderer::renderBackground(SDL_Renderer* _r)
{
	if (!background || background->path == "") return;
	
	if (!backgroundTexture){
		loadBackgroundTexture(_r);
	}

	/* calc offset based on viewport */
	SDL_Rect dst = {
		background->dst.x + viewPort.x, 
		background->dst.y + viewPort.y,
		background->dst.w,
		background->dst.h
	};
	SDL_Rect src = background->src;

	/* loop/tile BG, for scrolling esp */
	SDL_Rect intersect;
	if (SDL_IntersectRect(&viewPort, &dst, &intersect)){
		dst = intersect;
		
		if (intersect.w < background->dst.w){
			float intersectFrag = (float)intersect.w / (float)background->dst.w;
			float nonIntersectFrag = 1.0 - intersectFrag;
			if (background->dst.x > 0){
				//std::cout << "DEBUG: scrolling right " << std::endl;
				
				src.w *= intersectFrag;
				src.x *= nonIntersectFrag;

				/* tile */
				SDL_Rect tileDst = {
					viewPort.x,
					dst.y,
					viewPort.w - intersect.w,
					dst.h,
				};

				SDL_Rect tileSrc ={
					background->src.x + background->src.w * intersectFrag,
					background->src.y,
					background->src.w - background->src.w * intersectFrag,
					background->src.h, 

				};
				if (SDL_RenderCopy( _r, backgroundTexture, &tileSrc, &tileDst ) < 0){
					std::cerr << "Could not render background" << std::endl;
				}
			
			} else {
				//std::cout << "DEBUG: scrolling left " << std::endl;
				
				src.x += (float)src.w * nonIntersectFrag;

				/* tile */
				SDL_Rect tileDst = {
					viewPort.x + intersect.w,
					viewPort.y,
					viewPort.w - intersect.w,
					viewPort.h,
				};

				SDL_Rect tileSrc ={
					background->src.x,
					background->src.y,
					background->src.w - background->src.w * intersectFrag,
					background->src.h, 
				};
				if (SDL_RenderCopy( _r, backgroundTexture, &tileSrc, &tileDst ) < 0){
					std::cerr << "Could not render background" << std::endl;
				}
			}
		}
		
		if (intersect.h < background->dst.h){
			float intersectFrag = (float)intersect.h / (float)background->dst.h;
			float nonIntersectFrag = 1.0 - intersectFrag;
			if (background->dst.y > 0){
				//std::cout << "DEBUG: scrolling down " << std::endl;
				
				src.h *= intersectFrag;
				src.y *= nonIntersectFrag;

				/* tile */
				SDL_Rect tileDst = {
					dst.x,
					viewPort.y,
					dst.w,
					viewPort.h - intersect.h,
				};

				SDL_Rect tileSrc ={
					background->src.x,
					background->src.y + background->src.h * intersectFrag,
					background->src.w,
					background->src.h - background->src.h * intersectFrag, 

				};
				if (SDL_RenderCopy( _r, backgroundTexture, &tileSrc, &tileDst ) < 0){
					std::cerr << "Could not render background" << std::endl;
				}

			} else {
				//std::cout << "DEBUG: scrolling up " << std::endl;
				
				src.y += (float)src.h * nonIntersectFrag;
				
				/* tile */
				SDL_Rect tileDst = {
					viewPort.x,
					viewPort.y + intersect.h,
					viewPort.w,
					viewPort.h - intersect.h
				};

				SDL_Rect tileSrc ={
					background->src.x,
					background->src.y,
					background->src.w,
					background->src.h - background->src.h * intersectFrag, 
				};
				if (SDL_RenderCopy( _r, backgroundTexture, &tileSrc, &tileDst ) < 0){
					std::cerr << "Could not render background" << std::endl;
				}
			}
		}

		if (SDL_RenderCopy( _r, backgroundTexture, &src, &dst ) < 0){
			std::cerr << "Could not render background" << std::endl;
		}
	} else {
		/* reset when totally out of view port */
		if (background->dst.x >= viewPort.w){
			background->dst.x = 0;
		} else 	if (background->dst.x + background->dst.w <= 0){
			background->dst.x += background->dst.w;
		}
		if (background->dst.y + background->dst.h <= 0){
			background->dst.y += background->dst.h;
		} else if (background->dst.y - background->dst.h <= 0){
			background->dst.y -= background->dst.h;
		}
		/* Draw again immediately stop stop flicker */
		if (SDL_RenderCopy( _r, backgroundTexture, &background->src, &viewPort ) < 0){
			std::cerr << "Could not render background" << std::endl;
		}
		
	}
	// SDL_SetRenderDrawColor(_r, 255, 0, 255, 255);
	// SDL_RenderDrawRect(_r, &viewPort);
}

void ConvoRenderer::resetConvoTextRender()
{
	entireConvoTextRendered = false;
	currentRenderedConvoChars = 0;
	convoLines = {};
}

//TODO: place in Visuals
SDL_Texture* Renderer::loadTexture( std::string path, SDL_Renderer* r)
{
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == NULL) {
        std::cerr << sprintf("Graphics: Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
    }
    else{
        // 0x64, 0x64, 0x64 is color that will be used for transparency because it's somewhat easy to remember
        SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0x64, 0x64, 0x64));
        newTexture = SDL_CreateTextureFromSurface(r, loadedSurface);

        if (newTexture == NULL) {
            std::cerr << sprintf("Graphics: Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
        }
        SDL_FreeSurface(loadedSurface);
    }

    return newTexture;
}

const void Visuals::renderClear() const
{
    SDL_SetRenderDrawColor( _r, 148, 62, 62, 255 );
    SDL_RenderClear( _r );
}

void Visuals::loadSpriteSheet(std::string name, std::string path)
{
	if (spriteSheets.find(name) != spriteSheets.end()){
		std::cout << "Debug: Spritesheet '" << name << "' already loaded" << std::endl;
		return;
	}
	spriteSheets[name] = Renderer::loadTexture(path, _r);
}

void Visuals::renderPresent() const
{
	SDL_RenderPresent(_r);
}

bool Visuals::render() const
{
	renderClear();
	for (auto renderer : renderers){
		renderer.second->renderSpecifics(_w, _r, _f);
		renderer.second->renderBackground(_r);
		renderer.second->renderSprites(_r);
	}
	renderPresent();
}
void Visuals::addRenderer(std::string rName, Renderer* _renderer)
{
	
	if (renderPrioMap.find(rName) == renderPrioMap.end()){
		std::cerr << "Tried to add renderer that is not in prio list :" << rName << std::endl;
		return;
	}
	int prio = renderPrioMap.at(rName);

	std::cout << "DEBUG: added renderer '"<< rName <<" 'with prio " << prio << std::endl;
	if (removeRenderer(rName)){
		std::cout << "DEBUG overwriting renderer with prio " << prio << std::endl;
	}
	renderers[prio] = _renderer;
}


bool Visuals::removeRenderer(std::string rName)
{
	if (renderPrioMap.find(rName) == renderPrioMap.end()){
		std::cerr << "Tried to remove renderer that is not in prio list :" << rName << std::endl;
		return false;
	}
	int key = renderPrioMap.at(rName);

	if (renderers.find(key) != renderers.end()){
		std::cout << "DEBUG: removed renderer with prio " << key << std::endl;
		auto tmp = renderers[key];
		renderers.erase(key);
		delete(tmp);
		return true;
	}
	return false;
}

void Visuals::removeAllRenderers() 
{
	for (auto renderer : renderers){
		free(renderer.second);
	}
	renderers = {};
}

Renderer* Visuals::getRenderer(std::string rName)
{

	if (renderPrioMap.find(rName) == renderPrioMap.end()){
		std::cerr << "Tried to remove renderer that is not in prio list :" << rName << std::endl;
		return NULL;
	}
	int key = renderPrioMap.at(rName);


	if (renderers.find(key) == renderers.end()){
		throw std::runtime_error("Tried to get nonexisting renderer: " + key);
	}
	return renderers[key];
}

bool Visuals::hasRenderer(std::string rName)
{
	if (renderPrioMap.find(rName) == renderPrioMap.end()){
		std::cerr << "checked for renderer that is not in prio list :" << rName << std::endl;
		return false;
	}
	int key = renderPrioMap.at(rName);


	return renderers.find(key) != renderers.end();
}

void Renderer::renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f)
{}


const int Renderer::getRelativeCharW(int charH) const
{
	return charH / FONT_CHAR_W_H_RATIO;
}

const int Renderer::getRelativeCharH(int charW) const
{
	return charW * FONT_CHAR_W_H_RATIO;
}

ConvoRenderer::ConvoRenderer(int* selectedAnswer, SDL_Rect viewPort, int textPadding)
: Renderer(viewPort), selectedAnswer(selectedAnswer), textPadding(textPadding)
{
	fontH = DEFAULT_FONT_H;
	characterW = getRelativeCharW(fontH);
	//convoBox = {0, 0, viewPort.w, viewPort.h};
}

void ConvoRenderer::setText(std::string _text, std::map<int, std::string> _answers, bool _prompt, int _renderSpeedDivider)
{
	text = _text;
	answers = _answers;
	renderSpeedDivider = _renderSpeedDivider;
	prompt = _prompt;
	resetConvoTextRender();
}

void ConvoRenderer::setPromptText(std::string _text)
{
	promptText = _text;
}

void ConvoRenderer::renderSpecifics(SDL_Window* _w, SDL_Renderer* _r, TTF_Font* _f)
{
	renderConvo(_r, _f);
}

InventoryRenderer::InventoryRenderer(Inventory* inventory, int* selectedAnswer, bool* itemIsSelected, SDL_Rect viewPort, int textPadding)
: Renderer(viewPort), selectedAnswer(selectedAnswer), inventory(inventory), itemIsSelected(itemIsSelected), textPadding(textPadding)
{
	std::cout << "DEBUG: SDF" << std::endl;
}

void InventoryRenderer::renderSpecifics(SDL_Window* _w, SDL_Renderer* _r, TTF_Font* _f)
{
	renderInventory(_r, _f);
}

TransitionRenderer::TransitionRenderer(ChangeSceneAction* action, SDL_Rect viewPort, int length)
:Renderer(viewPort), a(action), length(length)
{}

void TransitionRenderer::renderSpecifics(SDL_Window* _w, SDL_Renderer* _r, TTF_Font* _f)
{
	SDL_SetRenderDrawColor(_r, 0, 0, 0, 255);
    SDL_RenderFillRect(_r, &viewPort);
	counter ++;
}

ChangeSceneAction* TransitionRenderer::done()
{
	return counter >= length ? a : NULL;
}

FadeoutRenderer::FadeoutRenderer(ChangeSceneAction* action, SDL_Rect viewPort, uint8_t speed)
:Renderer(viewPort), a(action), speed(speed)
{}

void FadeoutRenderer::renderSpecifics(SDL_Window* _w, SDL_Renderer* _r, TTF_Font* _f)
{
	SDL_SetRenderDrawColor(_r, 0, 0, 0, darkness);
    SDL_RenderFillRect(_r, &viewPort);

	darkness =  darkness + speed > 255 ? 255 : darkness + speed;
}

ChangeSceneAction* FadeoutRenderer::done()
{
	return darkness == 255 ? a : NULL;
}

bool Visuals::initSDL()
{
	if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 ) {
		std::cerr << "Could not init SDL: " << SDL_GetError() << std::endl;	
		return false;
	}
	
	if (!createWindow()){
		return false;
	}

	_r = SDL_CreateRenderer( _w, -1, SDL_RENDERER_ACCELERATED );
	if( _r == NULL ){
		std::cerr << "Could not create renderer: " << SDL_GetError() << std::endl;	
		return false;
	}

	//SDL_SetRenderDrawColor( _r, 0xFF, 0xFF, 0xFF, 0xFF );
	if (SDL_SetRenderDrawBlendMode(_r, SDL_BLENDMODE_BLEND) < 0){
		std::cerr << "Could not set blend mode: " << SDL_GetError() << std::endl;	
		return false;
	}

	int imgFlags = IMG_INIT_PNG;
	if( !( IMG_Init( imgFlags ) & imgFlags ) ){
		std::cerr << "Could not init SDL_image: " << SDL_GetError() << std::endl;	
		return false;
	}
	if( TTF_Init() == -1 ){
		std::cerr << "Could not init SDL_font: " << SDL_GetError() << std::endl;	
		return false;
	}
	_f = TTF_OpenFont( "assets/OpenSans-Regular.ttf", 28 );
	if( _f == NULL ){
		std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;	
        return false;
    }
	 //TODO: init sound
	 return true;
}


bool Visuals::addRenderPrio(std::string rendererName, int prio)
{
	if (renderPrioMap.find(rendererName) != renderPrioMap.end()){
		std::cerr << "Tried to overwrite render prio (new renderer: " << rendererName << std::endl;
		return false;
	}
	renderPrioMap[rendererName] = prio;
	return true;
}


bool Visuals::createWindow(std::string title)
{
    _w = SDL_CreateWindow( title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN );
        if( _w == NULL ){
            std::cerr <<  "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
	return true;
}

Visuals::Visuals()
{
	if (!initSDL()){
		exit(1);
	}
}

Visuals::~Visuals()
{
	//TODO: exit subsytems like font
	for (auto& it: spriteSheets) {
    	SDL_DestroyTexture(it.second);
	}
	for (auto renderer : renderers){
		delete(renderer.second);
	}
    SDL_DestroyRenderer( _r );
    SDL_DestroyWindow( _w );
    _r = NULL;
    _w = NULL;
    IMG_Quit();
    SDL_Quit();
}