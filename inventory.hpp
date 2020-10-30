#ifndef __INVENTORY_HPP__
#define __INVENTORY_HPP__

#include <unordered_map>
#include <vector>
#include "item.hpp"
#include "action.hpp"

typedef std::unordered_map<std::string, std::pair<Item*, int>> itemListen_t;

class Inventory{
	private: 
		itemListen_t items;
	public:
		std::vector<Action*> useItem(itemType_t type, UseItemParams params, int amount = 1);
		itemListen_t getItems();
		int getAmount(std::string type);
		/* returns false if not enough */
		bool decrease(std::string type, int amount = 1);
		void add(Item* item, int amount = 1);
		/* set when submenu is drawn, so item can be used from there */
		Item* itemWithSubMenuOpen;
		bool submenuIsOpen();
		int size();

};

#endif