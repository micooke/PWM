#ifndef PWM_H
#define PWM_H

//+------------+---+--------+--------+--------+--------+--------+
//| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 8b ePS |   --   |   --   |   --   |
//|            +---+--------+--------+--------+--------+--------+
//| ATtiny85   | A |   D0   |   D1   |   --   |   --   |   --   |
//|            | B |   D1   |   D3   |   --   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 16b PS | 8b PS  |   --   |   --   |
//|            +---+--------+--------+--------+--------+--------+
//| ATmega328p | A |   D6#  |   D9   |  D12*  |   --   |   --   |
//|            | B |   D5   |  D10   |   D3   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 8b PS  |   --   | 16b PS | 8b ePS |
//|            +---+--------+--------+--------+--------+--------+
//| ATmega32u4 | A |  D11   |   D9   |   --   |   D5   |  D13   |
//|            | B |   D3   |  D10   |   --   |   --   |  D10   |
//|            | C |   --   |  D11   |   --   |   --   |   --   |
//|            | D |   --   |   --   |   --   |   --   |   D6   |
//+------------+---+--------+--------+--------+--------+--------+
// 8b/16b : 8 bit or 16 bit timer
// PS/ePS : Regular prescalar, Extended prescalar selection
//  PS = [0,1,8,64,256,1024]
// ePS = [0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384]
// # toggled output. The frequency is half the set frequency, the duty cycle is fixed at 50%
// * same as #, but software PWM. It is implemented through the respective TIMERx_OVF_vect ISR
// 

#ifndef _DEBUG
#define _DEBUG 0
#endif

#if defined(__AVR_ATtiny85__) | defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__)
#ifndef __AVR_ATtinyX5__
#define __AVR_ATtinyX5__
#endif
#endif

class PWM {
protected:
	uint32_t base_clock;
	uint8_t PS_IDX[5] = { 0,0,0,0,0 };
	uint16_t PS[5] = { 0,0,0,0,0 };

public:
	void(*interrupt_routine1)();
	void(*interrupt_routine2)();
	void(*interrupt_routine3)();
	void(*interrupt_routine4)();
	
	static void interrupt_routine_empty() {}
	//void (*interrupt_routine0)(); // N/A - ISR(TIMER0_OVF_vect) is already defined in wiring.h so cannot be used

	PWM() : base_clock(F_CPU)
	{
		//attachISR(0, interrupt_routine_empty);
		attachISR(1, interrupt_routine_empty);
		attachISR(2, interrupt_routine_empty);
		attachISR(3, interrupt_routine_empty);
		attachISR(4, interrupt_routine_empty);
	}

	void set(const uint8_t &Timer, const char &ABCD_out, const uint32_t &FrequencyHz, const uint16_t DutyCycle_Divisor = 2, const bool invertOut = false);
	void start(const int8_t Timer = -1);
	void stop(const int8_t Timer = -1);
	void print();
	
	void attachInterrupt(const uint8_t &Timer, void(*isr)())
	{
		disableInterrupt(Timer);
		attachISR(Timer, isr);
	}
	void detachInterrupt(const uint8_t &Timer, void(*isr)())
	{
		disableInterrupt(Timer);
		attachISR(Timer, interrupt_routine_empty);
	}
	uint16_t getPeriodRegister(const uint8_t Timer)
	{
	  switch (Timer)
	  {
		 case 0:
			return OCR0A;
		 case 1:
		 #if defined(__AVR_ATtinyX5__)
			return OCR1C;
		 #else
			return ICR1;
		 #endif
		 #if defined(__AVR_ATmega328p__) | defined(__AVR_ATmega328P__)
		 case 2:
			return OCR2A;
		 #elif defined(__AVR_ATmega32u4__) | defined(__AVR_ATmega32U4__)
		 case 3:
			return ICR3;
		 case 4:
			return OCR4C;
		 #endif
		 default:
			return 0;
	  }
	}
protected:
	void attachISR(const uint8_t &Timer, void(*isr)())
	{
		switch (Timer)
		{
			//		case 0:
			//			interrupt_routine0 = isr;
			//			break;
		case 1:
			interrupt_routine1 = isr;
			break;
		case 2:
			interrupt_routine2 = isr;
			break;
		case 3:
			interrupt_routine3 = isr;
			break;
		case 4:
			interrupt_routine4 = isr;
			break;
		}
	}

	void enableInterrupt(const int8_t Timer = -1);
	void disableInterrupt(const int8_t Timer = -1);
	void printRegister(volatile uint16_t _register, String _preTab = "         ")
	{
#if (_DEBUG > 0)
#if defined(__AVR_ATtinyX5__) // ATtiny core uses WString, not String - so no .c_str() method
		for (uint8_t i = 0; i < _preTab.length(); ++i)
		{
			Serial.print(_preTab[i]);
		}
#else
		Serial.print(_preTab.c_str());
#endif
		Serial.print(F("[   ")); Serial.print((_register >> 7) & 0x1, DEC); Serial.print(F("   "));
		Serial.print(F("|   ")); Serial.print((_register >> 6) & 0x1, DEC); Serial.print(F("   "));
		Serial.print(F("|   ")); Serial.print((_register >> 5) & 0x1, DEC); Serial.print(F("   "));
		Serial.print(F("|   ")); Serial.print((_register >> 4) & 0x1, DEC); Serial.print(F("   "));
		Serial.print(F("|   ")); Serial.print((_register >> 3) & 0x1, DEC); Serial.print(F("   "));
		Serial.print(F("|   ")); Serial.print((_register >> 2) & 0x1, DEC); Serial.print(F("   "));
		Serial.print(F("|   ")); Serial.print((_register >> 1) & 0x1, DEC); Serial.print(F("   "));
		Serial.print(F("|   ")); Serial.print((_register >> 0) & 0x1, DEC); Serial.println(F("   ]"));
#endif
	}
};


#if defined(__AVR_ATtinyX5__)
#include <PWM_ATtinyX5.h>
#elif defined(__AVR_ATmega328p__) | defined(__AVR_ATmega328P__) 
#include <PWM_ATmega328p.h>
#elif defined(__AVR_ATmega32u4__) | defined(__AVR_ATmega32U4__) 
#include <PWM_ATmega32u4.h>
#endif

PWM pwm;

#ifndef PWM_NOISR
// ISR(TIMER0_OVF_vect) is already defined in wiring.h
//#if defined(TIMER0_OVF_vect)
//ISR(TIMER0_OVF_vect) { pwm.interrupt_routine0(); }
//#endif
#if defined(TIMER1_OVF_vect)
ISR(TIMER1_OVF_vect) { pwm.interrupt_routine1(); }
#endif
#if defined(TIMER2_OVF_vect)
ISR(TIMER2_OVF_vect) { pwm.interrupt_routine2(); }
#endif
#if defined(TIMER3_OVF_vect)
ISR(TIMER3_OVF_vect) { pwm.interrupt_routine3(); }
#endif
#if defined(TIMER4_OVF_vect)
ISR(TIMER4_OVF_vect) { pwm.interrupt_routine4(); }
#endif
#endif

#endif
