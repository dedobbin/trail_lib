#include <vector>
#include <string>
#include <iostream>
#include <iostream>
#include <iomanip>

#include "time.hpp"
#include "assert.h"

timestamp current = 0;
timestamp start = 0;

#define LOWEST_YEAR 1849

std::vector<int> monthLen {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 
};

std::vector<std::string> monthNameMap {
	"januari", "februari", "maart", "april", "mei", "juni", "juli", "augustus", "september", "oktober", "november", "december"
};

//TODO: now is called in loop in dateToString, just init once
std::vector<int> monthStartDays ()
 { 
	std::vector<int> ret;
	int t = 0;
	for (int i = 0 ; i < 11; i++){
		t += monthLen[i];
		ret.push_back(t);
	}
	return ret;
 }

/* night, morning, day or evening*/
 std::string timeOfDay(int hour)
{
	if (hour >= 6 && hour < 12){
		return "morning";
	}  if (hour >= 12 && hour < 18){
		return "day";
	} else if (hour >= 18 && hour < 23){
		return "evening";
	} else {
		return "night";
	}
}

std::string dateToString(timestamp date, bool numbersOnly = false)
{
	int r = current;

	int years = r / MINUTES_IN_YEAR;
	r = r % MINUTES_IN_YEAR;
	int days = r / MINUTES_IN_DAY;
	r %= MINUTES_IN_DAY;
	int hours = r / MINUTES_IN_HOUR;
	r %= MINUTES_IN_HOUR;

	//TODO: this is dirty, clean it
	std::string monthName;
	int i;
	int accountedDays = 0;
	for (i = 0; i < 12; i++){
		int startDay = monthStartDays()[i];
		monthName = monthNameMap[i];
		if (days < startDay){
			break;
		} else {
			accountedDays+= monthLen[i];
		}
	}
	if (i == 12){
		accountedDays -= monthLen[i-1];
	}

	int dayOfMonth = days - accountedDays + 1;

	// std::cout << "years passed: " << years << std::endl;
	// std::cout << "days passed: " << days << std::endl;
	// std::cout << "hours passed: " << hours << std::endl;
	// std::cout << "minutes passed: " << r << std::endl;
	// std::cout << "month: " << monthName << std::endl;
	// std::cout << "day of month : " << dayOfMonth << std::endl;
	
	std::string res;
	res += std::to_string(dayOfMonth) + " ";
	res += monthName + " ";
	res += std::to_string(years + LOWEST_YEAR) + ", ";
	if (numbersOnly){
		//diy formatting
		res += (hours < 10 ? "0" : "") + std::to_string(hours) + ":";
		res += (r < 10 ? "0" : "") + std::to_string(r);
	} else {
		res += timeOfDay(hours);
	}

	return res;
}

std::string Time::currentDateString(bool numbersOnly)
{
	return dateToString(current, numbersOnly);
}

int Time::currentMinuteInDay()
{
	return current % MINUTES_IN_DAY;
}

void Time::init(int day, MonthName month, int year, int hour, int minute)
{
	start = 0;
	current = 0;
	assert(year >= LOWEST_YEAR);
	assert(month >= 0 && month <= 11);
	assert(day > 0 && day <= monthLen[month]);
	assert(hour >= 0 && hour < 24);
	assert(minute >= 0 && minute < MINUTES_IN_HOUR);
	
	int y = year - LOWEST_YEAR;
	current += MINUTES_IN_YEAR * y;
	for (int i = 0; i < month; i++){
		current += monthLen[i] *  MINUTES_IN_DAY;
	}
	current += (day -1) * MINUTES_IN_DAY;
	current += hour * MINUTES_IN_HOUR;
	current += minute;
	start = current;
}

timestamp Time::getCurrent()
{
	return current;
}

timestamp Time::minutesSince(timestamp other)
{
	return current - other;
}

void Time::advanceYear()
{
	current += MINUTES_IN_YEAR;
}

void Time::advanceDay()
{
	current += MINUTES_IN_DAY;
}

void Time::advanceHour()
{
	current += MINUTES_IN_HOUR;
}

void Time::advanceMinute(int amount)
{
	current += MINUTES_IN_MINUTE * amount;
}