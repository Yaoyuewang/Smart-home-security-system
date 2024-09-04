#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h" // Declarations of the LCD functions

#define NIBBLE_HIGH

void lcd_writenibble(unsigned char);

#define LCD_RS (1 << PB1)
#define LCD_E (1 << PB2)
#define LCD_Bits (LCD_RS | LCD_E)

#ifdef NIBBLE_HIGH
#define LCD_Data_D 0xf0 // Bits in Port D for LCD data
#define LCD_Status 0x80 // Bit in Port D for LCD busy status
#endif

void lcd_init(void)
{
#ifdef NIBBLE_HIGH
    DDRD |= LCD_Data_D; // Set PORTD bits 4-7 for output
#endif
    DDRB |= LCD_Bits;
    PORTB |= LCD_RS; // Set RS for data write

    _delay_ms(45);         // Delay at least 15ms
    lcd_writenibble(0x30); // Use lcd_writenibble to send 0b0011
    _delay_ms(5);          // Delay at least 4msec

    lcd_writenibble(0x30); // Use lcd_writenibble to send 0b0011
    _delay_us(120);        // Delay at least 100usec

    lcd_writenibble(0x30); // Use lcd_writenibble to send 0b0011, no delay needed
    _delay_ms(2);          // Delay at least 2ms

    lcd_writenibble(0x20); // Use lcd_writenibble to send 0b0010
    _delay_ms(2);          // Delay at least 2ms

    lcd_writecommand(0x28); // Function Set: 4-bit interface, 2 lines

    lcd_writecommand(0x0f); // Display and cursor on
}
void lcd_clear(void)
{
    lcd_moveto(0, 0);
    lcd_stringout("                                        ");
    lcd_moveto(1, 0);
    lcd_stringout("                                        ");
}
void lcd_clearLine1(void)
{
    lcd_moveto(0, 0);
    lcd_stringout("                                        ");
}
void lcd_clearLine2(void)
{
    lcd_moveto(1, 0);
    lcd_stringout("                                        ");
}

void lcd_moveto(unsigned char row, unsigned char col)
{
    unsigned char pos;
    pos = row * 0x40 + col;
    lcd_writecommand(0x80 | pos);
}

void lcd_stringout(char *str)
{
    char ch;
    while ((ch = *str++) != '\0')
        lcd_writedata(ch);
}

void lcd_writecommand(unsigned char x)
{
    PORTB &= ~LCD_RS; // Clear RS for command write
    lcd_writebyte(x);
    lcd_wait();
}

void lcd_writedata(unsigned char x)
{
    PORTB |= LCD_RS; // Set RS for data write
    lcd_writebyte(x);
    lcd_wait();
}

void lcd_writebyte(unsigned char x)
{
    lcd_writenibble(x);
    lcd_writenibble(x << 4);
}

void lcd_writenibble(unsigned char x)
{
#ifdef NIBBLE_HIGH
    PORTD &= ~LCD_Data_D;
    PORTD |= (x & LCD_Data_D); // Put high 4 bits of data in PORTD
#endif
    PORTB &= ~(LCD_E); // Set R/W=0, E=0
    PORTB |= LCD_E;    // Set E to 1
    PORTB |= LCD_E;    // Extend E pulse > 230ns
    PORTB &= ~LCD_E;   // Set E to 0
}
void lcd_wait()
{
#ifdef USE_BUSY_FLAG
    unsigned char bf;

#ifdef NIBBLE_HIGH
    PORTD &= ~LCD_Data_D; // Set for no pull ups
    DDRD &= ~LCD_Data_D;  // Set for input
#else
    PORTB &= ~LCD_Data_B; // Set for no pull ups
    PORTD &= ~LCD_Data_D;
    DDRB &= ~LCD_Data_B; // Set for input
    DDRD &= ~LCD_Data_D;
#endif

    PORTB &= ~(LCD_E | LCD_RS); // Set E=0, R/W=1, RS=0

    do
    {
        PORTB |= LCD_E;         // Set E=1
        _delay_us(1);           // Wait for signal to appear
        bf = PIND & LCD_Status; // Read status register high bits
        PORTB &= ~LCD_E;        // Set E=0
        PORTB |= LCD_E;         // Need to clock E a second time to fake
        PORTB &= ~LCD_E;        //   getting the status register low bits
    } while (bf != 0);          // If Busy (PORTD, bit 7 = 1), loop

#ifdef NIBBLE_HIGH
    DDRD |= LCD_Data_D; // Set PORTD bits for output
#else
    DDRB |= LCD_Data_B; // Set PORTB, PORTD bits for output
    DDRD |= LCD_Data_D;
#endif
#else
    _delay_ms(2); // Delay for 2ms
#endif
}
