# PWM library
This library allows you to use any available timer to produce a PWM output.
The design intent is to control the pulse width and PWM period, with a maximum range of supported frequencies.
It uses the Fast PWM method, but this may change to include the Phase correct PWM method.

## TODO
* I can be smarter when setting the PeriodRegister. I plan to use the ISR as a counter to implement a static correction table to add +0 or +1 to the ICRx or OCRx register.

## Output

![ATtiny85](ATtiny85.png?raw=true)
![ATmega328p](ATmega328p.png?raw=true)
![ATmega32u4](ATmega32u4(ProMicro).png?raw=true)
