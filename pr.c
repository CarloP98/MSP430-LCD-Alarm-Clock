#include "msp430xG46x.h"
#include <stdio.h>
#define SEG_A 0x01
#define SEG_B 0x02
#define SEG_C 0x04
#define SEG_D 0x08
#define SEG_E 0x40
#define SEG_F 0x10
#define SEG_G 0x20
#define SEG_H 0x80
#define SW1 BIT0&P1IN
#define SW2 BIT1&P1IN
#define NOTE_c 261
#define NOTE_d 294
#define NOTE_e 329
#define NOTE_f 349
#define NOTE_g 392
#define NOTE_a 440
#define NOTE_b 493
#define NOTE_C 523

unsigned char ch; // hold char from UART RX
unsigned char rx_flag; // receiver rx status flag

char TIME[4];
char ALARM[4];
char stage = 0;
unsigned short sec=0, Asec=1;
unsigned short min=0, Amin=1;
unsigned short hr=0, Ahr=1;
short pos=-1, correctTime =0, correctAlarm=0;
char snooze=0, stop=0, songPlaying=0;
char Msg0[] = "Enter current time: ";
char Msg1[] = "\r\nSet alarm time: ";
char error0[] = "\r\nTry again \r\nEnter current time: ";
char error1[] = "\r\nTry again \r\nSet alarm time: ";
char reset[] = "\r\n\nPress ESC to change Alarm time \r\n\nALARM
set for \r\n";
char currentTime[] = "Time: \r";

void SPI_Setup(void)
{
	UCB0CTL0 = UCMSB + UCMST + UCSYNC; // sync. mode, 3-pin SPI,
	UCB0CTL1 = UCSSEL_2 + UCSWRST; // SMCLK and Software reset
	UCB0BR0 = 0x02; // Data rate = SMCLK/2 ~= 500kHz
	UCB0BR1 = 0x00;
	P3SEL |= BIT1 + BIT2 + BIT3; // P3.1,P3.2,P3.3 option select
	UCB0CTL1 &= ~UCSWRST; // **Initialize USCI state
	machine**
}

unsigned char SPI_State(unsigned char State)
{
	while((P3IN & 0x01)); // Verifies busy flag
	IFG2 &= ~UCB0RXIFG;
	UCB0TXBUF = State; // set the state to start SPI
	while (!(IFG2 & UCB0RXIFG)); // USCI_B0 TX buffer ready?
	return UCB0RXBUF;
}
void UART0_putchar(char c)
{
	while (!(IFG2 & UCA0TXIFG));
	UCA0TXBUF = c;
}
void Serial_Initialize(void)
{
	P2SEL |= BIT4+BIT5; // Set UC0TXD and UC0RXD to transmit
	UCA0CTL1 |= BIT0; // Software reset
	UCA0CTL0 = 0; // USCI_A0 control register
	UCA0CTL1 |= UCSSEL_2; // Clock source SMCLK - 1048576 Hz
	UCA0BR0=54; // baud rate - 1048576 Hz / 19200
	UCA0BR1=0; // baud rate upper byte
	UCA0MCTL=0x0A; // Modulation
	UCA0CTL1 &= ~BIT0; // Software reset
	IE2 |=UCA0RXIE; // Enable USCI_A0 RX interrupt
}
void playSong(void)
{
	int song[] = { NOTE_C, NOTE_b, NOTE_g, NOTE_C, NOTE_b, NOTE_e, 0,
	NOTE_C, NOTE_c, NOTE_g, NOTE_a, NOTE_C };
	int beats[] = { 16, 16, 16, 8, 8, 16, 32, 16, 16, 16, 8, 8 };
	int note = 0;
	
	while( ((SW2) !=0) && ((SW1) !=0) )
	{
		TB0CTL = TBSSEL_1 + MC_1;
		int x = 32768/(2*song[note]);
		P2OUT |= 0x06;
		
		if(song[note]==0)
		{
			TB0CTL = TBSSEL_1 + MC_0;
			for(unsigned int x=0; x<(beats[note]*1000); x++);
			TB0CTL = TBSSEL_1 + MC_1;
		}
		else
		{
			TBCCR0 = x;
			for(unsigned int x=0; x<(beats[note]*1000); x++);
		}
		TB0CTL = TBSSEL_1 + MC_0;
		for(unsigned int x=0; x<1000; x++);
			note++;
		if(note == 12)
		{
			TB0CTL = TBSSEL_1 + MC_0;
			for(long x=0; x<90000; x++);
			note=0;
		}
	}

	P2OUT &= ~0x06;
	
	if((SW2) ==0)
	{
		songPlaying=0;
		TB0CTL = TBSSEL_1 + MC_0;
		if(min<50) Amin = min+10;
		else if(hr<23) { Ahr = hr+1; Amin = min-50; }
		else if(hr == 23){Ahr = 0; Amin = min-50;}
	}
	if((SW1) ==0)
	{
		songPlaying=0;
		TB0CTL = TBSSEL_1 + MC_0;
		Ahr = 0; Amin=0; Asec=0;
	}
}

void SetTime(void)
{
	sec++;
	if(sec == 60) { sec = 0; min++;}
	if(min == 60) { min = 0; hr++;}
	if(hr == 24) { hr=0; }
	if(min == Amin)
	if(hr == Ahr)
	playSong();
}
void SendTime(void) //change int to ASCII for Hypertrminal
{
	static int x=0;
	if (hr<10 ) { currentTime[7] = '0'; currentTime[8] = hr+'0'; }
	else if(hr>=10 && hr<20) { currentTime[7] = '1'; currentTime[8] = hr-10+'0'; }
	else if(hr>=20 ) { currentTime[7] = '2'; currentTime[8] = hr-20+'0'; }
	if(x==1) {currentTime[9] = 0x20; x=0;}
	else if(x==0) {currentTime[9] = 0x3A; x=1;}
	if (min<10 ) { currentTime[10] = '0'; currentTime[11] =min +'0'; }
	else if(min>=10 && min<20) { currentTime[10] = '1'; currentTime[11] =min-10+'0'; }
	else if(min>=20 && min<30) { currentTime[10] = '2'; currentTime[11] =min-20+'0'; }
	else if(min>=30 && min<40) { currentTime[10] = '3'; currentTime[11] =min-30+'0'; }
	else if(min>=40 && min<50) { currentTime[10] = '4'; currentTime[11] =min-40+'0'; }
	else if(min>=50 && min<60) { currentTime[10] = '5'; currentTime[11] =min-50+'0'; }
	for(int i=0; i<sizeof(currentTime); i++) UART0_putchar(currentTime[i]);
}

void changeLCD(unsigned int a, unsigned int b,int c) //change LCD according to what time it is
{
	static int x=0;
	if(a<10) LCDM9 = SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F;//0x:xx
	else if(a>=10 && a<20) LCDM9 = SEG_B+SEG_C;//1x:xx
	else if(a>=20) LCDM9 = SEG_A+SEG_B+SEG_G+SEG_E+SEG_D;//2x:xx
	if(a==9 || a==19) LCDM8 =SEG_F+SEG_A+SEG_B+SEG_C+SEG_G;//x9:xx
	if(a==8 || a==18) LCDM8=SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F+SEG_G; //x8:xx
	if(a==7 || a==17) LCDM8 =SEG_A+SEG_B+SEG_C;//x7:xx
	if(a==6 || a==16) LCDM8 =SEG_C+SEG_D+SEG_E+SEG_F+SEG_G;//x6:xx
	if(a==5 || a==15) LCDM8 =SEG_A+SEG_F+SEG_G+SEG_C+SEG_D;//x5:xx
	if(a==4 || a==14) LCDM8 =SEG_F+SEG_B+SEG_G+SEG_C;//x4:xx
	if(a==3 || a==13 || a==23) LCDM8 =SEG_A+SEG_B+SEG_C+SEG_D+SEG_G;//x3:xx
	if(a==2 || a==12 || a==22) LCDM8 =SEG_A+SEG_B+SEG_G+SEG_E+SEG_D;//x2:xx
	if(a==1 || a==11 || a==21) LCDM8 =SEG_B+SEG_C;//x1:xx
	if(a==0 || a==10 || a==20) LCDM8 =SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F;//x0:xx
	if(b<10) LCDM7 = SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F;//xx:0x
	else if(b>=10 && b<20) LCDM7 = SEG_B+SEG_C;//xx:1x
	else if(b>=20 && b<30) LCDM7 = SEG_A+SEG_B+SEG_G+SEG_E+SEG_D;//xx:2x
	else if(b>=30 && b<40) LCDM7 =SEG_A+SEG_B+SEG_C+SEG_D+SEG_G;//xx:3x
	else if(b>=40 && b<50) LCDM7 =SEG_F+SEG_B+SEG_G+SEG_C;//xx:4x
	else if(b>=50 && b<60) LCDM7 =SEG_A+SEG_F+SEG_G+SEG_C+SEG_D;//xx:5x
	if(b==9 || b==19 || b==29 || b==39 || b==49 || b==59) LCDM6=SEG_F+SEG_A+SEG_B+SEG_C+SEG_G; //xx:x9
	if(b==8 || b==18 || b==28 || b==38 || b==48 || b==58) LCDM6=SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F+SEG_G; //xx:x8
	if(b==7 || b==17 || b==27 || b==37 || b==47 || b==57) LCDM6=SEG_A+SEG_B+SEG_C; //xx:x7
	if(b==6 || b==16 || b==26 || b==36 || b==46 || b==56) LCDM6=SEG_C+SEG_D+SEG_E+SEG_F+SEG_G; //xx:x6
	if(b==5 || b==15 || b==25 || b==35 || b==45 || b==55) LCDM6=SEG_A+SEG_F+SEG_G+SEG_C+SEG_D; //xx:x5
	if(b==4 || b==14 || b==24 || b==34 || b==44 || b==54) LCDM6=SEG_F+SEG_B+SEG_G+SEG_C; //xx:x4
	if(b==3 || b==13 || b==23 || b==33 || b==43 || b==53) LCDM6=SEG_A+SEG_B+SEG_C+SEG_D+SEG_G; //xx:x3
	if(b==2 || b==12 || b==22 || b==32 || b==42 || b==52) LCDM6=SEG_A+SEG_B+SEG_G+SEG_E+SEG_D; //xx:x2
	if(b==1 || b==11 || b==21 || b==31 || b==41 || b==51) LCDM6 =SEG_B+SEG_C;//xx:x1
	if(b==0 || b==10 || b==20 || b==30 || b==40 || b==50) LCDM6=SEG_A+SEG_B+SEG_C+SEG_D+SEG_E+SEG_F; //xx:x0
	if(c==0)
	{
		if(x==1) {LCDMEM[6] |=BIT7; x=0;}
		else if(x==0) {LCDMEM[6] &=~BIT7; x=1;}
	}
	else LCDMEM[6] |=BIT7;
}

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
	_EINT();
	P2DIR |= 0x06;
	P2OUT &= ~0x06;
	P5OUT = 0; // All P5.x reset
	P5SEL = 0x1C; // P5.2/3/4 = LCD COM lines
	P5DIR = 0xFF; // All P5.x outputs
	TACTL = TASSEL_1 + MC_1;
	TACCR0 = 32768;
	TACCTL0 = CCIE;
	P3DIR |= BIT5;
	P3SEL |= BIT5;
	TB0CCTL4 = OUTMOD_4;
	TB0CTL = TBSSEL_1 + MC_1;
	Serial_Initialize();
	SPI_Setup();

	for (int z = 100; z > 0; z--); // Delay to allow baud rate stabilize
	for(int i = 0; i < sizeof(Msg0); i++) UART0_putchar(Msg0[i]);
	for (int i = 19; i > 0; i--) LCDMEM[i] = 0; // Clear LCD
	
	LCDACTL = LCDON + LCD4MUX + LCDFREQ_128; // 4mux LCD, ACLK/128
	LCDAPCTL0 = 0x7E;
	while (1)
	{
		while (!(rx_flag & 0x01)); // wait until receive the character from
		HyperTerminal
		rx_flag = 0; // clear rx_flag
		////////////////////////////////////////////////////////////////////////////////
		if(ch>=0x30 && ch<=0x39) //0-9
		{
			if(pos<3)
			{
				pos++;
				if(stage==0) TIME[pos] = ch;
				if(stage==1) ALARM[pos] = ch;
				UART0_putchar(ch);
				if(pos==1) UART0_putchar(0x3A);
			}
		}
		////////////////////////////////////////////////////////////////////////////////
		if(ch == 0x1B) //ESC
		{
			if(stage ==2)
			{
				stage=0;
				correctTime=1;
				pos=-1;
				ch = 0xD;
			}
		}
		////////////////////////////////////////////////////////////////////////////////
		if(ch == 0xD) //ENTER
		{
			if(pos==3)
			{
				if(stage==0) // check if time enter is correct
				{
					if(TIME[0] - '0'==2){
						if(TIME[1] - '0'<4)
						if(TIME[2] - '0'<6)
						correctTime=1;
					}
					else if(TIME[0] - '0'==0 || TIME[0] - '0'==1){
						if(TIME[2] - '0'<6)
						correctTime=1;
					}
					else correctTime=0;
				}
				else if(stage ==1) // check if alarm entered is correct
				{
					if(ALARM[0] - '0'==2){
						if(ALARM[1] - '0'<4)
						if(ALARM[2] - '0'<6)
						correctAlarm=1;
					}
					else if(ALARM[0] - '0'==0 || ALARM[0] - '0'==1){
						if(ALARM[2] - '0'<6)
						correctAlarm=1;
					}
					else correctAlarm=0;
				}
			}
			if(stage ==0)
			{
				if(correctTime==1) //if time entered is correct convert ASCIItime to int
				{
					static char i=0;
					if(i==0)
					{
						sec = 0;
						hr = ( (TIME[0] -'0')*10) + (TIME[1]-'0');
						min = ( (TIME[2] -'0')*10) + (TIME[3]-'0');
						i=1;
					}
					for(int i = 0; i < sizeof(Msg1); i++) UART0_putchar(Msg1[i]);
					stage = 1;
				}
				else
				{
					LCDM9 = SEG_A+SEG_F+SEG_G+SEG_C+SEG_D;
					for(int i = 0; i < sizeof(error0); i++)
					UART0_putchar(error0[i]);
				}
			}
			else if(stage ==1)
			{
				if(correctAlarm==1)
				{
					Ahr = ( (ALARM[0] -'0')*10)+(ALARM[1]-'0'); //ASCII to int
					Amin = ( (ALARM[2] -'0')*10)+(ALARM[3]-'0'); //ASCII to int
					reset[66]=ALARM[0];
					reset[67]=ALARM[1];
					reset[68]=0x3A;
					reset[69]=ALARM[2];
					reset[70]=ALARM[3];
					for(int i = 0; i < sizeof(reset); i++)
					UART0_putchar(reset[i]);
					SendTime();
					stage = 2;
				}
				else for(int i=0; i<sizeof(error1); i++)
				UART0_putchar(error1[i]);
			}
			pos=-1;
		}
		////////////////////////////////////////////////////////////////////////////////
	}
}
// Interrupt for USCI Rx
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIB0RX_ISR (void)
{
	ch = UCA0RXBUF; // character received is moved to a
	rx_flag=0x01; // signal main function receiving a
}
#pragma vector=TIMERA0_VECTOR
__interrupt void TIMERA_ISA(void)
{
	SetTime();
	if(stage==2)
	{
		SendTime();
		if( (SW1)!=0 && (SW2)==0 )changeLCD(Ahr,Amin,1);
		else changeLCD(hr,min,0);
	}
	TACCTL1 &= ~CCIFG;
}