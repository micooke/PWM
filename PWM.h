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

static void pwm_empty_interrupt() {}

class PWM {
protected:
	uint32_t base_clock;
	uint8_t PS_IDX[5] = { 0,0,0,0,0 };
	uint16_t PS[5] = { 0,0,0,0,0 };

public:
	PWM() : base_clock(F_CPU){}
	
	void set(const uint8_t &Timer, const char &ABCD_out, const uint32_t &FrequencyHz, const uint16_t DutyCycle_Divisor = 2, const bool invertOut = false);
	void start(const int8_t Timer = -1);
	void stop(const int8_t Timer = -1);
	void print();
	
	void attachInterrupt(const uint8_t &Timer, const char &ABCD_out, void(*isr)());
	void detachInterrupt(const uint8_t &Timer, const char &ABCD_out);

	void set_register(const int8_t Timer = -1, const char ABCD_out = 'o', uint16_t register_value = 0)
	{
		switch (Timer)
		{
		 case 0:
			switch (ABCD_out)
			{
				case 'b':
				case 'B':
					OCR0B = register_value;
					break;
				case 'a':
				case 'A':
				default:
					OCR0A = register_value;
					break;
			}
		 case 1:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					OCR1A = register_value;
					break;
				case 'b':
				case 'B':
					OCR1B = register_value;
					break;
				#if defined(__AVR_ATmega32u4__) | defined(__AVR_ATmega32U4__)
				case 'c':
				case 'C':
					OCR1C = register_value;
					break;
				#endif
				default:
					#if defined(__AVR_ATtinyX5__)
						OCR1C = register_value;
					#else
						ICR1 = register_value;
					#endif
					break;
			}
		 #if defined(__AVR_ATmega328p__) | defined(__AVR_ATmega328P__)
		 case 2:
			switch (ABCD_out)
			{
				case 'b':
				case 'B':
					OCR2B = register_value;
					break;
				case 'a':
				case 'A':
				default:
					OCR2A = register_value;
					break;
			}
		 #elif defined(__AVR_ATmega32u4__) | defined(__AVR_ATmega32U4__)
		 case 3:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					OCR3A = register_value;
					break;
				case 'b':
				case 'B':
					OCR3B = register_value;
					break;
				case 'c':
				case 'C':
					OCR3C = register_value;
					break;
				default:
					ICR3 = register_value;
					break;
			}
		 case 4:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					OCR4A = register_value;
					break;
				case 'b':
				case 'B':
					OCR4B = register_value;
					break;
				case 'd':
				case 'D':
					OCR4D = register_value;
					break;
				default:
					OCR4C = register_value;
					break;
			}
		 #endif
	  }
	}
	
	uint16_t get_register(const int8_t Timer = -1, const char ABCD_out = 'o')
	{
	  switch (Timer)
	  {
		 case 0:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					return 0;
				case 'b':
				case 'B':
					return OCR0B;
				default:
					return OCR0A;
			}
		 case 1:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					return OCR1A;
				case 'b':
				case 'B':
					return OCR1B;
				#if defined(__AVR_ATmega32u4__) | defined(__AVR_ATmega32U4__)
				case 'c':
				case 'C':
					return OCR1C;
				#endif
				default:
					#if defined(__AVR_ATtinyX5__)
						return OCR1C;
					#else
						return ICR1;
					#endif
			}
		 #if defined(__AVR_ATmega328p__) | defined(__AVR_ATmega328P__)
		 case 2:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					return 0;
				case 'b':
				case 'B':
					return OCR2B;
				default:
					return OCR2A;
			}
		 #elif defined(__AVR_ATmega32u4__) | defined(__AVR_ATmega32U4__)
		 case 3:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					return OCR3A;
				case 'b':
				case 'B':
					return OCR3B;
				case 'c':
				case 'C':
					return OCR3C;
				default:
					return ICR3;
			}
		 case 4:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					return OCR4A;
				case 'b':
				case 'B':
					return OCR4B;
				case 'd':
				case 'D':
					return OCR4D;
				default:
					return OCR4C;
			}
		 #endif
		 default:
			return 0;
	  }
	}

	void enableInterrupt(const int8_t Timer = -1, const char ABCD_out = 'o');
	void disableInterrupt(const int8_t Timer = -1, const char ABCD_out = 'o');
	
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

#endif
