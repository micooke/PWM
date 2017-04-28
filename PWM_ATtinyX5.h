#ifndef PWM_ATtinyX5_H
#define PWM_ATtinyX5_H

//+------------+---+--------+--------+--------+--------+--------+
//| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 8b ePS |   --   |   --   |   --   |
//|            +---+--------+--------+--------+--------+--------+
//| ATtiny85   | A |   D0   |   D1   |   --   |   --   |   --   |
//|            | B |   D1   |   D3   |   --   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
// 8b/16b : 8 bit or 16 bit timer
// PS/ePS : Regular prescalar, Extended prescalar selection
//  PS = [0,1,8,64,256,1024]
// ePS = [0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384]
// 

//void(*pwm_interrupt0)();
//void(*pwm_interrupt0a)();
//void(*pwm_interrupt0b)();

void(*pwm_interrupt1)();
void(*pwm_interrupt1a)();
void(*pwm_interrupt1b)();

#ifndef PWM_NOISR
//TIMER0_OVF_vect is already defined in wiring.h (used by millis())
//ISR(TIMER0_OVF_vect) { interrupt0(); } 
//ISR(TIMER0_COMPA_vect) { pwm_interrupt0a(); }
//ISR(TIMER0_COMPB_vect) { pwm_interrupt0b(); }

ISR(TIMER1_OVF_vect) { pwm_interrupt1(); }
ISR(TIMER1_COMPA_vect) { pwm_interrupt1a(); }
ISR(TIMER1_COMPB_vect) { pwm_interrupt1b(); }
#endif

#define OCR0A_pin 0
#define OCR0B_pin 1
#define OCR1A_pin 1
#define OCR1B_pin 4

void PWM::set(const uint8_t &Timer, const char &ABCD_out, const uint32_t &FrequencyHz, const uint16_t DutyCycle_Divisor, const bool invertOut)
{
	uint16_t CSx3210 = 0;
	uint32_t FrequencyCount = base_clock / FrequencyHz;
	uint32_t PeriodRegister = 0;
	uint32_t PulseWidthRegister = 0;

	// COMx[10]   = [10] non-inverting ,[11] inverting mode
	const uint8_t COMx10 = 2 + invertOut;

	// Extended set of prescalar values
	//                   0 1 2 3 4  5  6  7  8   9   10  11   12   13   14    15
	uint16_t _PS[16] = { 0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384 };
	// Index of the regular set of prescalar values
	uint8_t N[6] = { 0,1,4,7,9,11 }; // _PS = { 0,1,8,64,256,1024 };

	// find the right prescalar
	switch (Timer)
	{
	case 1:
		// 8b timer, extended prescalar list
		// pg89 shows extended prescalars
		while ((FrequencyCount > (1 + _PS[CSx3210] * 0xFF)) & (CSx3210 < 15)) { ++CSx3210; }
		PS_IDX[Timer] = CSx3210;
		PS[Timer] = _PS[PS_IDX[Timer]];
		break;
	default: // Timer 0
		// 8b timer, regular rescalar list
		while ((FrequencyCount > (1 + _PS[N[CSx3210]] * 0xFF)) & (CSx3210 < 5)) { ++CSx3210; }
		PS_IDX[Timer] = CSx3210;
		PS[Timer] = _PS[N[PS_IDX[Timer]]];
		break;
	}

	// frequency = f_clk/(PS * (1 + PeriodRegister)); 
	PeriodRegister = (FrequencyCount / PS[Timer]) - 1;
	PulseWidthRegister = PeriodRegister / DutyCycle_Divisor;

	// WGMx[210]  =  [111] Fast PWM, OCRxA as top
	// WGMx[3210] = [1110] Fast PWM,  ICRx as top
	// WGMx[3210] = [1111] Fast PWM, OCRxA as top (Timer4 - OCR4C)

	switch (Timer)
	{
	case 0:
		//  8b : ATtiny85, ATmega328p, ATmega32u4
		// set the period register
		OCR0A = PeriodRegister;

		// set the COMx pin mode
		switch (ABCD_out)
		{
		case 'a':
		case 'A':
			pinMode(OCR0A_pin, OUTPUT);
			//TCCR0A = [COM0A1|COM0A0|COM0B1|COM0B0|   -  |   -  | WGM01| WGM00]
			// clear the old bits
			TCCR0A &= ~_BV(COM0A1) & ~_BV(COM0A0);
			// set the new bits
			TCCR0A |= _BV(COM0A0); // NB: OCR0A is toggled, hence half the frequency (its the only option)
			break;
		default:
			OCR0B = PulseWidthRegister;
			pinMode(OCR0B_pin, OUTPUT);
			// clear the old bits
			TCCR0A &= ~_BV(COM0B1) & ~_BV(COM0B0);
			// set the new bits
			TCCR0A |= (COMx10 << 4);
			break;
		}

		// Set the waveform mode to fast PWM
		//TCCR0A = [COM0A1|COM0A0|COM0B1|COM0B0|   -  |   -  | WGM01| WGM00]
		//TCCR0B = [ FOC0A| FOC0A|   -  |   -  | WGM02|  CS02|  CS01|  CS00]
		// clear the old bits
		TCCR0A &= ~_BV(WGM01) & ~_BV(WGM00);
		TCCR0B &= ~_BV(WGM02);
		// set the new bits
		TCCR0A |= _BV(WGM01) | _BV(WGM00);
		TCCR0B |= _BV(WGM02);
		break;
	case 1:
		// set the period register
		OCR1C = PeriodRegister;

		switch (ABCD_out)
		{
		case 'a':
		case 'A':
			OCR1A = PulseWidthRegister;
			pinMode(OCR1A_pin, OUTPUT);
			//TCCR1 =  [  CTC1| PWM1A|COM1A1|COM1A0|  CS13|  CS12|  CS11|  CS10]
			// clear the old bits
			TCCR1 &= ~_BV(PWM1A) & ~_BV(COM1A1) & ~_BV(COM1A0);
			// set the new bits
			TCCR1 |= _BV(PWM1A) | (COMx10 << 4);
			break;
		case 'b':
		case 'B':
			OCR1B = PulseWidthRegister;
			pinMode(OCR1B_pin, OUTPUT);
			//GTCCR =  [   TSM| PWM1B|COM1B1|COM1B0| FOC1B| FOC1A|  PSR1|  PSR0]
			// clear the old bits
			GTCCR &= ~_BV(PWM1B) & ~_BV(COM1B1) & ~_BV(COM1B0);
			// set the new bits
			GTCCR |= _BV(PWM1B) | (COMx10 << 4);
			break;
		}
		break;
	}
}
void PWM::start(const int8_t Timer)
{
	// stop the Timer before setting the new prescalar value
	stop(Timer);

	enableInterrupt(Timer);

	// Set the PWM prescalar (starts the timer)
	switch (Timer)
	{
	case 0:
		//TCCR0B = [ FOC0A| FOC0A|   -  |   -  | WGM02|  CS02|  CS01|  CS00]
		TCCR0B |= PS_IDX[0];
		break;
	case 1:
		//TCCR1 =  [  CTC1| PWM1A|COM1A1|COM1A0|  CS13|  CS12|  CS11|  CS10]
		TCCR1 |= PS_IDX[1];
		break;
	case -1: // All
		TCCR0B |= PS_IDX[0];
		TCCR1 |= PS_IDX[1];
		break;
	}
}
void PWM::stop(const int8_t Timer)
{
	// Set the PWM prescalar to zero (stops the timer)
	switch (Timer)
	{
	case 0:
		//TCCR0B = [ FOC0A| FOC0A|   -  |   -  | WGM02|  CS02|  CS01|  CS00]
		TCCR0B &= ~_BV(CS02) & ~_BV(CS01) & ~_BV(CS00);
		break;
	case 1:
		//TCCR1 =  [  CTC1| PWM1A|COM1A1|COM1A0|  CS13|  CS12|  CS11|  CS10]
		TCCR1 &= ~_BV(CS13) & ~_BV(CS12) & ~_BV(CS11) & ~_BV(CS10);
		break;
	case -1: // All
		TCCR0B &= ~_BV(CS02) & ~_BV(CS01) & ~_BV(CS00);
		TCCR1 &= ~_BV(CS13) & ~_BV(CS12) & ~_BV(CS11) & ~_BV(CS10);
		break;
	}

	disableInterrupt(Timer);
}
void PWM::print()
{
#if (_DEBUG > 0)
	Serial.println(F("Timer0"));
	printRegister(OCR0A, F("OCR0A  = "));
	printRegister(OCR0B, F("OCR0B  = "));
	Serial.println(F("TCCR0A = [ COM0A1| COM0A0| COM0B1| COM0B0|   --  |   --  |  WGM01|  WGM00]"));
	printRegister(TCCR0A);
	Serial.println(F("TCCR0B = [ FOC0A | FOC0A |   --  |   --  | WGM02 |  CS02 |  CS01 |  CS00 ]"));
	printRegister(TCCR0B);

	Serial.println(F("Timer1"));
	printRegister(OCR1A, F("OCR1A  = "));
	printRegister(OCR1B, F("OCR1B  = "));
	Serial.println(F("PLLCSR=  [  LSM  |   --  |   --  |   --  |   --  |  PCKE |  PLLE | PLOCK ]"));
	printRegister(PLLCSR);
	Serial.println(F("TCCR1 =  [  CTC1 | PWM1A | COM1A1| COM1A0|  CS13 |  CS12 |  CS11 |  CS10 ]"));
	printRegister(TCCR1);
	Serial.println(F("GTCCR =  [  TSM  | PWM1B | COM1B1| COM1B0| FOC1B | FOC1A |  PSR1 |  PSR0 ]"));
	printRegister(GTCCR);

	Serial.println(F("Timer interrupt registers"));
	Serial.println(F("TIMSK  = [   --  | OCIE1A| OCIE1B| OCIE0A| OCIE0B| TOIE1 | TOIE0 |   --  ]"));
	printRegister(TIMSK);

	// print the frequency settings
	// frequency = f_clk/(PS * (1 + PeriodRegister));
	// Note : When PS[x] is large, the result of :
	// ## TimerFrequency = base_clock / (PS[1] * (PeriodRegister + 1)) ##
	// gets quantized. Hence it needs to take place over two lines
	uint32_t TimerFrequency;

	Serial.print(F("base clock : ")); Serial.print(base_clock, DEC); Serial.println(F("Hz"));
	Serial.print(F("PRESCALAR[0] : ")); Serial.print(PS[0]); Serial.print(F(", OCR0A : ")); Serial.println(OCR0A);
	TimerFrequency = base_clock / PS[0];
	TimerFrequency /= (OCR0A + 1);
	Serial.print(F("Timer0 : ")); Serial.print(TimerFrequency, DEC); Serial.println(F("Hz"));

	Serial.print(F("PRESCALAR[1] : ")); Serial.print(PS[1]);
	Serial.print(F(", OCR1C : ")); Serial.println(OCR1C);
	TimerFrequency = base_clock / PS[1];
	TimerFrequency /= (OCR1C + 1);

	Serial.print(F("Timer1 : ")); Serial.print(TimerFrequency); Serial.println(F("Hz"));
#endif
}
void PWM::attachInterrupt(const uint8_t &Timer, const char &ABCD_out, void(*isr)())
{
	enableInterrupt(Timer, ABCD_out);
	
	switch (Timer)
	{
		case 1:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					pwm_interrupt1a = isr;
					break;
				case 'b':
				case 'B':
					pwm_interrupt1b = isr;
					break;
				default:
					pwm_interrupt1 = isr;
			}
			break;
	}
}
void PWM::detachInterrupt(const uint8_t &Timer, const char &ABCD_out)
{
	disableInterrupt(Timer, ABCD_out);
	
	switch (Timer)
	{
		case 1:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					pwm_interrupt1a = pwm_empty_interrupt;
					break;
				case 'b':
				case 'B':
					pwm_interrupt1b = pwm_empty_interrupt;
					break;
				default:
					pwm_interrupt1 = pwm_empty_interrupt;
			}
			break;
	}
}
void PWM::enableInterrupt(const int8_t Timer, const char ABCD_out)
{
	// Timer overflow interrupts
	//TIMSK  = [   -  |OCIE1A|OCIE1B|OCIE0A|OCIE0B| TOIE1| TOIE0|   -  ]

	switch (Timer)
	{
	case 0:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK |= _BV(OCIE0A); break;
			case 'b':
			case 'B':
				TIMSK |= _BV(OCIE0B); break;
			default:
				TIMSK |= _BV(TOIE0);
		}
		break;
	case 1:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK |= _BV(OCIE1A); break;
			case 'b':
			case 'B':
				TIMSK |= _BV(OCIE1B); break;
			default:
				TIMSK |= _BV(TOIE1);
		}
		break;
	}
}
void PWM::disableInterrupt(const int8_t Timer, const char ABCD_out)
{
	// Timer overflow interrupts
	//TIMSK  = [   -  |OCIE1A|OCIE1B|OCIE0A|OCIE0B| TOIE1| TOIE0|   -  ]

	switch (Timer)
	{
	case 0:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK &= ~_BV(OCIE0A); break;
			case 'b':
			case 'B':
				TIMSK &= ~_BV(OCIE0B); break;
			default:
				TIMSK &= ~_BV(TOIE0);
		}
		break;
	case 1:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK &= ~_BV(OCIE1A); break;
			case 'b':
			case 'B':
				TIMSK &= ~_BV(OCIE1B); break;
			default:
				TIMSK &= ~_BV(TOIE1);
		}
		break;
	}
}

#endif
