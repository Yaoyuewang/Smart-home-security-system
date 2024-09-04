#include <stdbool.h> 

// Declaration of global variables if they are used outside this module
extern bool initialMenuDisplayed;
extern bool messageDisplayed;


// Function prototypes for the LCD messages
void displayInitialMenu(void);
void displaySwitch1Message(void);
void displaySwitch2Message(void);
void unlocked(void);
void lockedMessage(void);
void errorMessage(void);
void intruderMessage(void);