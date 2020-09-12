#ifndef __CONVO_H__
#define __CONVO_H__

#include <unordered_map>
#include <string>
#include <vector>
#include <map>

#include "action.hpp"

class Convo{ 
	public:
		struct Text{
			std::string text;
			std::map<int, std::string> answers;
			bool prompt = false;
		};
		Convo();
		~Convo();
		void addText(int id, Text text, bool overwriteAnswers = true);
		//TODO: multiple links in 1 call?
		void linkTexts(int src, int dst, int answer = 0);
		void setAction(int textId, Action* action, int answer = 0);
		std::vector<Action*> progress(int selectedAnswer);
		/* return next text ID, negative is there is none */
		int nextTextId(int selectedAnswer);
		std::vector<Action*> getLinkedActions(int selectedAnswer);
		void setActionText(int id);
		void setActiveText(int id);
		std::pair<int, Text> current();
		void print();
		/* Mainly for debug */
		std::map<int, Text> getTexts();
		Text getText(int id);
		bool active();
		void end();
	private:
		int activeTextId;
		std::map <int, Text> texts;
		//TODO: make normal maps?
		std::unordered_map<int, std::unordered_map<int, int>> links;
		std::unordered_map<int, std::unordered_map<int, std::vector<Action*>>> actionMap;
};

#endif