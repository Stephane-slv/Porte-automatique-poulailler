/**
	SwitchEncoder.h - Library for Rotary Encoder breakout by Snootlab
	Coded by Laetitia Hardy-Dessources
	Based on Ben Buxton's Rotary lib 
	
	2015.02.26 - v1.0
	
	Licence CC-BY-SA 3.0
*/

#ifndef SwitchEncoder_h
#define SwitchEncoder_h

#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif

// Values returned by 'process'
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

class SwitchEncoder
{
	public:
		SwitchEncoder(char CHA, char CHB, char SWI);
		char process();
		int count();
		void resetCount();
		void setCount(int c);
		int button();
		
	private:
		unsigned char state;
		unsigned char _cha;
		unsigned char _chb;
		unsigned char _swi;
		int _count;
};

#endif
 
