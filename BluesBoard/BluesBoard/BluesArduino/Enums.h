// Enums.h

#ifndef _ENUMS_h
#define _ENUMS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

enum DisplayType { Score_Screen, Shots_Screen };

struct BoxScore
{
	struct TimeRemaining
	{
		short minutes;
		short seconds;
	};

	TimeRemaining timeRemaining;
	short bluesGoals;
	short bluesSog;
	short oppGoals;
	short oppSOG;
};

class EnumsClass
{
 protected:


 public:
	void init();
};

extern EnumsClass Enums;

#endif

