#include <Arduino.h>
#include <util/atomic.h>

//ATtiny85
//            +-\/-+
//   RST PB5 1|*   |8 VCC
// !OC1B PB3 2|    |7 PB2 D2
//  OC1B PB4 3|    |6 PB1 D1 OC0B  OC1A
//       GND 4|    |5 PB0 D0 OC0A !OC1A
//            +----+
//
//Arduino Nano
//   +----+=====+----+
//   |    | USB |    |
//   | D13+-----+D12~| OC2A
//   |3V3        D11 |
//   |Vref       D10~| OC1B
//   | A0/D14     D9~| OC1A
//   | A1/D15     D8 |
//   | A2/D16     D7 |
//   | A3/D17     D6~| OC0A
//   | A4/D18     D5~| OC0B
//   | A5/D19     D4 |
// 20| A6         D3~| OC2B
// 21| A7         D2 |
//   | 5V        GND |
//   | RST       RST |
//   | GND       TX1 |0
//   | Vin       RX1 |1
//   |  5V MOSI GND  |
//   |   [ ][ ][ ]   |
//   |   [*][ ][ ]   |
//   | MISO SCK RST  |
//   +---------------+
//
//Arduino Micro
//      +-----+=====+-----+
//      |     | USB |     |
// OC4A | D13 +-----+ D12~| !OC4D
//      |3V3          D11~|  OC0A  OC1C
//      |Vref         D10~|  OC1B  OC4B
//      | A0/D14       D9~|  OC1A  !OC4B
//      | A1/D15       D8 |
//      | A2/D16       D7 |
//      | A3/D17       D6~|  OC4D
//      | A4/D18       D5~|  OC3A  !OC4A
//      | A5/D19       D4 |
//    20| A6           D3~|  OC0B
//    21| A7           D2 |
//      | 5V          GND |
//      | RST         RST |
//      | GND         TX1 |0
//      | Vin         RX1 |1
//      |MISO           SS|
//      |SCK          MOSI|
//      |  MISO[*][ ]VCC  |
//      |   SCK[ ][ ]MOSI |
//      |   RST[ ][ ]GND  |
//      +-----------------+
//
//Sparkfun Pro Micro
//              +----+=====+----+
//              |[J1]| USB |    |
//             1| TXO+-----+RAW |
//             0| RXI       GND |
//              | GND       RST |
//              | GND       VCC |
//             2| D2         A3 |21
//        OC0B 3|~D3         A2 |20
//             4| D4         A1 |19
// OC3A  !OC4A 5|~D5         A0 |18
//        OC4D 6|~D6        D15 |15 
//             7| D7        D14 |14 
//             8| D8        D16 |16 
// OC1A  !OC4B 9|~D9        D10~|10 OC1B  OC4B
//              +---------------+
//
//+------------+---+--------+--------+--------+--------+--------+
//| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 8b ePS |   --   |   --   |   --   |
//|            +---+--------+--------+--------+--------+--------+
//| ATtiny85   | A |   D0#  |   D1   |   --   |   --   |   --   |
//|            | B |   D1   |   D3   |   --   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 16b PS | 8b PS  |   --   |   --   |
//|            +---+--------+--------+--------+--------+--------+
//| ATmega328p | A |   D6#  |   D9   |  D12*  |   --   |   --   |
//|            | B |   D5   |  D10   |   D3   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 8b PS  |   --   | 16b PS | 8b ePS |
//|            +---+--------+--------+--------+--------+--------+
//| ATmega32u4 | A |  D11#  |   D9   |   --   |   D5   |  D13   |
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

//#define _DEBUG 1
//#define PWM_NOISR // Roll your own ISR
#include <PWM.h>

void setup()
{
#if (_DEBUG > 0)
	Serial.begin(9600);
#if defined(__AVR_ATmega32u4__) | defined(__AVR_ATmega32U4__)
	while !(Serial);
#endif
	Serial.println(F("\nTest PWM\n<setup>"));
#endif

	// divisor = 2 -> 50% duty cycle
	// divisor = 5 -> 20% duty cycle
#if defined(__AVR_ATtinyX5__)
	// Note : D0 / OCR0A pin is used for TinySoftwareSerial
#if (_DEBUG == 0)
	pwm.set(0, 'a', 5000, 2); // Always 50% duty cycle. Freq = Freq/2 (as output is toggled)
#endif
	pwm.set(0, 'b', 5000, 2);
	//pwm.set(1, 'a', 10000, 2); // same pin as OCR0B
	pwm.set(1, 'b', 10000, 2);
#elif defined(__AVR_ATmega32u4__) | defined(__AVR_ATmega32U4__)
	//pwm.set(0, 'a', 5000, 2); // NC - Sparkfun Pro Micro. Always 50% duty cycle. Freq = Freq/2 (as output is toggled)
	pwm.set(0, 'b', 5000, 2);
	pwm.set(1, 'a', 10000, 2);
	pwm.set(1, 'b', 10000, 2);
	//pwm.set(1, 'c', 10000, 2); // same pin as OCR0A
	pwm.set(3, 'a', 20000, 2);
	//pwm.set(4, 'a', 25000, 2); // NC - Sparkfun Pro Micro
	//pwm.set(4, 'b', 25000, 2); // same pin as OCR1B
	pwm.set(4, 'd', 25000, 2);
#elif defined(__AVR_ATmega328p__) | defined(__AVR_ATmega328P__)
	pwm.set(0, 'a', 5000, 2); // Always 50% duty cycle. Freq = Freq/2 (as output is toggled)
	pwm.set(0, 'b', 5000, 2);
	pwm.set(1, 'a', 10000, 2);
	pwm.set(1, 'b', 10000, 2);
	pwm.attachInterrupt(2, 'a', softPWM_OCR2A);
	pwm.set(2, 'a', 15000, 2); // SoftwarePWM. Always 50% duty cycle. Freq = Freq/2 (as output is toggled)
	pwm.set(2, 'b', 15000, 2);
#endif

	pwm.start(); //start all timers

#if (_DEBUG > 0)
	Serial.println(F("PWM setup complete"));
	pwm.print();
	Serial.println(F("</setup>"));
#endif
}

void loop() {}
