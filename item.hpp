#ifndef __ITEM_HPP__
#define __ITEM_HPP__

#include <string>
#include "type.hpp"
#include "character.hpp"

/* circ dep so forward declare */
class Scene; //for itemInventorySubMenu
class Action;

class InventorySubmenuData
{
	public:
		InventorySubmenuData(std::string type, std::string itemName);
		inventorySubmenuType_t type;
		virtual std::string getText() = 0;
		/* int can store additional data to define what answer means, for example if answer is a player, int can be his ID */
		/* this way an action can be decided on based on answer, like player with id is user */
		virtual std::vector<std::pair<int, std::string>> getAnswers() = 0;
		const std::string itemName = "nop";
};

class ConfirmInventorySubmenuData : public InventorySubmenuData
{
	public:
		ConfirmInventorySubmenuData(std::string itemName);
		std::string getText();
		std::vector<std::pair<int, std::string>> getAnswers();
};

class MapInventorySubmenuData :  public InventorySubmenuData
{
	public:
		MapInventorySubmenuData(std::string itemName, std::unordered_map<sceneId_t, Scene*>* scenes);
		std::string getText();
		std::vector<std::pair<int, std::string>> getAnswers();
		std::unordered_map<sceneId_t, Scene*>* scenes = NULL;
};

struct UseItemParams
{
	Character* user = NULL;
	Character* receiver = NULL;
	Scene* targetLocation = NULL;
};

/* 	
when int associated with selected answer from submenu (CANCEL_INVENTORY_SUBMENU), 
constructItemParamsFromInventoryMenuSelectionSpecific() will return NULL so handleInventoryInput() knows to close inventory sub menu without using item
¯\_(ツ)_/¯ CLIENT SHOULD NOT (accidentaly) USE THIS VALUE FOR HIS OWN SUBMENUDATA ANSWERS  ¯\_(ツ)_/¯
*/
#define CANCEL_INVENTORY_SUBMENU -57324

class Item 
{
	public:
		Item(std::string type);
		~Item();
		virtual Action* use(UseItemParams params) = 0;

		const itemType_t type;
		std::string description = "nop";
		std::string getName();
		void setInventorySubmenuData(InventorySubmenuData* inventorySubmenuData);
		InventorySubmenuData* getInventorySubmenuData();
	protected:	
		InventorySubmenuData* inventorySubmenuData = NULL;
};

class Jonko : public Item
{
	public:
		Jonko();
		Action* use(UseItemParams params);

};

#endif