#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <util/delay.h>
#include "lcd.h"

#define					CLI() cli();
#define					SEI() sei();
unsigned char			led, count_int,g_led=0;
int						sendflag =0;
char					ch=1;


//���� ���� 
unsigned int			base_number[3];

unsigned char			segment_which[4] = {0xf7, 0xfb, 0xfd, 0xfe};
enum{
	SEGMENT_1 = 1,   //1�� �ڸ�
	SEGMENT_2,   //2���ڸ�
	SEGMENT_3,   //3���ڸ�
	SEGMENT_4,   //4���ڸ�
};

#define EX_LCD_DATA		(*(volatile unsigned char *)0x8000)//LCD
#define EX_LED			(*(volatile unsigned char *)0x8008)//LED
#define EX_SS_DATA      (*(volatile unsigned char *)0x8002)   //7Segment   //����
#define EX_SS_SEL		(*(volatile unsigned char *)0x8003) //7Segment   //����

unsigned char			segement_data[10] = 
{0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f};   //����
unsigned char			display_7sg_num[3] = {0, 1, 2};

void					delay(int n);
void					init(); 
void					adc_init(void);
void					uart0_init(void);



ISR(TIMER0_COMP_vect)
{
	count_int++;
	if(count_int==55){
		led=led^0xff;
		PORTD=led;
		count_int=0;

	}
}
void startConvertion()
{
	ADCSRA = 0xc7;
	ADMUX = 0x00;
	ADCSRA = ADCSRA | 0xc7;
}

unsigned int readConvertData(void)
{
	volatile unsigned int temp;
	while((ADCSRA & 0x10) == 0);
	ADCSRA = ADCSRA | 0x10;
	temp = (int)ADCL + (int)ADCH*256;
	ADCSRA = ADCSRA | 0x10;
	return temp;
}


void checkNumber_Strike(int n1, int n2, int n3);
int checkNumber_Ball(int *n);

//���� ���� ���� 
void SetBaseNumber()
{
	srand((unsigned int) time(NULL));
	while( base_number[0] == base_number[1] || 
		base_number[1] == base_number[2] || 
		base_number[0] == base_number[2])
	{
		base_number[0] = rand() % 9;
		base_number[1] = rand() % 9;
		base_number[2] = rand() % 9;
	}
}

void main(void){
	unsigned int keydata;
	int key_array = 0;
	int key_count[3] = {0, 0, 0}; //Ű ��� ���ȴ��� ī��Ʈ ��

	int nSaveStrikeResult[8];		//����� ��µ��� ����
	int nSaveBallResult[8];		//����� ��µ��� ����
	int nSave_i = 0;		//����� �ø���


	unsigned int adc_data;	//�������װ� 

	init();
	lcdInit();
	SetBaseNumber();	//��� ���� �ʱ�ȭ

	lcd_puts(1, "Game Start!");
	lcd_puts(2, "Hello Base Ball");
	delay(10000);		//10�� ���
	lcdClear();			//ȭ�� Ŭ��������

	while(1){
		//�������� 
		startConvertion();
		adc_data = readConvertData();
		
		int a = (key_count[0] * 1) + (key_count[1] * 10) + (key_count[2] * 100);
		keydata = (PINB & 0xff);
		switch(keydata)
		{
		case 0x01 :   //1�� Right
			if( key_array -1 <= -1)
				break;
			key_array--;
			delay(50);
			break;
		case 0x02:   //2�� Select
			lcd_gotoxy(8,2);
			lcd_putn3(a);	//SELECT������ �� .
			lcd_gotoxy(8,1);

			nSaveStrikeResult[nSave_i] = 
				checkNumber_Strike(key_count); //��Ʈ����ũ üũ
			nSaveBallResult[nSave_i] = 
				checkNumber_Ball(key_count); //��Ʈ����ũ üũ



			//����ϰ� ���������� ����Ʈ�ϼ� lcdRegWrite(0x1c) �������̵�
			//lcdRegWrite(0x18)�����̵�
			delay(100);
			break;
		case 0x04:   //3�� Left
			if( key_array +1 >= 3)
				break;
			key_array++;
			delay(50);
			break;
		case 0x08:   //4�� Down
			if(key_count[key_array] - 1 <= -1)
				break;
			key_count[key_array]--;
			delay(50);
			break;
		case 0x10:   //5�� Up
			if(key_count[key_array] + 1 >= 10)
				break;
			key_count[key_array]++;
			delay(50);
			break;
		}

		EX_SS_SEL = segment_which[SEGMENT_1];
		EX_SS_DATA = segement_data[key_count[0]];
		delay(5);
		EX_SS_SEL = segment_which[SEGMENT_2];
		EX_SS_DATA = segement_data[key_count[1]];
		delay(5);
		EX_SS_SEL = segment_which[SEGMENT_3];
		EX_SS_DATA = segement_data[key_count[2]];
		delay(5);
		//���ΰ��� ������ �ٽ��ϰ� ó��
		//�������װ� �ϸ� LCD�� �ű�� 

	}
}
void delay(int n)
{
	volatile i,j;
	for(i=1; i<n; i++) {
		for(j=1; j<600; j++);
	}
}



/**********************************************
				�ʱ�ȭ �Լ�
************************************************/
void init()
{
	CLI();
	XDIV = 0x00;
	XMCRA = 0x00;
	adc_init();
	PORTA = 0x00;
	DDRA = 0xFF;
	PORTB =0x00;
	DDRB = 0x00;  //ket Switch�� �Է����� ����
	PORTC =0x00;
	DDRC = 0x03;
	DDRD=0xFF;    //LED
	PORTD=0xFF;    //LED
	PORTG =0x00;
	DDRG = 0x00; //Write, ale ��ȣ
	TCCR0=0x1F;    //CTC���� ���������Ϸ� 1024 COM!*0=1 �񱳸�ġ ��� ���
	OCR0=0xff;    //8��Ʈ �˲�ä��
	TCNT0 = 0x00;   //0���� 8��Ʈ
	TIMSK=0x02;    //��º����ͷ�Ʈ ���
	EX_SS_SEL = 0x0f;   //�ʱⰩ ����Ʈ OFF
	MCUCR = 0x80;    
	ASSR=0x00;
	SEI();
}
void adc_init(void)
{
	ADCSRA = 0x00;
	ADMUX = 0x20;
	ACSR = 0x80;
	ADCSRA = 0xc7;
}
void uart0_init(void)
{
	UCSR0B = 0x00;
	UCSR0A = 0x00;
	UCSR0C = 0x06;
	UBRR0L = 0x67;
	UBRR0H = 0x00;
	UCSR0B = 0x18;
}


/**********************************************
				���� üũ �Լ�
************************************************/
void checkNumber_Strike(int *n)
{
	int st = 0;
	int i = 0;

	for( i =0 ; i < 3; i++) 	
	{
		if( n[i] == base_number[i] )
			st++;
	}
	return st;
}
int checkNumber_Ball(int *n)
{
	int ba = 0;
	if( n[0] == base_number[1] || n[0] == base_number[2])
		ba++;
	if( n[1] == base_number[0] || n[1] == base_number[2])
		ba++;
	if( n[2] == base_number[1] || n[2] == base_number[0])
		ba++;

	return ba;
}