#include <iostream>

#include "action.hpp"

Action::Action(actionType_t _type)
:actionType(_type)
{}

std::string Action::toString()
{
	return actionType;
}

ChangeSceneAction::ChangeSceneAction(sceneType_t _sceneType, sceneId_t _id) 
: Action("CHANGE_SCENE"), sceneType(_sceneType),id(_id)
{}

ChangeSceneAction::ChangeSceneAction(sceneType_t _sceneType, sceneId_t _id, bool skipGameOverCheck) 
: Action("CHANGE_SCENE"), sceneType(_sceneType),id(_id), skipGameOverCheck(skipGameOverCheck)
{}

SetConvoActionTrigger::SetConvoActionTrigger(int textId, Action* action, int answerId)
: Action("SET_CONVO_ACTION_TRIGGER"), textId(textId), action(action), answerId(answerId)
{}

LinkTexts::LinkTexts(int srcText, int dstText, int answer)
: Action("LINK_TEXTS"), srcText(srcText), dstText(dstText), answer(answer)
{}

SetText::SetText(int textId, std::string text)
:Action("SET_TEXT"), textId(textId), text(text)
{};

AdvanceTimeAction::AdvanceTimeAction(int amount)
: Action("ADVANCE_TIME"), amount(amount)
{}

TransitionAction::TransitionAction(ChangeSceneAction* nextScene, int length)
: Action("TRANSITION"), length(length), nextScene(nextScene)
{}

FadeOutAction::FadeOutAction(ChangeSceneAction* nextScene, uint8_t speed)
: Action("FADE_OUT"), speed(speed), nextScene(nextScene)
{}

SetSelectedAnswer::SetSelectedAnswer(int* answerId)
: Action("CHANGE_SELECTED_ANSWER"), answerId(answerId)
{}

ConvoEntryPointArgs::ConvoEntryPointArgs(int convo, int entryPoint) 
: Action("SET_CONVO_ENTRY_POINT"), convo(convo), entryPoint(entryPoint)
{}