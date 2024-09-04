#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lcd.h" // Declarations of the LCD functions
#include "keypadSelf.h"
#include "time.h"
#include "lcd_msg.h"

#define FOSC 9830400                      // Clock frequency = Oscillator freq.
#define BAUD 9600                         // UART0 baud rate
#define MYUBRR FOSC / 16 / BAUD - 1       // Value for UBRR0 register
#define BDIV (FOSC / 100000 - 16) / 2 + 1 // Puts I2C rate just below 100kHz

/* Address of the EEPROM on the I2C bus */

#define switch1 !(PIND & (1 << PD0))
#define switch2 !(PIND & (1 << PD1))
#define doorOpen (PIND & (1 << PD2))
#define ouput1 (PORTC & (1 << PC0))
/* Address where we store test data in the EEPROM */
#define DATA_ADDR 500
uint8_t rdata[128];

uint8_t readBuf[32];
uint8_t writeBuf[32];

uint8_t writeBuf1[32];
uint8_t readBuf1[32];

char setUpKey[5]; // SETTING

char passKey[5];
char inputChecker[5];
uint8_t status;

#define OSTR_SIZE 80

char ostr[OSTR_SIZE];

// void printArray(char *array, int size)
// {
//     lcd_clear();      // Clear the LCD screen
//     lcd_moveto(0, 0); // Move cursor to the beginning

//     // Temporarily create a string to hold each character for printing
//     char tempStr[2] = {0}; // 2 characters: one for the current char, one for null terminator

//     for (int i = 0; i < size; i++)
//     {
//         tempStr[0] = array[i];  // Set the current character into the temp string
//         lcd_stringout(tempStr); // Print the temp string (current character) to the LCD
//     }
// }

void setupDipSwitch()
{
    // Can Remove PD2 - Not needed
    DDRD &= ~((1 << PD0) | (1 << PD1) | (1 << PD2)); // Set PD0, PD1, and PD2 as input
    PORTD |= (1 << PD0) | (1 << PD1) | (1 << PD2);   // Enable pull-up resistors for PD0, PD1, and PD2
}

bool isDipSwitchOff()
{
    return (PIND & (1 << PD0)) && (PIND & (1 << PD1));
}
// Function Definition

ISR(TIMER1_COMPA_vect)
{
    readBuf1[0] = 0x01; // Start from the seconds register

    // Read current time from RTC
    status = i2c_io(clockAdd, writeBuf1, 1, readBuf1, 7);

    // Convert from BCD to decimal
    seconds = (readBuf1[0] / 16 * 10) + (readBuf1[0] % 16);
    minutes = (readBuf1[1] / 16 * 10) + (readBuf1[1] % 16);
    hours = (readBuf1[2] / 16 * 10) + (readBuf1[2] % 16);
    timeToUpdate = true; // unused
}

void setupTimer1()
{
    TCCR1B |= (1 << WGM12);  // Configure timer 1 for CTC mode
    TIMSK1 |= (1 << OCIE1A); // Enable CTC interrupt
    sei();                   // Enable global interrupts
    OCR1A = 31249;           // Set CTC compare value for 1Hz at 16MHz AVR clock, with a prescaler of 256
    TCCR1B |= (1 << CS12);   // Start timer at Fcpu/256
}

void led_locked(int *led_start, bool locked_vac, bool intruder_door, bool intruder_password)
{
    int led_stop = seconds;

    led_stop = abs(led_stop - *led_start);

    if (!locked_vac || intruder_door || intruder_password)
    {
        PORTB &= ~(1 << PB7);
    }
    else if ((locked_vac && (led_stop > 2)))
    {                        //
        PORTB ^= (1 << PB7); // Toggle PD3
        *led_start = seconds;
    }
}
void led_locked2(int *led_start2, bool locked_vac, bool intruder_door, bool intruder_password)
{
    int led_stop = seconds;

    led_stop = abs(led_stop - *led_start2);

    if (!locked_vac || intruder_door || intruder_password)
    {
        PORTB &= ~(1 << PB0);
    }
    else if ((locked_vac && (led_stop > 15)))
    {                        //
        PORTB ^= (1 << PB0); // Toggle PD3
        *led_start2 = seconds;
    }
}
void soundTrigger(int *led_start, bool locked_vac)
{
    int led_stop = seconds;

    if (locked_vac && (led_stop % 20 == 0)) //&& (led_stop == 10)
    {                                       //
        PORTC &= ~(1 << PC2);               // Toggle PD3
        _delay_ms(50);
    }
    else
    {
        PORTC |= (1 << PC2);
    }
}

void led_intruder()
{
    PORTC ^= (1 << PC0);
    _delay_ms(50);
    PORTC ^= (1 << PC0);
}

bool door_locked(bool locked, bool locked_vac)
{
    // PIND &= ~(1 << PD2); //set to low for now as there is no switch attached
    if ((PIND & (1 << PD2)) && (locked || locked_vac))
    {
        return true;
    }
    else
        return false;
}

int main(void)
{
    setupDipSwitch();
    DDRB |= (1 << PB7); // Set PB7 as output LED - pin 10
    DDRB |= (1 << PB0); // Set PB6 as output LED - pin 9

    DDRC |= (1 << PC3); // Set PD3 as output for LED
    DDRC |= (1 << PC2); // Set PD3 as output for LED
    DDRC |= (1 << PC1); // Set PD3 as output for LED

    PORTC |= (1 << PC3);
    PORTC |= (1 << PC2);
    PORTC |= (1 << PC1);

    DDRC |= (1 << PC0); // Set PD3 as output for LED
    DDRD |= (1 << PD2); // Set PD2 as output for reed switch
    // Initialize the I2C ports
    uint8_t status;
    i2c_init(BDIV);   // Initialize the I2C ports
    sci_init(MYUBRR); // Initialize the SCI ports
    char rdata[5];
    char countDown[5];

    int counter = 0;
    int enterPassCounter = 0, attemptCounter = 0;
    bool locked = false, locked_vac = false;
    int led_start = seconds, led_start2 = seconds, countdown = 0, count = 10;
    bool intruder_door = false, intruder_password = false;

    lcd_init();

    if (isClockSet())
    {
        lcd_clear();
        lcd_moveto(0, 0);
        lcd_stringout("Clock not set");
        _delay_ms(2000);
        writeBuf1[0x00] = 0x00; // DS1307 RTC seconds register address
        writeBuf1[0x01] = 0x00; // Seconds reset to 0 in BCD format for exact time setting
        writeBuf1[0x02] = 0x57; // Minutes in BCD format
        writeBuf1[0x03] = 0x11; // Hours in BCD format
        writeBuf1[0x04] = 0x00; // Day of the week in BCD format (Wednesday)
        writeBuf1[0x05] = 0x00; // Date in BCD format
        writeBuf1[0x06] = 0x00; // Month in BCD format
        writeBuf1[0x07] = 0x36; // Year (last two digits) in BCD format

        status = i2c_io(clockAdd, writeBuf1, 8, NULL, 0);
        markClockAsSet();
    }
    // Initiate LCD Screen

    // // Clear the LCD Screen - Any left over text on the LCD
    lcd_clear();
    lcd_moveto(0, 0);
    lcd_stringout("   Welcome to TYK         Security");
    _delay_ms(1000); // Quick delay before moving on to the next screen
    lcd_clear();
    lcd_moveto(0, 0);
    lcd_stringout("   Password Setup");
    lcd_moveto(1, 0);
    lcd_stringout(" Enter 4 digit pass");
    lcd_moveto(1, 28);
    while (enterPassCounter < 15) // Clear the keypad FIFO
    {
        writeBuf[0] = 0x06;
        writeBuf[1] = 0x01;
        status = i2c_io(keypadAdd, writeBuf, 2, NULL, 0);
        _delay_ms(10); // Small delay to allow the device to process the command
        writeBuf[0] = 0x03;
        status = i2c_io(keypadAdd, writeBuf, 1, readBuf, 1); // Send the read command
        enterPassCounter++;
    }
    while (counter < 4)
    {
        // Write to register 6 first
        writeBuf[0] = 0x06;
        writeBuf[1] = 0x01;
        status = i2c_io(keypadAdd, writeBuf, 2, NULL, 0);
        _delay_ms(40); // Small delay to allow the device to process the command

        // Prepare to read the keypad state
        writeBuf[0] = 0x03;
        status = i2c_io(keypadAdd, writeBuf, 1, readBuf, 1); // Send the read command

        // Check if a key was pressed. Assuming 0x00 means no key press.
        if (readBuf[0] != 0x00)
        {
            // Convert the read value to a string
            snprintf(rdata, 4, "%c", readBuf[0]);
            // Display the string on the LCD
            _delay_ms(10);
            lcd_stringout("*");
            passKey[counter] = rdata[0]; // Assuming passKey is a character array

            // Delay to debounce and avoid repeated reads from the same key press
            _delay_ms(40);
            counter++;
        }
        else if (counter == 4)
        {
            lcd_clear();
            lcd_moveto(0, 0);
            lcd_stringout("    Password Set"); // Printing out the passkey
            lcd_moveto(1, 0);
            lcd_stringout("    Welcome Home");
            _delay_ms(1000);
            break;
        }
        // Short delay to throttle the loop and reduce bus traffic
        _delay_ms(40);
    }

    setupTimer1();
    // BEGIN READING SEQUENCE
    // NEED TO FIX INTRUDER DOOR alarm -> disarm
    while (1)
    {
        displayCurrentTime();
        led_locked(&led_start, locked_vac, intruder_door, intruder_password);
        led_locked2(&led_start2, locked_vac, intruder_door, intruder_password);
        soundTrigger(&led_start, locked_vac);
        intruder_door = door_locked(locked, locked_vac);
        if (intruder_password)
        {
            intruderMessage();
            led_intruder();
            // Call function to play a sound
            PORTC &= ~(1 << PC3);
        }
        else if (intruder_door)
        {
            lcd_clear();
            alarmcountdown();
            lcd_moveto(1, 28);
            // clear FIFO buffer first
            while (enterPassCounter < 15) // Clear the keypad FIFO
            {
                writeBuf[0] = 0x06;
                writeBuf[1] = 0x01;
                status = i2c_io(keypadAdd, writeBuf, 2, NULL, 0);
                _delay_ms(10); // Small delay to allow the device to process the command
                writeBuf[0] = 0x03;
                status = i2c_io(keypadAdd, writeBuf, 1, readBuf, 1); // Send the read command
                enterPassCounter++;
            }
            // poll for 4 keypresses *** while loop
            enterPassCounter = 0;
            countdown = seconds;
            while (enterPassCounter < 4)
            {
                lcd_moveto(1, 28 + enterPassCounter);

                // Write to register 6 first
                writeBuf[0] = 0x06;
                writeBuf[1] = 0x01;
                status = i2c_io(keypadAdd, writeBuf, 2, NULL, 0);
                _delay_ms(40); // Small delay to allow the device to process the command

                // Prepare to read the keypad state
                writeBuf[0] = 0x03;
                status = i2c_io(keypadAdd, writeBuf, 1, readBuf, 1); // Send the read command

                // Check if a key was pressed. Assuming 0x00 means no key press.
                if (readBuf[0] != 0x00)
                {                                                // If a key was pressed
                    inputChecker[enterPassCounter] = readBuf[0]; // Store the pressed key

                    lcd_stringout("*"); // Display an asterisk for each character entered
                    enterPassCounter++; // Move to the next character
                    _delay_ms(40);      // Add some delay to debounce the keypad input
                }
                if (countdown != seconds)
                {
                    lcd_moveto(0, 28);
                    count--;
                    snprintf(countDown, sizeof(countDown), "%d", count);
                    lcd_stringout(countDown);
                    countdown = seconds;
                    if (count == 0)
                    {
                        intruder_door = false;
                        intruder_password = true;
                        // Call function to play a sound
                        PORTC &= ~(1 << PC3);
                        break;
                    }
                }
            }
            if (strcmp(inputChecker, passKey) == 0)
            {
                lockedMessage();
                locked = false;     // Assuming 'locked' is a variable you use to track the lock state
                attemptCounter = 0; // Reset the attempt counter
                enterPassCounter = 0;
                intruder_door = false, intruder_password = false;
                count = 10;
            }
            // Reset the counter and buffer for the next password entry attempt
            enterPassCounter = 0;
            memset(inputChecker, 0, sizeof(inputChecker));
        }
        else if (!switch1 && !switch2)
        {
            // If neither switch is active, consider redisplaying the initial menu
            messageDisplayed = false; // Allow specific switch messages to be shown if switches are activated
            locked = false;
            displayInitialMenu();
        }
        else if (switch1 && !intruder_door && !intruder_password)
        {
            while (enterPassCounter < 15) // Clear the keypad FIFO
            {
                writeBuf[0] = 0x06;
                writeBuf[1] = 0x01;
                status = i2c_io(keypadAdd, writeBuf, 2, NULL, 0);
                _delay_ms(10); // Small delay to allow the device to process the command
                writeBuf[0] = 0x03;
                status = i2c_io(keypadAdd, writeBuf, 1, readBuf, 1); // Send the read command
                enterPassCounter++;
            }
            displaySwitch1Message();
            enterPassCounter = 0;
            while (enterPassCounter < 4)
            {
                lcd_moveto(1, 28 + enterPassCounter);
                // Write to register 6 first
                writeBuf[0] = 0x06;
                writeBuf[1] = 0x01;
                status = i2c_io(keypadAdd, writeBuf, 2, NULL, 0);
                _delay_ms(40); // Small delay to allow the device to process the command

                // Prepare to read the keypad state
                writeBuf[0] = 0x03;
                status = i2c_io(keypadAdd, writeBuf, 1, readBuf, 1); // Send the read command

                // Check if a key was pressed. Assuming 0x00 means no key press.
                if (readBuf[0] != 0x00)
                {                                                // If a key was pressed
                    inputChecker[enterPassCounter] = readBuf[0]; // Store the pressed key

                    lcd_stringout("*"); // Display an asterisk for each character entered
                    enterPassCounter++; // Move to the next character
                    _delay_ms(40);      // Add some delay to debounce the keypad input
                }
                displayCurrentTime();
                intruder_door = door_locked(locked, locked_vac);
                if (intruder_door)
                    break;
            }
            inputChecker[enterPassCounter] = '\0'; // Null-terminate the input buffer
            if (strcmp(inputChecker, passKey) == 0 && !locked)
            {
                lockedMessage();
                locked = true;      // Assuming 'locked' is a variable you use to track the lock state
                attemptCounter = 0; // Reset the attempt counter
                enterPassCounter = 0;
            }
            else if (strcmp(inputChecker, passKey) == 0 && locked)
            {
                unlocked();
                // Directly display the initial menu again without checking switch position
                initialMenuDisplayed = false; // Ensure the initial menu is displayed again
                displayInitialMenu();
                locked = false;     // Assuming 'locked' is a variable you use to track the lock state
                attemptCounter = 0; // Reset the attempt counter
                enterPassCounter = 0;
            }
            else if (strcmp(inputChecker, passKey) != 0 && intruder_door == false)
            {
                // Incorrect password
                errorMessage();
                attemptCounter++;
                if (attemptCounter > 2)
                {
                    intruder_password = true;
                }
            }
            // Reset the counter and buffer for the next password entry attempt
            enterPassCounter = 0;
            memset(inputChecker, 0, sizeof(inputChecker));
        }
        else if (switch2 && !intruder_door && !intruder_password)
        {
            while (enterPassCounter < 15) // Clear the keypad FIFO
            {
                writeBuf[0] = 0x06;
                writeBuf[1] = 0x01;
                status = i2c_io(keypadAdd, writeBuf, 2, NULL, 0);
                _delay_ms(10); // Small delay to allow the device to process the command
                writeBuf[0] = 0x03;
                status = i2c_io(keypadAdd, writeBuf, 1, readBuf, 1); // Send the read command
                enterPassCounter++;
            }
            displaySwitch2Message(); // Assume similar logic for switch2
            enterPassCounter = 0;
            while (enterPassCounter < 4)
            {
                lcd_moveto(1, 28 + enterPassCounter);
                // Write to register 6 first
                writeBuf[0] = 0x06;
                writeBuf[1] = 0x01;
                status = i2c_io(keypadAdd, writeBuf, 2, NULL, 0);
                _delay_ms(10); // Small delay to allow the device to process the command

                // Prepare to read the keypad state
                writeBuf[0] = 0x03;
                status = i2c_io(keypadAdd, writeBuf, 1, readBuf, 1); // Send the read command

                // Check if a key was pressed. Assuming 0x00 means no key press.
                if (readBuf[0] != 0x00)
                {                                                // If a key was pressed
                    inputChecker[enterPassCounter] = readBuf[0]; // Store the pressed key
                    lcd_stringout("*");                          // Display an asterisk for each character entered
                    enterPassCounter++;                          // Move to the next character
                    _delay_ms(100);                              // Add some delay to debounce the keypad input
                }
                displayCurrentTime();
                intruder_door = door_locked(locked, locked_vac);

                if (intruder_door)
                    break;
                led_locked(&led_start, locked_vac, intruder_door, intruder_password);
                led_locked2(&led_start2, locked_vac, intruder_door, intruder_password);
                soundTrigger(&led_start, locked_vac);
            }
            inputChecker[enterPassCounter] = '\0'; // Null-terminate the input buffer
            if (strcmp(inputChecker, passKey) == 0 && !locked_vac)
            {
                lockedMessage();
                locked_vac = true;  // Assuming 'locked' is a variable you use to track the lock state
                attemptCounter = 0; // Reset the attempt counter
                enterPassCounter = 0;
                led_start = seconds;
                led_locked(&led_start, locked_vac, intruder_door, intruder_password);
                led_locked2(&led_start2, locked_vac, intruder_door, intruder_password);
                soundTrigger(&led_start, locked_vac);
            }
            else if (strcmp(inputChecker, passKey) == 0 && locked_vac)
            {
                unlocked();
                // Directly display the initial menu again without checking switch position
                initialMenuDisplayed = false; // Ensure the initial menu is displayed again
                displayInitialMenu();
                locked_vac = false; // Assuming 'locked' is a variable you use to track the lock state
                attemptCounter = 0; // Reset the attempt counter
                enterPassCounter = 0;
            }
            else if (strcmp(inputChecker, passKey) != 0 && intruder_door == false)
            {
                // Incorrect password
                errorMessage();
                attemptCounter++;
                if (attemptCounter > 2)
                {
                    intruder_password = true;
                }
            }
            // Reset the counter and buffer for the next password entry attempt
            enterPassCounter = 0;
            memset(inputChecker, 0, sizeof(inputChecker));
        }
        _delay_ms(100); // Adjust for your needs, helps with switch debounce
    }
}