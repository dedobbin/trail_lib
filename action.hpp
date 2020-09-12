#ifndef __ACTION_HPP__
#define __ACTION_HPP__

#include <vector>

#include "item.hpp"
#include "time.hpp"
#include "type.hpp"

#define DEFAULT_TRANSITION_LENGTH 2

class Action
{
	public:
		Action(actionType_t);
		const actionType_t actionType;
		std::string toString();
};

class ChangeSceneAction : public Action
{
	public:
		/*Skip game over check can be used so party is dead, yet no game over fadeout happend, used in game over cutscene*/
		ChangeSceneAction(sceneType_t sceneType, sceneId_t id);
		ChangeSceneAction(sceneType_t sceneType, sceneId_t id, bool skipGameOverCheck);
		const sceneType_t sceneType;
		const sceneId_t id = 0;
		
		/* Optional args */
		bool skipGameOverCheck = false;
};

/* Set a convoAction, not an action to set convo ..*/
class SetConvoActionTrigger : public Action
{
	public:
		SetConvoActionTrigger(int textId, Action* action, int answerId = 0);
		const int textId;
		const int answerId;
		Action* action;
};

class LinkTexts : public Action
{
	public:
		LinkTexts(int srcText, int dstText, int answer = 0);
		const int srcText;
		const int dstText;
		const int answer;
};

/* Changes a text of active convo */
/* TODO: make possible to set answers and make prompt */
class SetText : public Action
{
	public:
		SetText(int convoId, std::string text);
		const int textId;
		const std::string text;
};

class AdvanceTimeAction : public Action 
{
	public:
		AdvanceTimeAction(int amount = 0);
		/* Used if scene wants to progress time dynamically, scenes can also use static amount of course */
		const int amount;
};

class TransitionAction : public Action
{
	public:
		TransitionAction(ChangeSceneAction* nextScene, int length = DEFAULT_TRANSITION_LENGTH);
		const ChangeSceneAction* nextScene;
		const int length;
};

class FadeOutAction : public Action
{
	public:
		FadeOutAction(ChangeSceneAction* nextScene, uint8_t speed = 1);
		const ChangeSceneAction* nextScene;
		const uint8_t speed;
};

/* Pointer so id can be resolved when action is processed instead of when action is linked to convo text */
class SetSelectedAnswer : public Action
{
	public:
		SetSelectedAnswer(int* answerId);
		const int* answerId;
};

class ConvoEntryPointArgs: public Action
{
	public:
		ConvoEntryPointArgs(int convo, int entryPoint);
		const int convo;
		const int entryPoint;
};



#endif