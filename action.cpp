#include <iostream>

/* circ dep for StartConvoAction, so include here instead of header */
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

ChangeConvoSpeed::ChangeConvoSpeed(int val)
: Action("CHANGE_CONVO_SPEED"), val(val)
{}

StartConvo::StartConvo(Convo* convo, int entryPoint, bool interrupt, bool forceCloseInventory)
: Action("START_CONVO"), convoId(-1), convo(convo), interrupt(interrupt), entryPoint(entryPoint), forceCloseInventory(forceCloseInventory) 
{}

StartConvo::StartConvo(int convoId, bool interrupt, bool forceCloseInventory)
: Action("START_CONVO"), convoId(convoId), convo(NULL), interrupt(interrupt), entryPoint(-1), forceCloseInventory(forceCloseInventory) 
{}


ConvoEntryPointArgs::ConvoEntryPointArgs(int entryPoint, int convoId, sceneId_t sceneId) 
: Action("SET_CONVO_ENTRY_POINT"), convoId(convoId), entryPoint(entryPoint), sceneId(sceneId)
{}

SetText::SetText(int textId, std::string text, int convoId, sceneId_t sceneId)
:Action("SET_TEXT"), textId(textId), text(text), convoId(convoId), sceneId(sceneId)
{}

SetConvoActionTrigger::SetConvoActionTrigger(int textId, Action* action, int answerId, int convoId, sceneId_t sceneId)
: Action("SET_CONVO_ACTION_TRIGGER"), textId(textId), action(action), answerId(answerId), convoId(convoId), sceneId(sceneId)
{}

LinkTexts::LinkTexts(int srcText, int dstText, int answer, int convoId, sceneId_t sceneId)
: Action("LINK_TEXTS"), srcText(srcText), dstText(dstText), answer(answer), convoId(convoId), sceneId(sceneId)
{}