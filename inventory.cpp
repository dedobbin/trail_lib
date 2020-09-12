#include "inventory.hpp"

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