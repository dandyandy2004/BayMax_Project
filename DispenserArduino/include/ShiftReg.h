#include <avr/io.h>
#include <avr/interrupt.h>
//#include <avr/signal.h>
#include <util/delay.h>

#ifndef SHIFTREG_H
#define SHIFTREG_H

#define shift_register_PORT PORTD //we'll be using port B in this example
#define shift_register_DDR  DDRD

#define DS_pin PD4   //data pin

#define SHCP_pin PD5  //shift clock pin (serial clock)
#define STCP_pin PD6  //store clock pin (latch/register clock)

#define HC595DataHigh() (shift_register_PORT |= (1<<DS_pin)) //just some basic macros
#define HC595DataLow()  (shift_register_PORT &= ~(1<<DS_pin))

void initialise_74HC595()
{
    shift_register_DDR |= ((1<<SHCP_pin)|(1<<STCP_pin)|(1<<DS_pin)); //sets all the pins to output
}

void pulse_shift_clock() {
    shift_register_PORT |= (1 << SHCP_pin);
    shift_register_PORT &= ~(1 << SHCP_pin);
}
void write_74HC595(uint8_t data)
{
    shift_register_PORT &= ~(1<<STCP_pin);

    for(uint8_t i=0; i<8; i++)
    {
        if(data & 0b10000000)
            HC595DataHigh();
        else
            HC595DataLow();

        pulse_shift_clock();
        data <<= 1;
    }

    shift_register_PORT |= (1<<STCP_pin);
}




#endif