#ifndef __ACTION_HPP__
#define __ACTION_HPP__

#include <vector>

#include "item.hpp"
#include "time.hpp"
#include "type.hpp"

/* circ dep for StartConvoAction, forward declare */
class Convo;

typedef std::unordered_map<sceneArgsKey_t, long> SceneInitArgs_t;

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
		ChangeSceneAction(sceneType_t sceneType, sceneId_t id);
		const sceneType_t sceneType;
		const sceneId_t id = 0;
		
		/* Optional args */
		SceneInitArgs_t sceneInitArgs;

};

/* Set a convoAction, not an action to set convo ..*/
class SetConvoActionTrigger : public Action
{
	public:
		/* When convo id < 0, uses defaultConvo, when scene < 0 uses currentScene (in Game)*/
		SetConvoActionTrigger(int textId, Action* action, int answerId = 0, int convoId = -1, sceneId_t sceneId = -1);
		const int textId;
		const int answerId;
		Action* action;
		const int convoId;
		const sceneId_t sceneId;
};

class LinkTexts : public Action
{
	public:
		/* When convo id < 0, uses defaultConvo, when scene < 0 uses currentScene (in Game) */
		LinkTexts(int srcText, int dstText, int answer = 0, int convoId = -1, int sceneId_t = -1);
		const int srcText;
		const int dstText;
		const int answer;
		const int convoId;
		const sceneId_t sceneId;
};

/* Changes a text of active convo */
/* TODO: make possible to set answers and make prompt */
class SetText : public Action
{
	public:
		/* When convo id < 0, uses defaultConvo, when scene < 0 uses currentScene */
		SetText(int textId, std::string text, int convoId = -1, sceneId_t sceneId = -1);
		const int textId;
		const std::string text;
		const int convoId;
		const int sceneId;
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
		/* if convoId < 0, default convo is used, if sceneId < 0 currentScene is used (in Game)*/
		ConvoEntryPointArgs(int entryPoint, int convoId = -1, sceneId_t sceneId = -1);
		const int convoId;
		const int sceneId;
		const int entryPoint;
};

class ChangeConvoSpeed : public Action
{
	public:
		ChangeConvoSpeed(int val);
		const int val;
};

class StartConvo : public Action
{
	public:
		/* when convo is set, convoId is ignored, otherwise start convo with that id and entryPoint is ignored */
		StartConvo(int convoId, bool interrupt = true, bool forceCloseInventory = true);
		StartConvo(Convo* convo, int entryPoint = 0, bool interrupt = true, bool forceCloseInventory = true);
		const int convoId;
		Convo* convo = NULL;
		const bool forceCloseInventory;
		const int entryPoint;
		const bool interrupt;
};

#endif