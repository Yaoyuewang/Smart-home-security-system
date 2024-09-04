#include "lcd_msg.h"
#include "lcd.h"
#include <util/delay.h>
bool initialMenuDisplayed = false;
bool messageDisplayed = false;

void displayInitialMenu()
{
    // Display the initial menu only if it hasn't been displayed yet
    if (!initialMenuDisplayed)
    {
        lcd_clear();
        lcd_moveto(0, 0);
        lcd_stringout("Select Arming Mode");
        lcd_moveto(1, 0);
        lcd_stringout("1. Normal");
        lcd_moveto(0, 20);
        lcd_stringout("2. Vacation");
        lcd_moveto(1, 20);
        lcd_stringout("MODE: OFF");
        initialMenuDisplayed = true; // Mark as displayed
    }
}
void displaySwitch1Message()
{
    // Ensure this message causes the initial menu to be considered for redisplay if necessary
    if (!messageDisplayed || initialMenuDisplayed)
    {
        lcd_clear();
        lcd_moveto(0, 0);
        lcd_stringout("  Selected Normal");
        lcd_moveto(1, 0);
        lcd_stringout(" Enter Pass to LOCK");
        messageDisplayed = true;
        initialMenuDisplayed = false; // Reset this flag if going back to the initial menu later
    }
}

void displaySwitch2Message()
{
    if (!messageDisplayed || initialMenuDisplayed)
    { // Similar guard as for switch1
        lcd_clear();
        lcd_moveto(0, 0);
        lcd_stringout("  Selected Vacation");
        lcd_moveto(1, 0);
        lcd_stringout(" Enter Pass to LOCK");
        messageDisplayed = true; // Mark as displayed
        initialMenuDisplayed = false;
    }
}
void unlocked(void)
{
    lcd_clear();
    lcd_moveto(1, 0);
    lcd_stringout(" Security Disarmed");
    _delay_ms(1000);
}
void lockedMessage(void)
{
    lcd_clear();
    lcd_moveto(0, 0);
    lcd_stringout("Enter Pass to UNLOCK");
    lcd_moveto(1, 0);
    lcd_stringout("   Security Armed");
}
void errorMessage()
{
    lcd_clear();
    lcd_moveto(0, 0);
    lcd_stringout("Incorrect Pass - Try Again");
}
void intruderMessage()
{
    lcd_clear();
    lcd_moveto(0, 0);
    lcd_stringout("INTRUDER ALERT!     POLICE NOTIFIED!");
}

void alarmcountdown()
{
    lcd_clear();
    lcd_stringout("COUNTDOWN TO ALARM:");
}