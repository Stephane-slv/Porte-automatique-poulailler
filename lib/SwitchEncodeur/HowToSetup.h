/**
	SwitchEncoder.h - Library for Rotary Encoder breakout by Snootlab
	Coded by Laetitia Hardy-Dessources
	Based on Ben Buxton's Rotary lib 
	
	2015.02.25 - v1.0
	
	Licence CC-BY-SA 3.0
*/

/* To use this library you need to enable interrupts on the pins you've attached CHA and CHB to.
 * You can use pins D0 through D13 (though I wouldn't recommend using D0-D1 that are used for serial communication).
 * Each time you change the wiring you need to set up the corresponding interrupts.
 *
 * Note : SWI doesn't require an interrupt to be read, you can plug it into any pin you want.
 * 
 * If the encoder is wired to pins between D0 & D7 you'll need to enable interrupts on port D, 
 * i.e. set PCIE2 bit in PCICR register.
 * Then you'll have to set the corresponding bits in PCMSK2 register : 
 * D0 -> PCINT16
 * D1 -> PCINT17
 * D2 -> PCINT18
 * D3 -> PCINT19
 * D4 -> PCINT20
 * D5 -> PCINT21
 * D6 -> PCINT22
 * D7 -> PCINT23
 *  
 * If the encoder is wired to pins between D8 and D13 you need to enable interrupts on port B, 
 * i.e. set PCIE0 bit in PCICR register.
 * Then you'll have to set the corresponding bits in PCMSK0 register : 
 * D8 -> PCINT0
 * D9 -> PCINT1
 * D10 -> PCINT2
 * D11 -> PCINT3
 * D12 -> PCINT4
 * D13 -> PCINT5
 * 
 * Then you'll process data during an ISR (Interrupt Service Routine) defined in your sketch.
 * That ISR will be triggered by PCINT2_vect or PCINT0_vect according to your setting.
 * 
 * See example sketch to understand better how it works !
 */