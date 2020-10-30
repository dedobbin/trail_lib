#include <iostream>
#include <locale> 
#include "item.hpp"

/* circ dep so include here */
#include "action.hpp"
#include "scene.hpp"

Item::Item(itemType_t type)
:type(type)
{}

Item::~Item()
{
	if (inventorySubmenuData){
		delete(inventorySubmenuData);
	}
}

std::string Item::getName()
{
	std::string name = "";
	std::locale loc;
	for(auto elem : type)
		name+= std::tolower(elem,loc);
	return name;
}


void Item::setInventorySubmenuData(InventorySubmenuData* _inventorySubmenuData)
{
	if (inventorySubmenuData){
		delete(inventorySubmenuData);
	}
	inventorySubmenuData = _inventorySubmenuData;
}

InventorySubmenuData* Item::getInventorySubmenuData()
{
	return inventorySubmenuData;
}


Jonko::Jonko()
: Item("JONKO")
{
	/* default, can be overwritten */
	inventorySubmenuData = new ConfirmInventorySubmenuData(getName());
	description = "These are magical herbs";
	//description = "this is a long description for debug purposes.. this is a long description for debug purposes.. this is a long description for debug purposes.. ";
}

Action* Jonko::use(UseItemParams params)
{
	//std::cout << "DEBUG: jonko used on " << params.user->name << std::endl;
	params.user->status = "SKAFFA";
	//return new Action("QUIT");
	return NULL;
}

InventorySubmenuData::InventorySubmenuData(inventorySubmenuType_t type, std::string itemName)
:type(type), itemName(itemName)
{};

ConfirmInventorySubmenuData::ConfirmInventorySubmenuData(std::string itemName)
: InventorySubmenuData("CONFIRM", itemName)
{}

std::string ConfirmInventorySubmenuData::getText()
{
	return "Use " + itemName + "?";
}

std::vector<std::pair<int, std::string>> ConfirmInventorySubmenuData::getAnswers()
{
	return {
		{0, "Use"},
		{CANCEL_INVENTORY_SUBMENU, "Cancel"},
	};
}

MapInventorySubmenuData::MapInventorySubmenuData(std::string itemName, std::unordered_map<sceneId_t, Scene*>* scenes)
: InventorySubmenuData("MAP", itemName), scenes(scenes)
{}

std::string MapInventorySubmenuData::getText()
{
	return "Use " + itemName + " where?";
}

std::vector<std::pair<int, std::string>> MapInventorySubmenuData::getAnswers()
{
	std::vector<std::pair<int, std::string>>  answers;
	for (auto scene : *scenes){
		if (scene.second->isDiscoveredLocation()){
			answers.push_back({scene.first, scene.second->getName()});
		}
	}
	answers.push_back({CANCEL_INVENTORY_SUBMENU, "Cancel"});

	return answers;
}
