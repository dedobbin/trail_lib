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
#include <assert.h>
#include <arpa/inet.h> //ntohl
#include <dump_lib.hpp>

#include "visuals.hpp"
#include "time.hpp"

/* stuff for convo renderer */
static int counter = 0;
static int currentRenderedConvoChars = 0;
static std::vector<std::string> convoLines;
bool entireConvoTextRendered = false;
SDL_Rect convoSrcRect = {0, 0, 0, 0};
std::vector<int> currentRenderedConvoCharsOnLine;
int currentConvoLine = 0;

/* after all convo text is rendered, some delay before being able to continue */
int ticksAfterAllConvoRendered = 0;

/* blink cursor arrow thing */
#define BLINK_DIVIDER 20
static bool renderBlink = false;


Background::Background(std::string path, SDL_Rect src)
:path(path), src(src)
{}

Background::Background(Background* _background)
:path(_background->path), src(_background->src)
{}

std::vector<std::string> specialTokens = {"<br/>"};

/* init static stuff */
std::unordered_map<rendererType_t, Background*> Visuals::defaultRendererBackgrounds = {};
std::unordered_map<std::string, SDL_Texture*> Visuals::backgroundTextureMap = {};
std::unordered_map<std::string, SDL_Texture*> Visuals::spriteSheetTextureMap = {};

int Visuals::renderCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
   	int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius -1;
    status = 0;

    while (offsety >= offsetx) {

        status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
                                     x + offsety, y + offsetx);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
                                     x + offsetx, y + offsety);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
                                     x + offsetx, y - offsety);
        status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
                                     x + offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2*offsetx) {
            d -= 2*offsetx + 1;
            offsetx +=1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}


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

Background* Visuals::createBackground(std::string path)
{
	auto size = Visuals::imageSize(path);
	return new Background(path, {0, 0, size.first, size.second});
}

Renderer::Renderer(rendererType_t type, SDL_Rect viewPort, std::vector<Sprite*>* sprites)
:viewPort(viewPort), sprites(sprites), type(type)
{}

Renderer::~Renderer()
{	
	if (background){
		delete(background);
	}

	/* sprites are owned by scene, don't free here */
	// if (sprites){
	// 	for (auto sprite : *sprites){
	// 		delete(sprite);
	// 	}
	// 	sprites = NULL;
	// }	
}

void Renderer::setBackground(Background* bg)
{
	if (background){
		free(background);
	}
	background = bg;
	background->dst = {0, 0, viewPort.w, viewPort.h};
}

SDL_Texture* Visuals::loadBackgroundTexture(std::string path, SDL_Renderer* _r)
{
	if (backgroundTextureMap.count(path) > 0){
		return backgroundTextureMap[path];
	} else if (_r){
		struct stat buffer;
		bool fileExists =  (stat (path.c_str(), &buffer) == 0);
		if (fileExists){
			auto tex = Visuals::loadTexture(path, _r);
			backgroundTextureMap[path] = tex;
			return tex;
		}
	}
	std::string errorStr = "Unable to load background \"" + path + "\" from" + (_r ?  " disk" : "backgroundTextureMap");
	std::cerr << errorStr << std::endl;
	exit(1);
	return NULL;
}

SDL_Texture* Visuals::loadSpriteSheetTexture(std::string path, SDL_Renderer* _r)
{
	if (Visuals::spriteSheetTextureMap.find(path) != Visuals::spriteSheetTextureMap.end()){
		return spriteSheetTextureMap[path];
	} else if (_r){
		struct stat buffer;
		bool fileExists =  (stat (path.c_str(), &buffer) == 0);
		if (fileExists){
			auto tex = Visuals::loadTexture(path, _r);
			spriteSheetTextureMap[path] = tex;
			return tex;
		}
	}
	std::string errorStr = "Unable to load spritesheet \"" + path + + "\" from " + (_r ?  " disk" : "spriteSheetTextureMap");
	std::cerr << errorStr << std::endl;
	exit(1);
	return NULL;
}

const void InventoryRenderer::renderInventory(SDL_Renderer* _r, TTF_Font* _f) const
{
	int y = textPaddingY;
	int x = textPaddingX;
	int h;
	SDL_Color c;
	int j = 0;
	for (auto it : inventory->getItems()){
		int amount = it.second.second;
		c = COLOR_DEFAULT;
		if (inventory->submenuIsOpen()){
			if (it.second.first == inventory->itemWithSubMenuOpen){
				c = COLOR_SELECTION;
			}
		} else if (*selectedAnswer == j){
			c = COLOR_SELECTION;
			
		}

		auto item = it.second.first;
		SDL_Rect dst = renderText(item->type, c, x, y, _r, _f);
		renderText(std::to_string(amount), COLOR_DEFAULT, viewPort.w / 2, y, _r, _f);
		y += dst.h;
		j++;
	}
}

const SDL_Rect Renderer::renderText(std::string text, SDL_Color color, int x, int y, SDL_Renderer* _r, TTF_Font* _f) const
{
	if (!ignoreViewPort){
		x+= viewPort.x;
		y+= viewPort.y;
	};

	SDL_Rect dst = {x, y, 0, 0};
	TTF_SizeText(_f,text.c_str(),&dst.w,&dst.h);

	if (x < viewPort.x 
		|| dst.x + dst.w > viewPort.x + viewPort.w
		|| y < viewPort.y 
		|| dst.y + dst.h > viewPort.y + viewPort.h

	){
		std::cerr << "renderText: outside of viewport (" << text << ")" << std::endl;
		return {0,0,0,0};
	}

	SDL_Surface* textSurface = TTF_RenderText_Solid(_f, text.c_str(), color);
	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(_r, textSurface);

	SDL_RenderCopy(_r, textTexture, NULL, &dst); 
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);
	
	return dst;		
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
			
			SDL_Texture* texture = Visuals::loadSpriteSheetTexture(it->spriteSheetPath, _r);
			if (SDL_RenderCopyEx( _r, texture, &src, &dst , NULL, NULL, it->flip) < 0){
				std::cerr << "Tried to render sprite from non-existing spritesheet: '" << it->spriteSheetPath << "'" << std::endl;
			}	
		}
	}
}


std::vector<std::string> Visuals::breakLines(TTF_Font* _f, std::string text, SDL_Rect viewPort, int textPaddingX)
{
	std::vector<std::string> result;
	auto words = dump_lib::str_split(text, ' ');
	int j = 0;
	int curLen = 0;
	int curLine = 0;
	result.push_back("");
	int lineOffset = 0;

	int curLineW = 0;
	for (auto it = words.begin(); it != words.end(); it++ ){
		int wordW;
		TTF_SizeText(_f, (*it + " ").c_str(), &wordW, NULL);
		if (*it == "<br/>" || curLineW + wordW > viewPort.w - textPaddingX){
			curLine++;
			currentRenderedConvoCharsOnLine.push_back(0);
			result.push_back("");
			curLineW = 0;
		}
		if (std::find(specialTokens.begin(), specialTokens.end(), *it) == specialTokens.end()){
			result[curLine]+= *it + " ";
			curLineW += wordW;
		}
	}
	return result;
}

bool ConvoRenderer::entireConvoTextIsRendered() const
{
	return 
		renderSpeedDivider == 0 ||
		( ticksAfterAllConvoRendered >= delayAfterConvoRendered 
		&& entireConvoTextRendered );
}

void ConvoRenderer::renderConvo(SDL_Renderer* _r, TTF_Font* _f)
{	
    int fontH;
    TTF_SizeText(_f, "a", NULL, &fontH);

	/* before anything is drawn, reset */
	if (currentRenderedConvoChars == 0){
        /* bit wonky to reset here and not in scene, but it be like that*/
        if (answers.size() > 0){
            *selectedAnswer = answers.begin()->first;
        } else {
            *selectedAnswer = 0;
        }
		entireConvoTextRendered = false;
		currentRenderedConvoCharsOnLine = {};
		currentRenderedConvoChars = 0;
		convoLines = {};
		currentConvoLine = 0;
		ticksAfterAllConvoRendered = 0;

		convoLines = Visuals::breakLines(_f, text, viewPort, textPadding);

		currentRenderedConvoCharsOnLine.push_back(0);

		/* draw instantly when divider == 0*/
		if (renderSpeedDivider == 0){
			currentConvoLine = convoLines.size() - 1;
			entireConvoTextRendered = true;
		}
	}

	SDL_Rect dst;
	for (int i = 0; i < currentConvoLine + 1; i++){
	 	int n;
		 if (renderSpeedDivider > 0){
			n = currentRenderedConvoCharsOnLine[i];
		} else {
			n = convoLines[i].size();
		}
		dst = renderText(convoLines[i].substr(0, n), COLOR_DEFAULT, textPadding, textPadding + i * fontH, _r, _f);

	}
	
	/* Progress char/line TODO: this is messy because needs to instant render when speeddivider == 0, and also set currentRenderedConvoChars, should clean*/
	if (renderSpeedDivider > 0 && counter % renderSpeedDivider == 0){
		if (currentRenderedConvoCharsOnLine[currentConvoLine] == convoLines[currentConvoLine].size()){
			if (convoLines.size() == currentConvoLine + 1){
				entireConvoTextRendered = true;
			} else {
				currentConvoLine ++;
			}	
		} else {
			currentRenderedConvoCharsOnLine[currentConvoLine]++;
			currentRenderedConvoChars++;
		}
	} else if (renderSpeedDivider == 0){
		currentRenderedConvoChars = text.size();
	}

	if (entireConvoTextRendered){
		if (ticksAfterAllConvoRendered++ < delayAfterConvoRendered){
			//std::cout << "DEBUG: ticksAfterAllConvoRendered: " << ticksAfterAllConvoRendered << std::endl;
		} else if (answers.size() > 0){
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
					_r, _f
				);
				x+= viewPort.w / answers.size();
                i++;
			}
		} else if (prompt){
			int y = viewPort.h - fontH - textPadding;
			int x = textPadding;
			if (promptText == ""){
				if (renderBlink){
					renderText("_", COLOR_DEFAULT,
						x, 
						y, 
						_r, _f
					);
				}
			} else {
				renderText(promptText, COLOR_DEFAULT,
					x, 
					y, 
					_r, _f
				);
			}
		} else if (renderBlink){
			int lineW;
			TTF_SizeText(_f, convoLines[convoLines.size()-1].c_str(), &lineW, NULL);
			renderText(
				"  >", COLOR_DEFAULT, 
				lineW,
				fontH * currentConvoLine + textPadding,
				_r, _f
			);
		}
	}
	
	if (counter % BLINK_DIVIDER == 0) renderBlink = !renderBlink;
	counter ++;
}

void Renderer::setBackgroundScroll(int speed, Direction dir)
{
	backgroundScrollDir = dir;
	backgroundScrollSpeed = speed;
}

void Renderer::renderBackground(SDL_Renderer* _r)
{	
	if (background && background->solidColor){
		auto color = background->color;
		SDL_SetRenderDrawColor( _r, color.r, color.g, color.b, color.a );
		SDL_RenderFillRect( _r, &viewPort );
		return;
	}
	
	//TODO: when there is no bg and no default bg for renderer, this will look for it everytime, make skipable?
	if (!background || background->path == ""){
		if (Visuals::defaultRendererBackgrounds.count(type) > 0){
			/* copy, because will free when renderer deleted */
			setBackground(new Background(Visuals::defaultRendererBackgrounds[type]));
			//std::cout << "DEBUG: USED DEFAULT BG FOR " << type << ": " << background->path << std::endl;
		} 
	}

	if (!background){
		return;
	}

	auto backgroundTexture = Visuals::loadBackgroundTexture(background->path, _r);

	if (backgroundScrollSpeed > 0){
		switch(backgroundScrollDir){
			case UP:
				background->dst.y =- backgroundScrollSpeed;
				break;
			case RIGHT:
				background->dst.x += backgroundScrollSpeed;
				break;
			case DOWN:
				background->dst.y += backgroundScrollSpeed;
				break;
			case LEFT:
				background->dst.x -= backgroundScrollSpeed;
		}
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
		DEBUG_setCurPos(dst);

		if (SDL_RenderCopy( _r, backgroundTexture, &src, &dst ) < 0){
			std::cerr << "Could not render background" << std::endl;
		}

		//DEBUG
		// SDL_SetRenderDrawColor(_r, 255, 0, 255, 255);
		// SDL_RenderFillRect(_r, &dst);
	} 
	
	else {
		background->dst = {0, 0, viewPort.w, viewPort.h};

		/* Draw again immediately stop stop flicker */
		if (SDL_RenderCopy( _r, backgroundTexture, &background->src, &viewPort ) < 0){
			std::cerr << "Could not render background" << std::endl;
		}
	}
}

void Renderer::DEBUG_setCurPos(SDL_Rect pos, bool print){
	print && std::cout << "DEBUG: bg moved " << abs(pos.w - DEBUG_prevPos.w) << std::endl;
	DEBUG_prevPos = pos;
}

void ConvoRenderer::resetConvoTextRender()
{
	entireConvoTextRendered = false;
	currentRenderedConvoChars = 0;
	convoLines = {};
}

SDL_Texture* Visuals::loadTexture( std::string path, SDL_Renderer* r)
{
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == NULL) {
        std::cerr << sprintf("Graphics: Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
    } else {
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

void Visuals::renderPresent() const
{
	SDL_RenderPresent(_r);
}

bool Visuals::render() const
{
	renderClear();
	for (auto renderer : renderers){
		renderer.second->renderBackground(_r);
		renderer.second->renderSpecifics(_w, _r, _f);
		renderer.second->renderSprites(_r);
	}
	renderPresent();
}
void Visuals::addRenderer(std::string rName, Renderer* _renderer)
{
	if (renderPrioMap.find(rName) == renderPrioMap.end()){
		std::cerr << "Tried to add renderer that is not in prio list: " << rName << std::endl;
		return;
	}
	int prio = renderPrioMap.at(rName);

	//std::cout << "DEBUG: added renderer '"<< rName <<" 'with prio " << prio << std::endl;
	if (removeRenderer(rName)){
		//std::cout << "DEBUG overwriting renderer with prio " << prio << std::endl;
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
		//std::cout << "DEBUG: removed renderer with prio " << key << std::endl;
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
		delete(renderer.second);
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

AtmosphereRenderer::AtmosphereRenderer(SDL_Rect viewPort, LocationData* locationData)
: Renderer("ATMOSPHERE", viewPort), locationData(locationData)
{
	/* do 1 step on init, so there is something on screen, even when animation is paused on init (which is currently impossible haha) */
	animationStep();
}

void AtmosphereRenderer::animationStep()
{
	points = {};
	int maxX = viewPort.x + viewPort.w + 10;
	int minX = viewPort.x - 10;
	int maxY = viewPort.y + viewPort.h - 10;
	int minY = viewPort.y + 10;


	int density = locationData->weatherSeverity * 100;
	for (int i = 0; i < density; i++){
		points.push_back({
            dump_lib::random_int_range(minX, maxX),
            dump_lib::random_int_range(minY, maxY)
		});
	}
}

bool AtmosphereRenderer::renderWeather(SDL_Window* _w, SDL_Renderer* _r)
{	
	if (locationData->weather == "NEUTRAL"){
		return false;
	}

	/* animate */
	if (counter % animationSpeedDivider == 0){
		animationStep();
	}

	if (locationData->weather == "RAIN"){
		SDL_SetRenderDrawColor(_r, 0, 0, 255, 255);
		for (auto point : points){
			Visuals::renderCircle(_r, point.x, point.y, 2);
		}
		return true;
	}
	return false;
}

void AtmosphereRenderer::renderSpecifics(SDL_Window* _w, SDL_Renderer* _r, TTF_Font* _f)
{
	if (!locationData->isLocation){
		return;
	}

	if (!locationData->indoors){
		renderWeather(_w, _r);
	}
	if (!animationPaused){
		counter ++;
	}
}

ConvoRenderer::ConvoRenderer(int* selectedAnswer, SDL_Rect viewPort, int textPadding)
: Renderer("CONVO", viewPort), selectedAnswer(selectedAnswer), textPadding(textPadding), delayAfterConvoRendered(10)
{}

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

InventoryRenderer::InventoryRenderer(Inventory* inventory, int* selectedAnswer, SDL_Rect viewPort)
: Renderer("INVENTORY", viewPort), selectedAnswer(selectedAnswer), inventory(inventory)
{
	textPaddingY = 5;
	textPaddingX = 10;

	int infoPosW = viewPort.w / 2  - 40;
	int infoPosX = viewPort.w - infoPosW;

	infoPos = {
		infoPosX, 
		0,
		infoPosW,
		viewPort.h / 2,
	};

	submenuPos = {
		infoPos.x,
		infoPos.h,
		infoPos.w,
		viewPort.h / 2,
	};
}

const void InventoryRenderer::renderSubmenu(SDL_Renderer* _r, TTF_Font* _f) const
{
    int fontH;
    TTF_SizeText(_f, "a", NULL, &fontH);
	//DEBUG
	// SDL_SetRenderDrawColor(_r, 255, 0, 255, 255);
	// SDL_Rect tmp = {viewPort.x + submenuPos.x,viewPort.y + submenuPos.y, submenuPos.w, submenuPos.h};
	// SDL_RenderDrawRect(_r, &tmp);

	int x = submenuPos.x + textPaddingX;
	int y = submenuPos.y + textPaddingY;
	auto submenuData = inventory->itemWithSubMenuOpen->getInventorySubmenuData();

	renderText(submenuData->getText(), COLOR_DEFAULT,x, y, _r, _f);

	auto answers = submenuData->getAnswers();
	for (int i =0; i < answers.size();i++){
		y+= fontH;
		SDL_Color c = COLOR_DEFAULT;
		if (i == *selectedAnswer){
			c = COLOR_SELECTION;
		}
		renderText(answers[i].second,c ,x, y, _r, _f);
	}
}

const void InventoryRenderer::renderItemInfo(SDL_Renderer* _r, TTF_Font* _f) const
{
    int fontH;
    TTF_SizeText(_f, "a", NULL, &fontH);
	//DEBUG
	// SDL_SetRenderDrawColor(_r, 123, 123, 300, 255);
	// SDL_Rect tmp = {viewPort.x + infoPos.x,viewPort.y + infoPos.y, infoPos.w, infoPos.h};
	// SDL_RenderDrawRect(_r, &tmp);

	int x = infoPos.x + textPaddingX;
	int y = textPaddingY;

	Item* toRender = NULL;
	if (!inventory->submenuIsOpen()){
		int i = 0;
		//TODO: use find if or something
		for (auto item : inventory->getItems()){
			if (i == *selectedAnswer){
				toRender = item.second.first;
				break;
			}
			i++;
		}
	} else {
		toRender = inventory->itemWithSubMenuOpen;
	}
	if (!toRender){
		return;
	}
	auto lines = Visuals::breakLines(_f, toRender->description, infoPos, textPaddingX);
	for (auto line : lines){
		renderText(line, COLOR_DEFAULT, x, y, _r, _f);
		y+= fontH;
	}
}

void InventoryRenderer::renderSpecifics(SDL_Window* _w, SDL_Renderer* _r, TTF_Font* _f)
{
	renderInventory(_r, _f);
	renderItemInfo(_r, _f);
	if (inventory->submenuIsOpen()){
		renderSubmenu(_r, _f);
	}
}

StatusBarRenderer::StatusBarRenderer(SDL_Rect viewPort, std::string locationName, Character* player)
: Renderer("STATUS", viewPort), locationName(locationName), player(player)
{}

void StatusBarRenderer::renderStatusBar(SDL_Renderer* _r, TTF_Font* _f)
{
	if (blackout) return;

    int fontH;
    TTF_SizeText(_f, "a", NULL, &fontH);

	int textBaseX = viewPort.x + 5;
	int textBaseY = viewPort.y + 0;

	int x = viewPort.x;
	int y = viewPort.y;
    // SDL_SetRenderDrawColor( _r, 255, 0, 255, 255 );
	// SDL_Rect rect = {x, y, viewPort.w -x, viewPort.h - y};
	// SDL_RenderDrawRect(_r, &rect);

	x = textBaseX;
	y = textBaseY;
	renderText("Location: " + locationName, {255, 255, 255}, x, y, _r, _f);
	x += viewPort.w / 2;
	renderText(Time::currentDateString(), {255, 255, 255}, x, y, _r, _f);

	y+= fontH;

	renderStatusBarSpecific(_r, _f, textBaseX, y);
}

void StatusBarRenderer::renderStatusBarSpecific(SDL_Renderer*_r, TTF_Font* _f, int x, int y)
{
	renderPlayerStatus(_r, _f, x, y);
}

void StatusBarRenderer::renderPlayerStatus(SDL_Renderer*_r, TTF_Font* _f, int x, int y)
{
	if (!player){
		//std::cout << "DEBUG: StatusBarRenderer::player == NULL, can't render player status" << std::endl;
		exit(1);
	}

    int fontH;
    TTF_SizeText(_f, "a", NULL, &fontH);

	int textBaseX = x;
	y += fontH - fontH /4; 
	renderText(player->name, COLOR_DEFAULT, x, y, _r, _f);
	x+= viewPort.w / 2;
	renderText(player->status, COLOR_DEFAULT, x, y, _r, _f);
}

void StatusBarRenderer::renderSpecifics(SDL_Window* w, SDL_Renderer* r, TTF_Font* f)
{
	renderStatusBar(r, f);
}

void StatusBarRenderer::setBlackout(bool _blackout)
{
	blackout = _blackout;
}


TransitionRenderer::TransitionRenderer(ChangeSceneAction* action, SDL_Rect viewPort, int length)
:Renderer("TRANSITION", viewPort), a(action), length(length)
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
:Renderer("FADE_OUT", viewPort), a(action), speed(speed)
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

	_r = SDL_CreateRenderer( _w, -1,  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
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
	//_f = TTF_OpenFont( "assets/OpenSans-Regular.ttf", fontH);
	_f = TTF_OpenFont( fontPath.c_str(), fontSize);
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
    _w = SDL_CreateWindow( title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenW, screenH, SDL_WINDOW_SHOWN );
        if( _w == NULL ){
            std::cerr <<  "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }
	return true;
}

Visuals::Visuals(int screenW, int screenH, std::string fontPath, int fontSize)
:fontPath(fontPath), fontSize(fontSize)
{
    setScreenSize(screenW, screenH);

	if (!initSDL()){
		exit(1);
	}
}

Visuals::~Visuals()
{	
	for (auto& texture : backgroundTextureMap){
		if (texture.second){
			SDL_DestroyTexture(texture.second);
		}
	}

	for (auto& it: spriteSheetTextureMap) {
    	SDL_DestroyTexture(it.second);
	}
	
	for (auto renderer : renderers){
		delete(renderer.second);
	}

	for (auto bg : defaultRendererBackgrounds){
		if (bg.second){
			delete(bg.second);
		}
	}

    SDL_DestroyRenderer( _r );
    SDL_DestroyWindow( _w );
    _r = NULL;
    _w = NULL;
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void Visuals::setScreenSize(int w, int h)
{
    std::cout << "TODO: (setScreenSize) Change actual window size if window was created" << std::endl; 
    std::cout << "TODO: (setScreenSize) Change viewPorts in active scene" << std::endl; 

    screenW = w;
    screenH = h;


    //int convoBarH =  h / 6;
    // VIEW_PORT_FULL_SCREEN = {0, 0, w, h};
    // VIEW_PORT_DEFAULT_STATUS_BAR = {0, 0, w, h/6};
    // VIEW_PORT_DEFAULT_CONVO_BAR = {0, h - h/6, w, h - h/6};


    VIEW_PORT_FULL_SCREEN = {0, 0, w, h};
    VIEW_PORT_DEFAULT_CONTENT = {0,  h/9, w, h -  h/9 - h /7};
    VIEW_PORT_DEFAULT_STATUS_BAR = {0, 0, w, h/9};
    VIEW_PORT_DEFAULT_INVENTORY = {0,  w/9, w, h - h/9};
    VIEW_PORT_DEFAULT_CONVO_BAR = {0, h - h/6, w, h/6};
}

std::pair<int, int> Visuals::getScreenSize()
{
    return {screenW, screenH};
}

SDL_Rect Visuals::VIEW_PORT_FULL_SCREEN = {};
SDL_Rect Visuals::VIEW_PORT_DEFAULT_CONVO_BAR = {};
SDL_Rect Visuals::VIEW_PORT_DEFAULT_STATUS_BAR = {};
SDL_Rect Visuals::VIEW_PORT_DEFAULT_INVENTORY = {};
SDL_Rect Visuals::VIEW_PORT_DEFAULT_CONTENT = {};