#include "inventory.hpp"
#include <iostream>
#include <algorithm>

itemListen_t Inventory::getItems()
{
	return items;
}

int Inventory::getAmount(std::string type)
{
	if (items.find(type) == items.end()){
		return 0;
	} else {
		return items[type].second;
	}
}

void Inventory::add(Item* item, int amount)
{
	std::string type = item->type;
	if (items.find(type) == items.end()){
		items[type] = {item, 0};
	}
	items[type].second += amount;
}

bool Inventory::decrease(std::string type, int amount)
{
	if (items[type].second - amount < 0){
		return false;
	} else {
		items[type].second -= amount;
		if (items[type].second == 0){
			items.erase(type);
		}
		return true;
	}
}

std::vector<Action*> Inventory::useItem(itemType_t type, UseItemParams params, int amount)
{
	std::vector<Action*> triggeredActions;
	for (auto it = items.begin(); it != items.end(); it++){
		if (it->first == type && it->second.second >= amount){
			for (int i = 0; i < amount; i++){
				auto action = items[type].first->use(params);
				if (action){
					triggeredActions.push_back(action);
				}
			}
			decrease(type, amount);
			return triggeredActions;
		}
	}
	//std::cout << "DEBUG: cannot use " << amount << " " << type << ", not enough in inventory" << std::endl;
	return {};
}

int Inventory::size()
{
	return items.size();
}

bool Inventory::submenuIsOpen()
{
	return itemWithSubMenuOpen != NULL 
		&& itemWithSubMenuOpen->getInventorySubmenuData(); 
}