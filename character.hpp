#ifndef __CHARACTER_HPP__
#define __CHARACTER_HPP__

#include "type.hpp"

class Character
{
	public:
		Character(std::string name);
		~Character();
		characterStatus_t status;
		const std::string name;
};

#endif