#ifndef __TIME_HPP__
#define __TIME_HPP__

#include <string>

typedef unsigned long timestamp;

#define MINUTES_IN_YEAR 525600
#define MINUTES_IN_DAY 1440
#define MINUTES_IN_HOUR 60
#define MINUTES_IN_MINUTE 1

namespace Time{
		// typedef struct Date{
		// 	int year;
		// 	int month;
		// 	int day;
		// 	int hour;
		// 	int minute;
		// };
		enum MonthName{
			JANUARY,
			FEBRUARY,
			MARCH,
			APRIL,
			MAY,
			JUNE,
			JULY,
			AUGUST,
			SEPTEMBER,
			OCTOBER,
			NOVEMBER,
			DECEMBER,
		};
		void advanceYear();
		void advanceDay();
		void advanceHour();
		void advanceMinute(int amount = 1);
		timestamp getCurrent();
		int currentMinuteInDay();
		timestamp minutesSince(timestamp a);
		void init(int day, MonthName month, int year, int hour = 0, int minute = 0);
		/* if numbersOnly is true will print time in hh:mm, otherwise its morning or night etc */
		std::string currentDateString(bool numbersOnly = false);
};
#endif