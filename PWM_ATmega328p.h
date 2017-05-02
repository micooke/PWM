#ifndef PWM_ATmega328p_H
#define PWM_ATmega328p_H

//+------------+---+--------+--------+--------+--------+--------+
//| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 16b PS | 8b PS  |   --   |   --   |
//|            +---+--------+--------+--------+--------+--------+
//| ATmega328p | A |   D6#  |   D9   |  D12*  |   --   |   --   |
//|            | B |   D5   |  D10   |   D3   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
// 8b/16b : 8 bit or 16 bit timer
// PS/ePS : Regular prescalar, Extended prescalar selection
//  PS = [0,1,8,64,256,1024]
// ePS = [0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384]
// # toggled output. The frequency is half the set frequency, the duty cycle is fixed at 50%
// * same as #, but software PWM. It is implemented through the respective TIMERx_OVF_vect ISR
// 

//void(*pwm_interrupt0)() = &pwm_empty_interrupt;
//void(*pwm_interrupt0a)() = &pwm_empty_interrupt;
//void(*pwm_interrupt0b)() = &pwm_empty_interrupt;

void(*pwm_interrupt1)() = &pwm_empty_interrupt;
void(*pwm_interrupt1a)() = &pwm_empty_interrupt;
void(*pwm_interrupt1b)() = &pwm_empty_interrupt;

void(*pwm_interrupt2)() = &pwm_empty_interrupt;
void(*pwm_interrupt2a)() = &pwm_empty_interrupt;
void(*pwm_interrupt2b)() = &pwm_empty_interrupt;

#ifndef PWM_NOISR
//TIMER0_OVF_vect is already defined in wiring.h (used by millis())
//ISR(TIMER0_OVF_vect) { interrupt0(); }
//ISR(TIMER0_COMPA_vect) { pwm_interrupt0a(); }
//ISR(TIMER0_COMPB_vect) { pwm_interrupt0b(); }

ISR(TIMER1_OVF_vect) { pwm_interrupt1(); }
ISR(TIMER1_COMPA_vect) { pwm_interrupt1a(); }
ISR(TIMER1_COMPB_vect) { pwm_interrupt1b(); }

ISR(TIMER2_OVF_vect) { pwm_interrupt2(); }
ISR(TIMER2_COMPA_vect) { pwm_interrupt2a(); }
ISR(TIMER2_COMPB_vect) { pwm_interrupt2b(); }
#endif

volatile bool OCR2A_state = false;

#define OCR0A_pin 6
#define OCR0B_pin 5
#define OCR1A_pin 9
#define OCR1B_pin 10
#define OCR2A_pin 12
#define OCR2B_pin 3

// HACK : I think OCR2A only toggles if OCR2A = TOP (255)
void softPWM_OCR2A()
{
	digitalWrite(OCR2A_pin, OCR2A_state);
	OCR2A_state = !OCR2A_state;
}

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
		// 16b timer, regular prescalar list
		while ((FrequencyCount > (1 + _PS[N[CSx3210]] * 0xFFFF)) & (CSx3210 < 5)) { ++CSx3210; }
		PS[Timer] = _PS[N[CSx3210]];
		break;
	default:
		// 8b timer, regular rescalar list
		while ((FrequencyCount > (1 + _PS[N[CSx3210]] * 0xFF)) & (CSx3210 < 5)) { ++CSx3210; }
		PS[Timer] = _PS[N[CSx3210]];
		break;
	}
	PS_IDX[Timer] = CSx3210;

	// frequency = f_clk/(PS * (1 + PeriodRegister)); 
	PeriodRegister = (FrequencyCount / PS[Timer]) - 1;
	PulseWidthRegister = PeriodRegister / DutyCycle_Divisor;
	
	#if (_DEBUG > 0)
	Serial.print(F("Timer "));
	Serial.println(Timer);
	Serial.print(F("PeriodRegister = "));
	Serial.println(PeriodRegister);
	Serial.print(F("PulseWidthRegister = "));
	Serial.println(PulseWidthRegister);
	#endif

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
		ICR1 = PeriodRegister;

		// set the COMx pin mode
		switch (ABCD_out)
		{
		case 'a':
		case 'A':
			OCR1A = PulseWidthRegister;
			pinMode(OCR1A_pin, OUTPUT);
			//TCCR1A = [COM1A1|COM1A0|COM1B1|COM1B0|   -  |   -  | WGM11| WGM10]
			// clear the old bits
			TCCR1A &= ~_BV(COM1A1) & ~_BV(COM1A0);
			// set the new bits
			TCCR1A |= (COMx10 << 6);
			break;
		case 'b':
		case 'B':
			OCR1B = PulseWidthRegister;
			pinMode(OCR1B_pin, OUTPUT);
			// clear the old bits
			TCCR1A &= ~_BV(COM1B1) & ~_BV(COM1B0);
			// set the new bits
			TCCR1A |= (COMx10 << 4);
			break;
		}

		// Set the waveform mode to fast PWM, ICR1 as TOP : 1110
		//TCCR1A = [COM1A1|COM1A0|COM1B1|COM1B0|   -  |   -  | WGM11| WGM10]
		//TCCR1B = [ ICNC1| ICES1|   -  | WGM13| WGM12|  CS12|  CS11|  CS10]
		// clear the old bits
		TCCR1A &= ~_BV(WGM11) & ~_BV(WGM10);
		TCCR1B &= ~_BV(WGM13) & ~_BV(WGM12);
		// set the new bits
		TCCR1A |= _BV(WGM11);
		TCCR1B |= _BV(WGM13) | _BV(WGM12);
		break;
	case 2:
		// set the period register
		OCR2A = PeriodRegister;

		// set the COMx pin mode
		switch (ABCD_out)
		{
		case 'a':
		case 'A':
			pinMode(OCR2A_pin, OUTPUT);
			//TCCR2A = [COM2A1|COM2A0|COM2B1|COM2B0|   -  |   -  | WGM21| WGM20]
			// clear the old bits
			TCCR2A &= ~_BV(COM2A1) & ~_BV(COM2A0);
			// set the new bits
			TCCR2A |= _BV(COM2A0);// NB: OCR2A is toggled, hence half the frequency (its the only option)
			break;
		case 'b':
		case 'B':
			OCR2B = PulseWidthRegister;
			pinMode(OCR2B_pin, OUTPUT);
			// clear the old bits
			TCCR2A &= ~_BV(COM2B1) & ~_BV(COM2B0);
			// set the new bits
			TCCR2A |= (COMx10 << 4);
			break;
		}

		// Set the waveform mode to fast PWM, OCR2A as TOP : 111
		//TCCR2A = [COM2A1|COM2A0|COM2B1|COM2B0|   -  |   -  | WGM21| WGM20]
		//TCCR2B = [ FOC2A| FOC2B|   -  |   -  | WGM22|  CS22|  CS21|  CS20]
		// clear the old bits
		TCCR2A &= ~_BV(WGM21) & ~_BV(WGM20);
		TCCR2B &= ~_BV(WGM22);
		// set the new bits
		TCCR2A |= _BV(WGM21) | _BV(WGM20);
		TCCR2B |= _BV(WGM22);
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
		//TCCR1B = [ ICNC1| ICES1|   -  | WGM13| WGM12|  CS12|  CS11|  CS10]
		TCCR1B |= PS_IDX[1];
		break;
	case 2:
		//TCCR2B = [ FOC2A| FOC2B|   -  |   -  | WGM22|  CS22|  CS21|  CS20]
		TCCR2B |= PS_IDX[2];
		break;
	case -1: // All
		TCCR0B |= PS_IDX[0];
		TCCR1B |= PS_IDX[1];
		TCCR2B |= PS_IDX[2];
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
		//TCCR1B = [ ICNC1| ICES1|   -  | WGM13| WGM12|  CS12|  CS11|  CS10]
		TCCR1B &= ~_BV(CS12) & ~_BV(CS11) & ~_BV(CS10);
		break;
	case 2:
		//TCCR2B = [ FOC2A| FOC2B|   -  |   -  | WGM22|  CS22|  CS21|  CS20]
		TCCR2B &= ~_BV(CS22) & ~_BV(CS21) & ~_BV(CS20);
		break;
	case -1: // All
		TCCR0B &= ~_BV(CS02) & ~_BV(CS01) & ~_BV(CS00);
		TCCR1B &= ~_BV(CS12) & ~_BV(CS11) & ~_BV(CS10);
		TCCR2B &= ~_BV(CS22) & ~_BV(CS21) & ~_BV(CS20);
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
	printRegister(ICR1, F("ICR1   = "));
	Serial.println(F("TCCR1A = [ COM1A1| COM1A0| COM1B1| COM1B0|   --  |   --  | WGM11 | WGM10 ]"));
	printRegister(TCCR1A);
	Serial.println(F("TCCR1B = [ ICNC1 | ICES1 |   --  | WGM13 | WGM12 |  CS12 |  CS11 |  CS10 ]"));
	printRegister(TCCR1B);

	Serial.println(F("Timer2"));
	printRegister(OCR2A, F("OCR2A  = "));
	printRegister(OCR2B, F("OCR2B  = "));
	Serial.println(F("TCCR2A = [ COM2A1| COM2A0| COM2B1| COM2B0|   --  |   --  | WGM21 | WGM20 ]"));
	printRegister(TCCR2A);
	Serial.println(F("TCCR2B = [ FOC2A | FOC2B |   --  |   --  | WGM22 |  CS22 |  CS21 |  CS20 ]"));
	printRegister(TCCR2B);

	Serial.println(F("Timer interrupt registers"));
	Serial.println(F("TIMSK0 = [   --  |   --  |   --  |   --  |   --  | OCIE0B| OCIE0A| TOIE0 ]"));
	printRegister(TIMSK0);
	Serial.println(F("TIMSK1 = [   --  |   --  |  ICIE1|   --  |   --  | OCIE1B| OCIE1A| TOIE1 ]"));
	printRegister(TIMSK1);
	Serial.println(F("TIMSK2 = [   --  |   --  |   --  |   --  |   --  | OCIE2B| OCIE2A| TOIE2 ]"));
	printRegister(TIMSK2);

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
	Serial.print(F(", ICR1 : ")); Serial.println(ICR1);
	TimerFrequency = base_clock / PS[1];
	TimerFrequency /= (ICR1 + 1);
	Serial.print(F("Timer1 : ")); Serial.print(TimerFrequency); Serial.println(F("Hz"));

	Serial.print(F("PRESCALAR[2] : ")); Serial.print(PS[2]); Serial.print(F(", OCR2A : ")); Serial.println(OCR2A);
	TimerFrequency = base_clock / PS[2];
	TimerFrequency /= (OCR2A + 1);
	Serial.print(F("Timer2 : ")); Serial.print(TimerFrequency); Serial.println(F("Hz"));
#endif
}
void PWM::attachInterrupt(const uint8_t &Timer, const char &ABCD_out, void(*isr)())
{
	disableInterrupt(Timer, ABCD_out);
	
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
		case 2:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					pwm_interrupt2a = isr;
					break;
				case 'b':
				case 'B':
					pwm_interrupt2b = isr;
					break;
				default:
					pwm_interrupt2 = isr;
			}
			break;
	}
	
	enableInterrupt(Timer, ABCD_out);
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
		case 2:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					pwm_interrupt2a = pwm_empty_interrupt;
					break;
				case 'b':
				case 'B':
					pwm_interrupt2b = pwm_empty_interrupt;
					break;
				default:
					pwm_interrupt2 = pwm_empty_interrupt;
			}
			break;
	}
}
void PWM::enableInterrupt(const int8_t Timer, const char ABCD_out)
{
	// Timer interrupts
	//TIMSK0 = [   -  |   -  |   -  |   -  |   -  |OCIE0B|OCIE0A| TOIE0]
	//TIMSK1 = [   -  |   -  | ICIE1|   -  |   -  |OCIE1B|OCIE1A| TOIE1] // ATmega328p
	//TIMSK2 = [   -  |   -  |   -  |   -  |   -  |OCIE2B|OCIE2A| TOIE2]
	
	switch (Timer)
	{
	case 0:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK0 |= _BV(OCIE0A); break;
			case 'b':
			case 'B':
				TIMSK0 |= _BV(OCIE0B); break;
			default:
				TIMSK0 |= _BV(TOIE0);
		}
		break;
	case 1:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK1 |= _BV(OCIE1A); break;
			case 'b':
			case 'B':
				TIMSK1 |= _BV(OCIE1B); break;
			default:
				TIMSK1 |= _BV(TOIE1);
		}
		break;
	case 2:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK2 |= _BV(OCIE2A); break;
			case 'b':
			case 'B':
				TIMSK2 |= _BV(OCIE2B); break;
			default:
				TIMSK2 |= _BV(TOIE2);
		}
		break;
	}
}
void PWM::disableInterrupt(const int8_t Timer, const char ABCD_out)
{
	// Timer interrupts
	//TIMSK0 = [   -  |   -  |   -  |   -  |   -  |OCIE0B|OCIE0A| TOIE0]
	//TIMSK1 = [   -  |   -  | ICIE1|   -  |   -  |OCIE1B|OCIE1A| TOIE1] // ATmega328p
	//TIMSK2 = [   -  |   -  |   -  |   -  |   -  |OCIE2B|OCIE2A| TOIE2]
	
	switch (Timer)
	{
	case 0:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK0 &= ~_BV(OCIE0A); break;
			case 'b':
			case 'B':
				TIMSK0 &= ~_BV(OCIE0B); break;
			default:
				TIMSK0 &= ~_BV(TOIE0);
		}
		break;
	case 1:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK1 &= ~_BV(OCIE1A); break;
			case 'b':
			case 'B':
				TIMSK1 &= ~_BV(OCIE1B); break;
			default:
				TIMSK1 &= ~_BV(TOIE1);
		}
		break;
	case 2:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK2 &= ~_BV(OCIE2A); break;
			case 'b':
			case 'B':
				TIMSK2 &= ~_BV(OCIE2B); break;
			default:
				TIMSK2 &= ~_BV(TOIE2);
		}
		break;
	}
}
#endif
