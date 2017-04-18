# PWM library
This library allows you to use any available timer to produce a PWM output.
The design intent is to control the pulse width and PWM period, with a maximum range of supported frequencies.
It uses the Fast PWM method, but this may change to include the Phase correct PWM method.

| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
|------------|---|--------|--------|--------|--------|--------|
|            |   | 8b PS  | 8b ePS |   --   |   --   |   --   |
| ATtiny85   | A |   D0   |   D1   |   --   |   --   |   --   |
|            | B |   D1   |   D3   |   --   |   --   |   --   |
|            |   | 8b PS  | 16b PS | 8b PS  |   --   |   --   |
| ATmega328p | A |   D6\#  |   D9   |  D12\*  |   --   |   --   |
|            | B |   D5   |  D10   |   D3   |   --   |   --   |
|            |   | 8b PS  | 8b PS  |   --   | 16b PS | 8b ePS |
| ATmega32u4 | A |  D11   |   D9   |   --   |   D5   |  D13   |
|            | B |   D3   |  D10   |   --   |   --   |  D10   |
|            | C |   --   |  D11   |   --   |   --   |   --   |
|            | D |   --   |   --   |   --   |   --   |   D6   |

8b/16b : 8 bit or 16 bit timer

PS/ePS : Regular prescalar, Extended prescalar selection

PS = [0,1,8,64,256,1024]

ePS = [0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384]

\# toggled output. The frequency is half the set frequency, the duty cycle is fixed at 50%

\* same as #, but software PWM. It is implemented through the respective TIMERx_OVF_vect ISR

## TODO
* I can be smarter when setting the PeriodRegister. I plan to use the ISR as a counter to implement a static correction table to add +0 or +1 to the ICRx or OCRx register.

## Output

![ATtiny85](ATtiny85.png?raw=true)
![ATmega328p](ATmega328p.png?raw=true)
![ATmega32u4](ATmega32u4(ProMicro).png?raw=true)
