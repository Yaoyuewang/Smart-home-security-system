#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#include "keypadSelf.h"

#define keypadAdd 0x96

/* Address where we store test data in the EEPROM */
#define DATA_ADDR 500
// Find divisors for the UART0 and I2C baud rates

uint8_t wrprom(uint8_t *p, uint16_t n, uint16_t a)
{
    uint16_t maxw; // Maximum bytes to write in the page
    uint8_t status;
    uint8_t adata[66]; // Array to hold the address and data

    while (n > 0)
    {
        adata[0] = a >> 8;   // Put EEPROM address in adata buffer,
        adata[1] = a & 0xff; // MSB first, LSB second
        // We can write up to the next 64 byte boundary,
        // but no more than is left to write
        maxw = 64 - (a % 64); // Max for this page
        if (n < maxw)
            maxw = n; // Number left to write in page

        memcpy(adata + 2, p, maxw); // Put data in buffer after address

        status = i2c_io(keypadAdd, 0x01, 2, NULL, 0);
        if (status != 0)
            return (status);
        _delay_ms(5); // Wait 5ms for EEPROM to write
        p += maxw;    // Increment array address
        a += maxw;    // Increment address
        n -= maxw;    // Decrement byte count
    }
    return (0);
}
uint8_t rdprom(uint8_t *p, uint16_t n, uint16_t a)
{
    uint8_t status;
    uint8_t adata[2]; // Array to hold the address

    adata[0] = a >> 8;   // Put EEPROM address in adata buffer,
    adata[1] = a & 0xff; // MSB first, LSB second

    status = i2c_io(keypadAdd, adata, 2, p, n);
    return (status);
}

uint8_t i2c_io(uint8_t device_addr,
               uint8_t *wp, uint16_t wn, uint8_t *rp, uint16_t rn)
{
    uint8_t status, send_stop, wrote, start_stat;

    status = 0;
    wrote = 0;
    send_stop = 0;

    if (wn > 0)
    {
        wrote = 1;
        send_stop = 1;

        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA); // Send start condition
        while (!(TWCR & (1 << TWINT)))
            ; // Wait for TWINT to be set
        status = TWSR & 0xf8;
        if (status != 0x08) // Check that START was sent OK
            return (status);

        TWDR = device_addr & 0xfe;         // Load device address and R/W = 0;
        TWCR = (1 << TWINT) | (1 << TWEN); // Start transmission
        while (!(TWCR & (1 << TWINT)))
            ; // Wait for TWINT to be set
        status = TWSR & 0xf8;
        if (status != 0x18)
        {                       // Check that SLA+W was sent OK
            if (status == 0x20) // Check for NAK
                goto nakstop;   // Send STOP condition
            return (status);    // Otherwise just return the status
        }

        // Write "wn" data bytes to the slave device
        while (wn-- > 0)
        {
            TWDR = *wp++;                      // Put next data byte in TWDR
            TWCR = (1 << TWINT) | (1 << TWEN); // Start transmission
            while (!(TWCR & (1 << TWINT)))
                ; // Wait for TWINT to be set
            status = TWSR & 0xf8;
            if (status != 0x28)
            {                       // Check that data was sent OK
                if (status == 0x30) // Check for NAK
                    goto nakstop;   // Send STOP condition
                return (status);    // Otherwise just return the status
            }
        }

        status = 0; // Set status value to successful
    }

    if (rn > 0)
    {
        send_stop = 1;

        // Set the status value to check for depending on whether this is a
        // START or repeated START
        start_stat = (wrote) ? 0x10 : 0x08;

        // Put TWI into Master Receive mode by sending a START, which could
        // be a repeated START condition if we just finished writing.
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
        // Send start (or repeated start) condition
        while (!(TWCR & (1 << TWINT)))
            ; // Wait for TWINT to be set
        status = TWSR & 0xf8;
        if (status != start_stat) // Check that START or repeated START sent OK
            return (status);

        TWDR = device_addr | 0x01;         // Load device address and R/W = 1;
        TWCR = (1 << TWINT) | (1 << TWEN); // Send address+r
        while (!(TWCR & (1 << TWINT)))
            ; // Wait for TWINT to be set
        status = TWSR & 0xf8;
        if (status != 0x40)
        {                       // Check that SLA+R was sent OK
            if (status == 0x48) // Check for NAK
                goto nakstop;
            return (status);
        }

        // Read all but the last of n bytes from the slave device in this loop
        rn--;
        while (rn-- > 0)
        {
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA); // Read byte and send ACK
            while (!(TWCR & (1 << TWINT)))
                ; // Wait for TWINT to be set
            status = TWSR & 0xf8;
            if (status != 0x50) // Check that data received OK
                return (status);
            *rp++ = TWDR; // Read the data
        }

        // Read the last byte
        TWCR = (1 << TWINT) | (1 << TWEN); // Read last byte with NOT ACK sent
        while (!(TWCR & (1 << TWINT)))
            ; // Wait for TWINT to be set
        status = TWSR & 0xf8;
        if (status != 0x58) // Check that data received OK
            return (status);
        *rp++ = TWDR; // Read the data

        status = 0; // Set status value to successful
    }

nakstop: // Come here to send STOP after a NAK
    if (send_stop)
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO); // Send STOP condition

    return (status);
}

void i2c_init(uint8_t bdiv)
{
    TWSR = 0;    // Set prescalar for 1
    TWBR = bdiv; // Set bit rate register
}

void sci_init(uint8_t ubrr)
{
    UBRR0 = ubrr;           // Set baud rate
    UCSR0B |= (1 << TXEN0); // Turn on transmitter
    UCSR0C = (3 << UCSZ00); // Set for asynchronous operation, no parity,
                            // one stop bit, 8 data bits
}

void sci_out(char ch)
{
    while ((UCSR0A & (1 << UDRE0)) == 0)
        ;
    UDR0 = ch;
}

void sci_outs(char *s)
{
    char ch;

    while ((ch = *s++) != (char)'\0')
        sci_out(ch);
}