// Note, this program uses timer2, therefore pins 11 and 3
//   will lose PWM functionality, and the tone() function
//   will not work

#include<Arduino.h>

#define MOTORPIN 13

// Store where we are in the cycle. If == 0 , we're on.
//   Otherwise, we're off. Increments every time 2ms timer
//   is called, and is mod by 10 (2ms on, 18ms off).
//   Volatile because it is modified in an interrupt.
volatile byte TIMERCOUNTER = 0;

void setup()
{
  pinMode(MOTORPIN,OUTPUT);
  
  // Here we make the settings for the 2ms interrupt.
  cli();                   //disable interrupts
  TCCR2A = 0;              //clear previous settings
  TCCR2B = 0;
  TCNT2  = 0;
  
  OCR2A = 125;             // compare match reg: 16MHz/256/500Hz = 125, every 2ms
  TCCR2B |= (1 << WGM12);  // ctc mode (clear timer on compare match)
  TCCR2B |= (1 << CS12);   // 256 prescalar
  TIMSK2 |= (1 << OCIE2A); // enble timer compare interrupt
  sei();                   //enable interrupts
}

void loop()
{
  // Do other stuff!
}

ISR(TIMER2_COMPA_vect){ // handle the 2ms timer interrupt
  if(!TIMERCOUNTER) //check if zero
    digitalWrite(MOTORPIN,HIGH);
  else
    digitalWrite(MOTORPIN,LOW);
    
  (++TIMERCOUNTER) %= 10; // Mod 10, 2ms+18ms = 20ms/(2ms interrupt) = 10
}
