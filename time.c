#include <avr/io.h>
#include <avr/eeprom.h>
#include "time.h"
#include "lcd.h"

// Variable definitions
volatile int seconds, minutes, hours;

volatile bool timeToUpdate = false;
uint8_t EEMEM clockSetFlag;
char timeString[9]; // HH:MM:SS

bool isClockSet()
{
    // Read the flag from EEPROM
    return eeprom_read_byte(&clockSetFlag) == 1;
}

void markClockAsSet()
{
    // Write the flag to EEPROM
    uint8_t flag = 1;
    eeprom_write_byte(&clockSetFlag, flag);
}

void displayCurrentTime(void)
{
    // Format the time string
    snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", hours, minutes, seconds);
    // lcd_clearLine2();
    lcd_moveto(1, 32);         // Adjust row and column as needed for your LCD
    lcd_stringout(timeString); // Show time
}


