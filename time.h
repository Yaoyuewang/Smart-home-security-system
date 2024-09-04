#include <stdio.h>
#include <stdbool.h>
#include <stdint.h> 

#define keypadAdd 0x96
#define clockAdd 0xD0
#define CLOCK_SET_FLAG_ADDR 0x00

extern volatile int hours, minutes, seconds;
extern volatile bool timeToUpdate;

// Function prototypes
bool isClockSet(void);
void markClockAsSet(void);
void displayCurrentTime(void);
void setupTimer1(void);