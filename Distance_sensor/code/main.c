/*
* DistanceSensorNew.c
*
* Created: 3/23/2013 4:30:29 PM * Author: Erl1 & Remsy & Kristian */
#define F_CPU 16000000 //Define MCU clock speed, to handle delays
#include <avr/io.h> #include <avr/delay.h> #include <avr/interrupt.h>
//LCD library by P.Fleury
#include "lcd.h" char buffer[7];
//Pulse detection variables
volatile uint16_t send = 0;
volatile uint16_t pulseCounter = 0;
volatile uint8_t flag = 1;
//Time variables
volatile uint16_t microSecondsCopy = 0; volatile uint16_t milliSecondsCopy = 0;
volatile uint16_t microSeconds = 0; volatile uint16_t milliSeconds = 0; volatile uint16_t seconds = 0;
float lengthMilli = 0; float lengthMicro = 0; float totalLength = 0;
int meter = 0; int cm = 0;
//Sends a 1ms pulse
void sendPulse() {
DDRB |= (1 << PB3); //Enable output for 40kHz (timer 0) TCCR1B |= (1 << CS11); //Start stopwatch (timer 1) _delay_ms(1); //wait 1ms
DDRB &= ~(1 << PB3); //Disable output for 40kHz (timer 0)
}
//Calculates range in meter and cm from milliseconds and microseconds
void calculateRange() {
// Contribution from the int milliSeconds
lengthMilli = milliSecondsCopy*340; // Multiply with the speed of sound (340m/s) lengthMilli /= (2*1000); // Divide by 2 to get distance to object
// Divide the int microSeconds to prevent overflow
microSecondsCopy /= 10;
// Contribution from the int mincroSeconds. Otherwise same procedure as for int milliSeconds
lengthMicro = microSecondsCopy*340; lengthMicro /= 1000;
lengthMicro /= (2*100);
// Combining the above contributions
totalLength = lengthMicro + lengthMilli;
// Get total length in meters
meter = totalLength;
// Get rest of total length in cm
totalLength -= meter; totalLength = 100*totalLength; cm = totalLength;
}
//Print range to display
void printRange() { lcd_gotoxy(0,0);
	lcd_puts("Range:"); utoa(meter,buffer,10); lcd_puts(buffer); lcd_puts("."); utoa(cm,buffer,10); lcd_puts(buffer); lcd_puts("m");
}
//Print stopwatch time to display
void printTime() {
//If there is a pulse if (seconds < 3)
	{
		lcd_gotoxy(0,1); lcd_puts("Time:"); utoa(seconds,buffer,10); lcd_puts(buffer);
		_delay_ms(50);
		lcd_puts(":"); utoa(milliSecondsCopy,buffer,10); lcd_puts(buffer);
		_delay_ms(50);
		lcd_puts(":"); utoa(microSecondsCopy,buffer,10); lcd_puts(buffer);
		_delay_ms(50);
	}
//If no pulse is detected
	else {
		lcd_gotoxy(0,1);
		lcd_puts("No pulse...");
PORTB &= ~(1 << PB0); //red LED on PORTB |= (1 << PB1); //red LED on _delay_ms(50);
} }
//Housekeeping
void clearVars() { microSecondsCopy = 0; milliSecondsCopy = 0;
	milliSeconds = 0; microSeconds = 0; seconds = 0;
	flag = 1;
	pulseCounter = 0;
	send = 0; }
//Test-pulse only used when attached to scope. Only used during testing
	void testPulse() { while(1) {
		sendPulse();
		_delay_ms(25); }
	}
	int main(void)
	{
//IO-config
DDRB |= (1 << PB0) | (1 << PB1); //Set LED as output PORTB &= ~((1 << PB0) | (1 << PB1)); //Set LED to low
DDRA |= (0 << PA7); //Signal read pin PORTA |= (0 << PA7); //No pullup
DDRB &= ~(1 << PB3); //40kHz output
DDRD |= (0 << PD2) | (0 << PD3); //Button input on PD2 and PD3
PORTD |= (1 << PD2) | (1 << PD3); //Enable pullup resistor on PD2 and PD3
//External interrupt config
GICR |= (1 << INT1); //Enable interrupt on button 1
GICR &= ~(1 << INT0); //Disable interrupt on button 2 MCUCR |= (1 << ISC11) | (1 << ISC01); //Detect falling edge GIFR |= (1 << INTF0) | (1 << INTF1); //Create interrupt vectors
//8-Bit timer0 config (40kHz pulse)
TCCR0 |= (1 << CS01); //Start timer0
TCCR0 |= (1 << WGM01) | (1 << COM00); //Enable CTC-mode and output on PB3 OCR0 = 0x18; //CTC Overflow value
//16-Bit timer config (2 MHz)
TCCR1B &= ~(1 << CS11); //16MHz div 8 = 2Mhz
TCCR1B |= (1 << WGM12); //Enable CTC mode
TCNT1 = 0; //Clear counter
OCR1A = 49; //CTC Overflow value
TIMSK |= (1 << TICIE1) | (1 << OCIE1A); //Enable timer-interrupt
sei(); //Enable global interrupts
//LCD initialize
lcd_init(LCD_DISP_ON); lcd_gotoxy(0,0); lcd_puts("RangeFinder"); lcd_gotoxy(0,1); lcd_puts("By:E.C.K.");
while(1)
{
	if (send)
	{
clearVars(); //Clear all variables sendPulse(); //Send 1ms pulse _delay_ms(1); //wait 1ms
//testPulse(); //Used only for scope testing //Check if received pulse is valid
while ((seconds < 4) && flag){ //Detects a rising edge on receiver
//If button is pressed
	while(PINA & (1 << PA7)) {
//Store time variables when detected for use in calculating the distance if (pulseCounter < 1)
		{
			microSecondsCopy = microSeconds;
			milliSecondsCopy = milliSeconds; }
//Sample pulse 5 times
			_delay_ms(0.1);
			pulseCounter++;
//If no pulse detected after 4 s, break if (seconds > 4)
			{
				break; }
			}
//5 successful samples = pulse received
			if (pulseCounter > 5)
			{
				flag = 0;
			}
			pulseCounter = 0; }
TCCR1B &= ~(1 << CS11); //Stop timer1 (stopwatch) TCNT1 = 0; //Reset timer1 count
PORTB &= ~(1 << PB0); //Turn green led off
GICR |= (1 << INT1); //Enable trigger button
lcd_clrscr(); //Clear LCD calculateRange(); //Find range printRange(); //Print range printTime(); //Print time
} }
}
//Trigger button
ISR(INT1_vect) {
send = 1; //Tells while-loop to send pulse
PORTB |= (1 << PB0); //Turn green led on
PORTB &= ~(1 << PB1); //Turn red led off
GICR &= ~(1 << INT1); //Disable further button-presses _delay_ms(250);
}
//Extra functions button (will use for something cool if time)
ISR(INT0_vect) {
	lcd_clrscr();
	lcd_gotoxy(0,0); lcd_puts("RangeFinder"); lcd_gotoxy(0,1);
	lcd_puts("By:E.C.K.");
PORTB |= (1 << PB0); //Turn green led on PORTB |= (1 << PB1); //Turn red led off _delay_ms(250);
}
//Stopwatch with a resolution of 25us
ISR(TIMER1_COMPA_vect) { microSeconds += 25;
	if (microSeconds == 1000) {
		milliSeconds++; microSeconds = 0;
		if (milliSeconds == 1000)
		{
			seconds++;
			milliSeconds = 0; 
		}
	}
}
