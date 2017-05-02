#ifndef PWM_ATmega32u4_H
#define PWM_ATmega32u4_H

//+------------+---+--------+--------+--------+--------+--------+
//| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
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

//void(*pwm_interrupt0)() = &pwm_empty_interrupt;
//void(*pwm_interrupt0a)() = &pwm_empty_interrupt;
//void(*pwm_interrupt0b)() = &pwm_empty_interrupt;

void(*pwm_interrupt1)() = &pwm_empty_interrupt;
void(*pwm_interrupt1a)() = &pwm_empty_interrupt;
void(*pwm_interrupt1b)() = &pwm_empty_interrupt;
void(*pwm_interrupt1c)() = &pwm_empty_interrupt;

void(*pwm_interrupt3)() = &pwm_empty_interrupt;
void(*pwm_interrupt3a)() = &pwm_empty_interrupt;
void(*pwm_interrupt3b)() = &pwm_empty_interrupt;
void(*pwm_interrupt3c)() = &pwm_empty_interrupt;

void(*pwm_interrupt4)() = &pwm_empty_interrupt;
void(*pwm_interrupt4a)() = &pwm_empty_interrupt;
void(*pwm_interrupt4b)() = &pwm_empty_interrupt;
void(*pwm_interrupt4d)() = &pwm_empty_interrupt;

#ifndef PWM_NOISR
//TIMER0_OVF_vect is already defined in wiring.h (used by millis())
//ISR(TIMER0_OVF_vect) { interrupt0(); }
//ISR(TIMER0_COMPA_vect) { pwm_interrupt0a(); }
//ISR(TIMER0_COMPB_vect) { pwm_interrupt0b(); }

ISR(TIMER1_OVF_vect) { pwm_interrupt1(); }
ISR(TIMER1_COMPA_vect) { pwm_interrupt1a(); }
ISR(TIMER1_COMPB_vect) { pwm_interrupt1b(); }
ISR(TIMER1_COMPC_vect) { pwm_interrupt1c(); }

ISR(TIMER3_OVF_vect) { pwm_interrupt3(); }
ISR(TIMER3_COMPA_vect) { pwm_interrupt3a(); }
ISR(TIMER3_COMPB_vect) { pwm_interrupt3a(); }
ISR(TIMER3_COMPC_vect) { pwm_interrupt3a(); }

ISR(TIMER4_OVF_vect) { pwm_interrupt4(); }
ISR(TIMER4_COMPA_vect) { pwm_interrupt4a(); }
ISR(TIMER4_COMPB_vect) { pwm_interrupt4b(); }
ISR(TIMER4_COMPD_vect) { pwm_interrupt4d(); }
#endif

#define OCR0A_pin 11
#define OCR0B_pin 3
#define OCR1A_pin 9
#define OCR1B_pin 10
#define OCR1C_pin 11
#define OCR3A_pin 5
#define OCR4A_pin 13
#define OCR4B_pin 10
#define OCR4D_pin 6

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
	case 0:
		// 8b timer, regular rescalar list
		while ((FrequencyCount > (1 + _PS[N[CSx3210]] * 0xFF)) & (CSx3210 < 5)) { ++CSx3210; }
		PS[Timer] = _PS[N[CSx3210]];
		break;
	case 1:
		// 16b timer, regular prescalar list
		while ((FrequencyCount > (1 + _PS[N[CSx3210]] * 0xFFFF)) & (CSx3210 < 5)) { ++CSx3210; }
		PS[Timer] = _PS[N[CSx3210]];
		break;
	case 3:
		// 16b timer, regular prescalar list
		while ((FrequencyCount > (1 + _PS[N[CSx3210]] * 0xFFFF)) & (CSx3210 < 5)) { ++CSx3210; }
		PS[Timer] = _PS[N[CSx3210]];
		break;
	case 4:
		// 8b timer, extended prescalar list
		while ((FrequencyCount > (1 + _PS[CSx3210] * 0xFF)) & (CSx3210 < 15)) { ++CSx3210; }
		PS[Timer] = _PS[CSx3210];
		break;
	}
	PS_IDX[Timer] = CSx3210;

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
	case 3:
		// set the period register
		ICR3 = PeriodRegister;

		//TCCR3A = [COM3A1|COM3A0|COM3B1|COM3B0|COM3C1|COM3C0| WGM31| WGM30]
		//TCCR3B = [ ICNC3| ICES3|   -  | WGM33| WGM32|  CS32|  CS31|  CS30]
		//TCCR3C = [ FOC3A|   -  |   -  |   -  |   -  |   -  |   -  |   -  ]
		switch (ABCD_out)
		{
		case 'a':
		case 'A':
			OCR3A = PulseWidthRegister;
			pinMode(OCR3A_pin, OUTPUT);
			// clear the old bits
			TCCR3A &= ~_BV(COM3A1) & ~_BV(COM3A0);
			// set the new bits
			TCCR3A = (COMx10 << 6);
			break;
		case 'b':
		case 'B':
			OCR3B = PulseWidthRegister;
			pinMode(OCR3A_pin, OUTPUT); // WARNING : I cannot find any information about where OC3B is
			// clear the old bits
			TCCR3A &= ~_BV(COM3B1) & ~_BV(COM3B0);
			// set the new bits
			TCCR3A = (COMx10 << 4);
			break;
		case 'c':
		case 'C':
			OCR3C = PulseWidthRegister;
			pinMode(OCR3A_pin, OUTPUT); // WARNING : I cannot find any information about where OC3C is
			// clear the old bits
			TCCR3A &= ~_BV(COM3C1) & ~_BV(COM3C0);
			// set the new bits
			TCCR3A = (COMx10 << 2);
			break;
		}

		// Set the waveform mode to fast PWM, ICR3 as TOP : 1110
		//TCCR3A = [COM3A1|COM3A0|COM3B1|COM3B0|COM3C1|COM3C0| WGM31| WGM30]
		//TCCR3B = [ ICNC3| ICES3|   -  | WGM33| WGM32|  CS32|  CS31|  CS30]
		// clear the old bits
		TCCR3A &= ~_BV(WGM31) & ~_BV(WGM30);
		TCCR3B &= ~_BV(WGM33) & ~_BV(WGM32);
		// set the new bits
		TCCR3A |= _BV(WGM31);
		TCCR3B |= _BV(WGM33) | _BV(WGM32);
		break;
	case 4:
		// set the period register
		OCR4C = PeriodRegister;

		//TCCR4A = [ COM4A1| COM4A0| COM4B1| COM4B0| FOC4A| FOC4B| PWM4A| PWM4B]
		//TCCR4B = [  PWM4X|   PSR4| DTPS41| DTPS40|  CS43|  CS42|  CS41|  CS40]
		//TCCR4C = [COM4A1S|COM4A0S|COM4B1S|COM4B0S|COM4D1|COM4D0| FOC4D| PWM4D]
		//TCCR4D = [  FPIE4|  FPEN4|  FPNC4|  FPES4| FPAC4|  FPF4| WGM41| WGM40]
		switch (ABCD_out)
		{
		case 'a':
		case 'A':
			OCR4A = PulseWidthRegister;
			pinMode(OCR4A_pin, OUTPUT);
			// clear the old bits
			TCCR4A &= ~_BV(COM4A1) & ~_BV(COM4A0) & ~_BV(PWM4A);
			// set the new bits
			TCCR4A |= (COMx10 << 6) | _BV(PWM4A);
			break;
		case 'b':
		case 'B':
			OCR4B = PulseWidthRegister;
			pinMode(OCR4B_pin, OUTPUT);
			// clear the old bits
			TCCR4A &= ~_BV(COM4B1) & ~_BV(COM4B0) & ~_BV(PWM4B);
			// set the new bits
			TCCR4A |= (COMx10 << 4) | _BV(PWM4B);
			break;
		case 'd':
		case 'D':
			OCR4D = PulseWidthRegister;
			pinMode(OCR4D_pin, OUTPUT);
			// clear the old bits
			TCCR4C &= ~_BV(COM4D1) & ~_BV(COM4D0) & ~_BV(PWM4D);
			// set the new bits
			TCCR4C |= (COMx10 << 2) | _BV(PWM4D);
			break;
		}
		//WGM4[10]  =  [00] Fast PWM,  OCR4C as top
		TCCR4D &= ~_BV(WGM41) & ~_BV(WGM40);
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
	case 3:
		//TCCR3B = [ ICNC3| ICES3|   -  | WGM33| WGM32|  CS32|  CS31|  CS30]
		TCCR3B |= PS_IDX[3];
		break;
	case 4:
		//TCCR4B = [  PWM4X|   PSR4| DTPS41| DTPS40|  CS43|  CS42|  CS41|  CS40]
		TCCR4B |= PS_IDX[4];
		break;
	case -1: // All
		TCCR0B |= PS_IDX[0];
		TCCR1B |= PS_IDX[1];
		TCCR3B |= PS_IDX[3];
		TCCR4B |= PS_IDX[4];
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
	case 3:
		//TCCR3B = [ ICNC3| ICES3|   -  | WGM33| WGM32|  CS32|  CS31|  CS30]
		TCCR3B &= ~_BV(CS32) & ~_BV(CS31) & ~_BV(CS30);
		break;
	case 4:
		//TCCR4B = [ PWM4X|  PSR4|DTPS41|DTPS40|  CS43|  CS42|  CS41|  CS40]
		TCCR4B &= ~_BV(CS43) & ~_BV(CS42) & ~_BV(CS41) & ~_BV(CS40);
		break;
	case -1: // All
		TCCR0B &= ~_BV(CS02) & ~_BV(CS01) & ~_BV(CS00);
		TCCR1B &= ~_BV(CS12) & ~_BV(CS11) & ~_BV(CS10);
		TCCR3B &= ~_BV(CS32) & ~_BV(CS31) & ~_BV(CS30);
		TCCR4B &= ~_BV(CS43) & ~_BV(CS42) & ~_BV(CS41) & ~_BV(CS40);
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

	Serial.println(F("Timer3"));
	printRegister(OCR3A, F("OCR3A  = "));
	printRegister(OCR3B, F("OCR3B  = "));
	printRegister(OCR3C, F("OCR3C  = "));
	Serial.println(F("TCCR3A = [ COM3A1| COM3A0| COM3B1| COM3B0| COM3C1| COM3C0| WGM31 | WGM30 ]"));
	printRegister(TCCR3A);
	Serial.println(F("TCCR3B = [ ICNC3 | ICES3 |   --  | WGM33 | WGM32 |  CS32 |  CS31 |  CS30 ]"));
	printRegister(TCCR3B);
	Serial.println(F("TCCR3C = [ FOC3A |   --  |   --  |   --  |   --  |   --  |   --  |   --  ]"));
	printRegister(TCCR3C);

	Serial.println(F("Timer4"));
	printRegister(OCR4A, F("OCR4A  = "));
	printRegister(OCR4B, F("OCR4B  = "));
	printRegister(OCR4C, F("OCR4C  = "));
	printRegister(OCR4D, F("OCR4D  = "));
	Serial.println(F("TCCR4A = [ COM4A1| COM4A0| COM4B1| COM4B0| FOC4A | FOC4B | PWM4A | PWM4B ]"));
	printRegister(TCCR4A);
	Serial.println(F("TCCR4B = [ PWM4X |  PSR4 | DTPS41| DTPS40|  CS43 |  CS42 |  CS41 |  CS40 ]"));
	printRegister(TCCR4B);
	Serial.println(F("TCCR4C = [COM4A1S|COM4A0S|COM4B1S|COM4B0S| COM4D1| COM4D0| FOC4D | PWM4D ]"));
	printRegister(TCCR4C);
	Serial.println(F("TCCR4D = [ FPIE4 | FPEN4 | FPNC4 | FPES4 | FPAC4 |  FPF4 | WGM41 | WGM40 ]"));
	printRegister(TCCR4D);

	Serial.println(F("Timer interrupt registers"));
	Serial.println(F("TIMSK0 = [   --  |   --  |   --  |   --  |   --  | OCIE0B| OCIE0A| TOIE0 ]"));
	printRegister(TIMSK0);
	Serial.println(F("TIMSK1 = [   --  |   --  |  ICIE1|   --  |   --  | OCIE1B| OCIE1A| TOIE1 ]"));
	printRegister(TIMSK1);
	Serial.println(F("TIMSK3 = [   --  |   --  |   --  |   --  |   --  | OCIE2B| OCIE2A| TOIE2 ]"));
	printRegister(TIMSK3);
	Serial.println(F("TIMSK4 = [ OCIE4D| OCIE4A| OCIE4B|   --  |   --  | TOIE4 |   --  |   --  ]"));
	printRegister(TIMSK4);

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

	Serial.print(F("PRESCALAR[3] : ")); Serial.print(PS[3]); Serial.print(F(", ICR3 : ")); Serial.println(ICR3);
	TimerFrequency = base_clock / PS[3];
	TimerFrequency /= (OCR3A + 1);
	Serial.print(F("Timer3 : ")); Serial.print(TimerFrequency); Serial.println(F("Hz"));

	Serial.print(F("PRESCALAR[4] : ")); Serial.print(PS[4]); Serial.print(F(", OCR4C : ")); Serial.println(OCR4A);
	TimerFrequency = base_clock / PS[4];
	TimerFrequency /= (OCR4C + 1);
	Serial.print(F("Timer4 : ")); Serial.print(TimerFrequency); Serial.println(F("Hz"));
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
		case 3:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					pwm_interrupt3a = isr;
					break;
				case 'b':
				case 'B':
					pwm_interrupt3b = isr;
					break;
				case 'c':
				case 'C':
					pwm_interrupt3c = isr;
					break;
				default:
					pwm_interrupt3 = isr;
			}
			break;
		case 4:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					pwm_interrupt4a = isr;
					break;
				case 'b':
				case 'B':
					pwm_interrupt4b = isr;
					break;
				case 'd':
				case 'D':
					pwm_interrupt4d = isr;
					break;
				default:
					pwm_interrupt4 = isr;
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
		case 3:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					pwm_interrupt3a = pwm_empty_interrupt;
					break;
				case 'b':
				case 'B':
					pwm_interrupt3b = pwm_empty_interrupt;
					break;
				case 'c':
				case 'C':
					pwm_interrupt3c = pwm_empty_interrupt;
					break;
				default:
					pwm_interrupt3 = pwm_empty_interrupt;
			}
			break;
		case 4:
			switch (ABCD_out)
			{
				case 'a':
				case 'A':
					pwm_interrupt4a = pwm_empty_interrupt;
					break;
				case 'b':
				case 'B':
					pwm_interrupt4b = pwm_empty_interrupt;
					break;
				case 'd':
				case 'D':
					pwm_interrupt4d = pwm_empty_interrupt;
					break;
				default:
					pwm_interrupt4 = pwm_empty_interrupt;
			}
			break;
	}
}
void PWM::enableInterrupt(const int8_t Timer, const char ABCD_out)
{
	// Timer interrupts
	//TIMSK0 = [   -  |   -  |   -  |   -  |   -  |OCIE0B|OCIE0A| TOIE0]
	//TIMSK1 = [   -  |   -  | ICIE1|   -  |OCIE1C|OCIE1B|OCIE1A| TOIE1]
	//TIMSK3 = [   -  |   -  | ICIE3|   -  |OCIE3C|OCIE3B|OCIE3A| TOIE3]
	//TIMSK4 = [OCIE4D|OCIE4A|OCIE4B|   -  |   -  | TOIE4|   -  |   -  ]
	
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
			case 'c':
			case 'C':
				TIMSK1 |= _BV(OCIE1C); break;
			default:
				TIMSK1 |= _BV(TOIE1);
		}
		break;
	case 3:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK3 |= _BV(OCIE3A); break;
			case 'b':
			case 'B':
				TIMSK3 |= _BV(OCIE3B); break;
			case 'c':
			case 'C':
				TIMSK3 |= _BV(OCIE3C); break;
			default:
				TIMSK3 |= _BV(TOIE3);
		}
		break;
	case 4:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK4 |= _BV(OCIE4A); break;
			case 'b':
			case 'B':
				TIMSK4 |= _BV(OCIE4B); break;
			case 'd':
			case 'D':
				TIMSK4 |= _BV(OCIE4D); break;
			default:
				TIMSK4 |= _BV(TOIE4);
		}
		break;
	}
}
void PWM::disableInterrupt(const int8_t Timer, const char ABCD_out)
{
	// Timer interrupts
	//TIMSK0 = [   -  |   -  |   -  |   -  |   -  |OCIE0B|OCIE0A| TOIE0]
	//TIMSK1 = [   -  |   -  | ICIE1|   -  |OCIE1C|OCIE1B|OCIE1A| TOIE1]
	//TIMSK3 = [   -  |   -  | ICIE3|   -  |OCIE3C|OCIE3B|OCIE3A| TOIE3]
	//TIMSK4 = [OCIE4D|OCIE4A|OCIE4B|   -  |   -  | TOIE4|   -  |   -  ]
	
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
			case 'c':
			case 'C':
				TIMSK1 &= ~_BV(OCIE1C); break;
			default:
				TIMSK1 &= ~_BV(TOIE1);
		}
		break;
	case 3:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK3 &= ~_BV(OCIE3A); break;
			case 'b':
			case 'B':
				TIMSK3 &= ~_BV(OCIE3B); break;
			case 'c':
			case 'C':
				TIMSK3 &= ~_BV(OCIE3C); break;
			default:
				TIMSK3 &= ~_BV(TOIE3);
		}
		break;
	case 4:
		switch (ABCD_out)
		{
			case 'a':
			case 'A':
				TIMSK4 &= ~_BV(OCIE4A); break;
			case 'b':
			case 'B':
				TIMSK4 &= ~_BV(OCIE4B); break;
			case 'd':
			case 'D':
				TIMSK4 &= ~_BV(OCIE4D); break;
			default:
				TIMSK4 &= ~_BV(TOIE4);
		}
		break;
	}
}

#endif
