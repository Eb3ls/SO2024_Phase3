#include "globals.h"
#include "scheduler.h"

void interruptHandler();
void nonTimerInterruptHandler(int line, int cause);
void pltInterruptHandler();
void pseudoClockInterruptHandler();

