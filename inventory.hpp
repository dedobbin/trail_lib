#ifndef __INVENTORY_HPP__
#define __INVENTORY_HPP__

#include <unordered_map>
#include "item.hpp"

typedef std::unordered_map<std::string, std::pair<Item*, int>> itemListen_t;

class Inventory{
	private: 
		itemListen_t items;
	public:
		itemListen_t getItems();
		int getAmount(std::string type);
		/* returns false if not enough */
		bool decrease(std::string type, int amount = 1);
		void add(Item* item, int amount = 1);
		Item* selectedItem;
};

#endif