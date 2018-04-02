/*
 * ow-gpio.cpp
 *
 * Created: 30.03.2018 15:57:07
 * Author : A.Kosorotikov
 */ 
#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#define W1_PIN	PIND
#define W1_N	PIND2

#define T_RESET 118
#define T_RELAX 4
#define T_PRESENCE 30
#define T_READ_BIT 4
#define T_SEND_0 6

#define T_PRESCALE_1    0b001<<CS00;
#define T_PRESCALE_8    0b010<<CS00;
#define T_PRESCALE_64   0b011<<CS00;

#define W1_WAIT_CHANGE { EICRA = 0b01 << ISC00; }
#define T0_RUN(t) { GTCCR = 1<<PSRSYNC; TCNT0 = 0; OCR0B = t; TCCR0B = T_PRESCALE_64; }
#define T0_STOP { TCCR0B = 0; }

#define LED_ON { PORTB |= 1<<5; }
#define LED_OFF { PORTB &= ~1<<5; }
#define LED_IMP { LED_ON; LED_OFF; }
    
#define W1_SEND_0 { DDRD |= 1<<2; }
#define W1_SEND_1 { DDRD &= ~1<<2; }

#define STATE_IDLE					0
#define STATE_RESET					1
#define STATE_WAIT_RESET_END		2
#define STATE_WAIT_SEND_PRESENCE	3
#define STATE_PRESENCE				4
#define STATE_COMMAND				5
#define STATE_SEARCH_ROM			6
#define STATE_SKIP_ROM				7

uint8_t w1_id[7] = {0xf0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
uint8_t w1_state;
uint8_t rw_byte;
uint8_t byte_counter;
uint8_t read_bit;
uint8_t bit_mask;
uint8_t triplet_bit;
uint8_t crc;

static const uint8_t PROGMEM dscrc_table[] = {
	0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

void crc8(char data)
{
	crc = pgm_read_byte(dscrc_table + (crc ^ data));
}

inline void init()
{
	//debug
	DDRB = 1<<5;
	
	w1_state = STATE_IDLE;
	
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
    if (W1_PIN & 1<<W1_N) {
        // was RISE
        switch(w1_state) {
            case STATE_RESET:
                T0_RUN(T_RELAX);
                break;
			case STATE_PRESENCE:
				T0_STOP;
                //TODO
                w1_state = STATE_COMMAND;
				bit_mask = 1;
				break;
            case STATE_COMMAND:
				rw_byte = (rw_byte >> 1) | read_bit;
				bit_mask <<= 1;
				if (bit_mask == 0) {
					switch (rw_byte) {
						case 0xf0:
							w1_state = STATE_SEARCH_ROM;
							byte_counter = 0;
							bit_mask = 1;
							triplet_bit = 0;
							crc = 0;
							rw_byte = w1_id[byte_counter];
							break;
						case 0xcc:
							w1_state = STATE_SKIP_ROM;
							LED_IMP;
							LED_IMP;
							break;
						default:
							w1_state = STATE_IDLE;
					}
				}
	            T0_STOP;
		        break;
			case STATE_SEARCH_ROM:
				T0_STOP;
				if (triplet_bit == 2) {
					if ((rw_byte & bit_mask) ^ read_bit) {
						w1_state = STATE_IDLE;
					} else {
						triplet_bit = 0;
						bit_mask <<= 1;
						if (bit_mask == 0) {
								LED_IMP;
							crc8(rw_byte);
							bit_mask = 1;
							byte_counter++;
							if (byte_counter == 7) {
								rw_byte = crc;
							} else if (byte_counter == 8) {
								w1_state = STATE_IDLE;
							} else {
								rw_byte = w1_id[byte_counter];
							}
						}
					}
				} else {
					triplet_bit++;
				}
				break;
            default:
                T0_STOP;
        }
    } else {
        // was FALL
        switch(w1_state) {
            case STATE_RESET:
            case STATE_PRESENCE:
                W1_SEND_0;
                w1_state = STATE_PRESENCE;
                T0_RUN(T_PRESENCE);
				break;
			case STATE_COMMAND:
				read_bit = 1<<7;
				T0_RUN(T_READ_BIT);
				break;
			case STATE_SEARCH_ROM:
				if (rw_byte & bit_mask) {
					switch(triplet_bit) {
						case 0:
							T0_RUN(T_SEND_0);
							break;
						case 1:
							W1_SEND_0;
							T0_RUN(T_SEND_0);
							break;
						case 2:
							T0_RUN(T_READ_BIT);
							read_bit = bit_mask;
							break;
					}
				} else {
					switch(triplet_bit) {
						case 0:
							W1_SEND_0;
							T0_RUN(T_SEND_0);
							break;
						case 1:
							T0_RUN(T_SEND_0);
							break;
						case 2:
							T0_RUN(T_READ_BIT);
							read_bit = bit_mask;
							break;
					}
				}
				
				break;
            default:
				// Run T0 in all states by FALL
                T0_RUN(0);
        }
	}
}

ISR(TIMER0_COMPA_vect)
{
    T0_STOP;
    w1_state = STATE_RESET;
}

ISR(TIMER0_COMPB_vect)
{
    switch(w1_state) {
        case STATE_RESET:
            W1_SEND_0;
            break;
        case STATE_PRESENCE:
            W1_SEND_1;
            break;
		case STATE_COMMAND:
			read_bit = 0;
			break;
		case STATE_SEARCH_ROM:
			W1_SEND_1;
			read_bit = 0;
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

