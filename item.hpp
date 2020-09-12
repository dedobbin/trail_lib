#ifndef __ITEM_HPP__
#define __ITEM_HPP__

#include <string>
#include "type.hpp"

class Item {
	public:
		Item(std::string type);
		void use(int amount = 1);
		itemType_t type;
};

#endif