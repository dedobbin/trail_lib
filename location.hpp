#ifndef __LOCATION_HPP__
#define __LOCATION_HPP__

#include "type.hpp"
#include <vector>

struct LocationData 
{
	bool isLocation = true;
	bool discovered = false;
	bool indoors = false;
	weatherType_t weather = "NEUTRAL";
	std::vector<weatherType_t> possibleWeatherTypes = {"NEUTRAL", "RAIN"};
	int weatherSeverity = 5;	/*~1-10*/
};

#endif 