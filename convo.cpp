#include <map>
#include <iostream>
#include <exception>
#include <string.h> //memcpy
#include "convo.hpp"

Convo::Convo():
texts (texts)
{
	activeTextId = -1;
}

Convo::~Convo()
{
	//TODO: free all actions in actionmap
}

void Convo::linkTexts(int src, int dst, int answer)
{	
	if (links.find(src) == links.end()){
		std::unordered_map<int, int> answerLink;
		answerLink[answer] = dst;
		links[src] = answerLink;
	} else {
		links[src][answer] = dst;
	}
}

void Convo::setAction(int textId, Action* action, int answer)
{
	if (actionMap.find(textId) == actionMap.end()){
		std::unordered_map<int, std::vector<Action*>> answerActions;
		answerActions[answer].push_back(action);
		actionMap[textId] = answerActions;
	} else {
		actionMap[textId][answer].push_back(action);
	}
}

void Convo::addText(int id, Text text, bool overwriteAnswers)
{
	/* DEBUG PRINT */
	// std::cout << "Added text " << id <<  ": " << text.text;
	// if (text.prompt){
	// 	std::cout << "(PROMPT)";
	// }
	// if (!overwriteAnswers){
	// 	std::cout << "(WONT OVERWRITE ANSWERS)";
	// }
	// std::cout << std::endl;
	// for (auto a : text.answers){
	// 	std::cout << "\t" << a.first << ": " << a.second << std::endl; 
	// }

	texts[id].text = text.text;
	texts[id].prompt = text.prompt;
	
	if (overwriteAnswers){
		texts[id].answers = text.answers;
	}
}

void Convo::setActiveText(int id)
{
	activeTextId = id;
}

int Convo::nextTextId(int selectedAnswer)
{
	if (links.find(activeTextId) != links.end() && links.at(activeTextId).find(selectedAnswer) != links.at(activeTextId).end()){
		return links[activeTextId][selectedAnswer];
	} else if (texts.find(activeTextId + 1) != texts.end()){
		return activeTextId + 1;
	} else {
		return -1;
	}
}

std::vector<Action*> Convo::getLinkedActions(int selectedAnswer)
{
	if (actionMap.find(activeTextId)!= actionMap.end() && actionMap[activeTextId].find(selectedAnswer) != actionMap[activeTextId].end()){
		return actionMap[activeTextId][selectedAnswer];	
	} else {
		return {};
	}
}

std::vector<Action*> Convo::progress(int selectedAnswer)
{
	std::string debugStr = "DEBUG convo progress " +  std::to_string(activeTextId) + " -> "; 

	/* check if convo progression is linked to an action */
	std::vector<Action*> triggeredActions = getLinkedActions(selectedAnswer);

	/* progress convo */
	int tmp = nextTextId(selectedAnswer);

	/* if getLinkedAction() returned NULL (no action) and there is no next text, end convo*/
	if (tmp < 0){
		end();
	} 

	/* if current text doesn't exist and no action is set, end convo */
	if (texts.find(activeTextId) == texts.end()){
		end();
	}

	/* Certain actions, you don't want to progress convo */
	/* TODO: make dynamic no-progress action list */
	for (auto a : triggeredActions){
		if (
			// a->actionType == ActionType::INN_REST ||
			// a->actionType == ActionType::SHOP_ACTION ||
			// a->actionType == ActionType::TRANSITION ||
			// a->actionType == ActionType::FADE_OUT ||
			// a->actionType == ActionType::CHANGE_SCENE ||
			a->actionType == "OPEN_INVENTORY"	//Should return to same text when closing convo
		){
			std::cout << "\tBEBUG: Convo progress cancelled" << std::endl;
			tmp = activeTextId;
			break;
		}
	}

	activeTextId = tmp;

	debugStr += std::to_string(activeTextId) + " (answer: " + std::to_string(selectedAnswer) + ")";
	std::cout << debugStr << std::endl;
	
	return triggeredActions;
}

Convo::Text Convo::getText(int id)
{
	return texts[id];
}

std::pair<int, Convo::Text> Convo::current()
{
	return {activeTextId, texts[activeTextId]};
}

void Convo::print()
{
	std::cout << " --- Convo debug print ---" << std::endl;
	for (auto text : texts){
		std::cout << text.first <<": " << text.second.text << std::endl;

		if (text.second.answers.size() > 0){

			if (text.second.prompt){
				throw std::runtime_error("Text with answers and prompt, should not happen");
			}

			/* all answers and their connections */
			for (auto a : text.second.answers){
				std::cout << "\t" << a.first << ": " << a.second;

				/* linked texts */
				if (links.find(text.first) != links.end()){
					if (links.at(text.first).find(a.first) != links.at(text.first).end()){
						std::cout << " -> " <<  links.at(text.first).at(a.first);
					}
				}

				/* linked actions */
				if (actionMap.find(text.first) != actionMap.end()){
					if (actionMap.at(text.first).find(a.first) != actionMap.at(text.first).end()){
						std::cout << "(";
						for (auto action : actionMap[text.first][a.first]){
							std::cout << " " << action->actionType << " ";
						}
						std::cout << ")";
					}
				}
				std::cout << std::endl;
			}
		} else {
			/* Text with no answers */

			/* Should have 0 - 1 connected texts, from answer 0 */
			if (links.find(text.first) != links.end()){
				if (links[text.first].size() > 1){
					/* this should not happen, give debug info */
					throw std::runtime_error("Text without answers has multiple connected texts, text ID: " 
						+ std::to_string(text.first) + ": " + text.second.text);
				} else {
					std::cout << "\t -> " << links[text.first].at(0);
				}
			} 
			
			/* Actions connected to answer 0 */
			if (actionMap.find(text.first) != actionMap.end()) {
				std::cout << "\t -> ";
				for (auto action : actionMap.at(text.first).at(0)){
					std:: cout << " "<< action->actionType << " ";
				}
			}
			std::cout << std::endl;
		}

	}
	std::cout << "------------------------" << std::endl;
}

std::map<int, Convo::Text> Convo::getTexts()
{
	return texts;
}

void Convo::end()
{
	activeTextId = -1;
}

bool Convo::active()
{
	return !(activeTextId < 0);
}