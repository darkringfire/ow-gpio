/*
 * ow-gpio.cpp
 *
 * Created: 30.03.2018 15:57:07
 * Author : A.Kosorotikov
 */ 
#define F_CPU 16000000

#define T_RESET 118
#define T_RELAX 4
#define T_PRESENCE 30

#define T_PRESCALE_1    0b001<<CS00;
#define T_PRESCALE_8    0b010<<CS00;
#define T_PRESCALE_64   0b011<<CS00;

//#define W1_WAIT_RISE { EICRA = 0b11 << ISC00; EIFR = 1<<INTF0; EIMSK = 1<<INT0; }
//#define W1_WAIT_FALL { EICRA = 0b10 << ISC00; EIFR = 1<<INTF0; EIMSK = 1<<INT0; }
#define W1_WAIT_CHANGE { EICRA = 0b01 << ISC00; }
//#define W1_NOWAIT { EIMSK = 0; }
//#define T0_OFF { TIMSK0 = 0;  TCCR0B = 0;  }
//#define T0_15US { TIFR0 = 1<<TOV0; TIMSK0 = 1<<TOIE0; TCNT0 = 0xff - 240; GTCCR = 1<<PSRSYNC; TCCR0B = T_PRESCALE_1; }
//#define T0_120US { TIFR0 = 1<<TOV0; TIMSK0 = 1<<TOIE0; TCNT0 = 0xff - 240; GTCCR = 1<<PSRSYNC; TCCR0B = T_PRESCALE_8; }
//#define T0_480US { TIFR0 = 1<<TOV0; TIMSK0 = 1<<TOIE0; TCNT0 = 0xff - 118; GTCCR = 1<<PSRSYNC; TCCR0B = T_PRESCALE_64; }
#define T0_RUN(t) { GTCCR = 1<<PSRSYNC; TCNT0 = 0; OCR0B = t; TCCR0B = T_PRESCALE_64; }
#define T0_STOP { TCCR0B = 0; }

#define LED_ON { PORTB |= 1<<5; }
#define LED_OFF { PORTB &= ~1<<5; }
#define LED_IMP { LED_ON; LED_OFF; }
    
#define W1_SEND_0 { DDRD |= 1<<2; }
#define W1_SEND_1 { DDRD &= ~1<<2; }

#define W1_STATE_IDLE		    0
#define W1_STATE_RESET              1
#define W1_STATE_WAIT_RESET_END     2
#define W1_STATE_WAIT_SEND_PRESENCE 3
#define W1_STATE_PRESENCE      4
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
	
	w1_state = W1_STATE_IDLE;
	
	// INT0
	W1_WAIT_CHANGE;
	EIMSK = 1 << INT0;
	
	//TC0
	//Mode: normal
	TCCR0A = 0<<WGM00;
	TCCR0B = 0<<WGM02;
    TIMSK0 = 1<<OCIE0A | 1<<OCIE0B;
    OCR0A = T_RESET;
    OCR0B = 0;
	
	sei();
}


ISR(INT0_vect)
{
    if (PIND & 1<<PIND2) {
        // was RISE
        switch(w1_state) {
            case W1_STATE_RESET:
                T0_RUN(T_RELAX);
                break;
            default:
                T0_STOP;
        }
    } else {
        // was FALL
        switch(w1_state) {
            case W1_STATE_RESET:
                W1_SEND_0;
                w1_state = W1_STATE_PRESENCE;
                T0_RUN(T_PRESENCE);
                break;
            case W1_STATE_PRESENCE:
                break;
            default:
                T0_RUN(0);
        }
	}
}

ISR(TIMER0_COMPA_vect)
{
    T0_STOP;
    w1_state = W1_STATE_RESET;
}

ISR(TIMER0_COMPB_vect)
{
    switch(w1_state) {
        case W1_STATE_RESET:
            W1_SEND_0;
            break;
        case W1_STATE_PRESENCE:
            W1_SEND_1;
            T0_STOP;
            //TODO
            w1_state = W1_STATE_IDLE;
            break;
        default:
            ;
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

