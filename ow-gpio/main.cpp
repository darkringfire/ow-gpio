/*
 * ow-gpio.cpp
 *
 * Created: 30.03.2018 15:57:07
 * Author : A.Kosorotikov
 */ 
#define F_CPU 16000000

#define W1_WAIT_RISE() (EICRA = 0b11 << ISC00)
#define W1_WAIT_FALL() (EICRA = 0b10 << ISC00)
#define T0_SEOI() (TIMSK0 |= 1<<TOIE0)
#define T0_CLOI() (TIMSK0 &= ~1<<TOIE0)
#define T0_480US() {TCCR0B = 0b011<<CS00; TIMSK0 |= 1<<TOIE0; TCNT0 = 0xff - 118;}

#define W1_STATE_WAIT_RESET		0
#define W1_STATE_WAIT_RESET_END 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t w1_state;

inline void init()
{
	//debug
	DDRB = 1<<5;
	
	w1_state = W1_STATE_WAIT_RESET;
	
	// INT0
	W1_WAIT_FALL();
	EIMSK = 1 << INT0;
	
	//TC0
	//Mode: normal
	TCCR0A = 0<<WGM00;
	TCCR0B = 0<<WGM02;
	
	sei();
}


ISR(INT0_vect)
{
	switch(w1_state)
	{
		case W1_STATE_WAIT_RESET:
			T0_480US();
			W1_WAIT_RISE();
			w1_state = W1_STATE_WAIT_RESET_END;
			break;
		default:
			W1_WAIT_FALL();
			w1_state = W1_STATE_WAIT_RESET;
	}
}

ISR(TIMER0_OVF_vect)
{
	T0_CLOI();
	PORTB |= 1<<5;
	PORTB &= ~1<<5;
}

int main(void)
{
	init();
    /* Replace with your application code */
    while (1) 
    {
    }
}

