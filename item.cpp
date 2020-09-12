#include <iostream>
#include "item.hpp"

Item::Item(itemType_t _type)
{
	type = _type;
}

void Item::use(int amount)
{
	std::cerr << "Base function Item::use should not be called" << std::endl;
	exit(1);
}