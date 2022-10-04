#define F_CPU 4000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define THERM_PORT PORTD
#define THERM_DDR DDRD
#define THERM_PIN PIND
#define THERM_DQ PORTD2

#define THERM_INPUT_MODE() THERM_DDR&=~(1<<THERM_DQ)
#define THERM_OUTPUT_MODE() THERM_DDR|=(1<<THERM_DQ)
#define THERM_LOW() THERM_PORT&=~(1<<THERM_DQ)
#define THERM_HIGH() THERM_PORT|=(1<<THERM_DQ)

#define lamp_off() LED_PORT &= ~(1<<LED);
#define lamp_on() LED_PORT |= (1<<LED);

#define SEG_PORT PORTB
#define SEG_DDR DDRB
#define SIGN_PORT PORTD
#define SIGN_DDR DDRD
#define SIGN1 PORTD0
#define SIGN2 PORTD4
#define SIGN3 PORTD1
#define SIGN4 PORTD6

#define LED_PORT PORTD
#define LED_DDR DDRD
#define LED PORTD5

int a=1;

uint8_t code[12]= {0xFC, 0x60, 0xDA, 0xF2, 102, 182, 190, 224, 254, 246, 2};

uint8_t num[5] = {0, 0, 0, 0};
int sign_num = 1;
int dot_position=0;

ISR(TIMER0_OVF_vect)
{
	PORTB = code[num[sign_num-1]];
	SIGN_PORT |= (1<<SIGN1)|(1<<SIGN2)|(1<<SIGN3)|(1<<SIGN4);
	switch (sign_num)
	{
	case 1:
		SIGN_PORT &= ~(1<<SIGN1);
		if(dot_position==1) SEG_PORT++;
		break;
	case 2:
		SIGN_PORT &= ~(1<<SIGN2);
		if(dot_position==2) SEG_PORT++;
		break;
	case 3:
		SIGN_PORT &= ~(1<<SIGN3);
		if(dot_position==3) SEG_PORT++;
		break;
	case 4:
		SIGN_PORT &= ~(1<<SIGN4);
		break;
	}
	if (sign_num<4) sign_num++; else sign_num=1;

}

uint8_t therm_reset(){
	uint8_t i;
	THERM_LOW();
	THERM_OUTPUT_MODE();
	sei();
	_delay_us(480);
	THERM_INPUT_MODE();
	_delay_us(60);
	i=(THERM_PIN & (1<<THERM_DQ));
	_delay_us(420);
	cli();
	return i;
}

void therm_write_bit(uint8_t bit){
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(2);
	if(bit) THERM_INPUT_MODE();
	sei();
	_delay_us(60);
	cli();
	THERM_INPUT_MODE();
}

uint8_t therm_read_bit(void){
	uint8_t bit=0;
	THERM_LOW();
	THERM_OUTPUT_MODE();
	_delay_us(2);
	THERM_INPUT_MODE();
	_delay_us(16);
	if(THERM_PIN&(1<<THERM_DQ)) bit=1;
	sei();
	_delay_us(45);
	cli();
	return bit;
}

uint8_t therm_read_byte(void){
	uint8_t i=8, n=0;
	while(i--){
		n>>=1;
		n|=(therm_read_bit()<<7);
	}
	return n;
}

void therm_write_byte(uint8_t byte){
	uint8_t i=8;
	while(i--){
		therm_write_bit(byte&1);
		byte>>=1;
	}
}

#define THERM_CMD_CONVERTTEMP 0x44
#define THERM_CMD_RSCRATCHPAD 0xbe
#define THERM_CMD_SEARCHROM 0xf0
#define THERM_CMD_READROM 0x33
#define THERM_CMD_MATCHROM 0x55
#define THERM_CMD_SKIPROM 0xcc

#define THERM_DECIMAL_STEPS_12BIT 625 //.0625

uint8_t rom[8];
int sign = 1;
char temp;
char temp2;

uint8_t temperature[2];
uint8_t digit=0;
uint16_t decimal=0;

void therm_read_temperature(){
	cli();
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_CONVERTTEMP);
	sei();
	while(!therm_read_bit());
	cli();
	therm_reset();
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_RSCRATCHPAD);
	temperature[0]=therm_read_byte();
	temperature[1]=therm_read_byte();
	digit=temperature[0]>>4;
	digit|=(temperature[1]&0x7)<<4;
	temp = temperature[0];
	temp2 = temperature[1];
	decimal=temperature[0]&0xf;
	decimal*=THERM_DECIMAL_STEPS_12BIT;
	sign = 1;
	if ((temperature[1]&0xF8) == 0xF8)
	{
		digit=127-digit;
		decimal= (10000-decimal)%10000;
		if (decimal==0) digit++;
		sign = 0;
	}
	sei();
}

int main(void)
{
	DDRB = 0xFF;
	PORTB = 0xFF;
	_delay_ms(500);
	DDRD |= (1<<SIGN1)|(1<<SIGN2)|(1<<SIGN3)|(1<<SIGN4);
	PORTD |= (1<<SIGN1)|(1<<SIGN2)|(1<<SIGN3)|(1<<SIGN4);
	_delay_ms(500);
	PORTD &= ~(1<<SIGN1);
	_delay_ms(500);
	PORTD &= ~(1<<SIGN2);
	_delay_ms(500);
	PORTD &= ~(1<<SIGN3);
	_delay_ms(500);
	PORTD &= ~(1<<SIGN4);
	PORTD &= ~((1<<SIGN1)|(1<<SIGN2)|(1<<SIGN3)|(1<<SIGN4));
	_delay_ms(500);
	for (int j=0; j<8; j++) //тест сегментов
	{
		for (int i=0; i<10; i++)
		{
			PORTB |= (1<<j);
			_delay_ms(10);
			PORTB &= ~(1<<j);
			_delay_ms(10);
		}
	}
	
	/*for (int k=0; k<10; k++) //тест цифр
	{
		PORTB = code[k];
		_delay_ms(100);
	}*/
	
		//TCCR0B |= (1<<CS01); //8
		TCCR0B |= (1<<CS00)|(1<<CS01); //64
		//TCCR0B |= (1<<CS02); //256
		TIMSK |= (1<<TOIE0);
		TIFR |= (1<<TOV0);
		sei();
	while(1)
	{
		therm_read_temperature();
		if ((sign==1)&(digit>=100))
		{
			num[0]=digit%1000/100;
			num[1]=digit%100/10;
			num[2]=digit%100%10;
			num[3]=decimal%10000/1000;
			dot_position=3;
		}
		else if ((sign==1)&((digit>=10)&(digit<100)))
		{
			num[0]=digit%100/10;
			num[1]=digit%100%10;
			num[2]=decimal%10000/1000;
			num[3]=decimal%1000/100;
			dot_position=2;
		}
		else if ((sign==1)&((digit>=0)&(digit<10)))
		{
			num[0]=digit%100%10;
			num[1]=decimal%10000/1000;
			num[2]=decimal%1000/100;
			num[3]=decimal%100/10;
			dot_position=1;
		}
		else if ((sign==0)&((digit>=0)&(digit<10)))
		{
			num[0]=10;
			num[1]=digit%10;
			num[2]=decimal%10000/1000;
			num[3]=decimal%1000/100;
			dot_position=2;
		}
		else if ((sign==0)&((digit>=10)&(digit<100)))
		{
			num[0]=10;
			num[1]=digit%100/10;
			num[2]=digit%100%10;
			num[3]=decimal%10000/1000;
			dot_position=3;
		}
	}
	
}