/*
 * ow-gpio.cpp
 *
 * Created: 30.03.2018 15:57:07
 * Author : A.Kosorotikov
 */ 
#define F_CPU 16000000

#define T_PRESCALE_1    0b001<<CS00;
#define T_PRESCALE_8    0b010<<CS00;
#define T_PRESCALE_64   0b011<<CS00;

#define W1_WAIT_RISE() { EICRA = 0b11 << ISC00; EIFR = 1<<INTF0; EIMSK = 1<<INT0; }
#define W1_WAIT_FALL() { EICRA = 0b10 << ISC00; EIFR = 1<<INTF0; EIMSK = 1<<INT0; }
#define W1_NOWAIT() { EIMSK = 0; }
#define T0_OFF() { TIMSK0 = 0;  TCCR0B = 0;  }
#define T0_15US() { TIFR0 = 1<<TOV0; TIMSK0 = 1<<TOIE0; TCNT0 = 0xff - 240; GTCCR = 1<<PSRSYNC; TCCR0B = T_PRESCALE_1; }
#define T0_120US() { TIFR0 = 1<<TOV0; TIMSK0 = 1<<TOIE0; TCNT0 = 0xff - 240; GTCCR = 1<<PSRSYNC; TCCR0B = T_PRESCALE_8; }
#define T0_480US() { TIFR0 = 1<<TOV0; TIMSK0 = 1<<TOIE0; TCNT0 = 0xff - 118; GTCCR = 1<<PSRSYNC; TCCR0B = T_PRESCALE_64; }

#define LED_ON() { PORTB |= 1<<5; }
#define LED_OFF() { PORTB &= ~1<<5; }
#define LED_IMP() { LED_ON(); LED_OFF(); }
    
#define W1_SEND_0() { DDRD |= 1<<2; }
#define W1_SEND_1() { DDRD &= ~1<<2; }

#define W1_STATE_WAIT_RESET		    0
#define W1_STATE_RESET              1
#define W1_STATE_WAIT_RESET_END     2
#define W1_STATE_WAIT_SEND_PRESENCE 3
#define W1_STATE_SEND_PRESENCE      4
#define W1_STATE_WAIT_COMMAND       5

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t w1_state;
uint8_t w1_id[9] = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0xf0};

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
        // wait FALL
		case W1_STATE_WAIT_RESET:
			W1_WAIT_RISE();
			T0_480US();
            w1_state = W1_STATE_RESET;
			break;
        // wait RISE
        case W1_STATE_WAIT_RESET_END:
            W1_NOWAIT();
            T0_15US();
            w1_state = W1_STATE_WAIT_SEND_PRESENCE;
            break;
        case W1_STATE_RESET:
            W1_WAIT_FALL();
            T0_OFF();
            w1_state = W1_STATE_WAIT_RESET;
            break;
		default:
			W1_WAIT_FALL();
			w1_state = W1_STATE_WAIT_RESET;
	}
}

ISR(TIMER0_OVF_vect)
{
	T0_OFF();
    switch(w1_state)
    {
        case W1_STATE_RESET:
            w1_state = W1_STATE_WAIT_RESET_END;
            break;
        case W1_STATE_WAIT_SEND_PRESENCE:
            T0_120US();
            w1_state = W1_STATE_SEND_PRESENCE;
            W1_SEND_0();
            // TODO
            LED_ON();
            break;
        case W1_STATE_SEND_PRESENCE:
            w1_state = W1_STATE_WAIT_COMMAND;
            W1_WAIT_FALL();
            W1_SEND_1();
            // TODO
            LED_OFF();
            w1_state = W1_STATE_WAIT_RESET;
            break;
            
    }            
}

int main(void)
{
	init();
    /* Replace with your application code */
    while (1) 
    {
    }
}

