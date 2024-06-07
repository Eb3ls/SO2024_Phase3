#include "../phase2/globals.h"

void TLBInvalidHandler(support_t* support_struct);
void pageFaultHandler();

void SYSCALLExceptionHandler(support_t* support_struct);
void programTrapHandler(support_t* support_struct);
void generalExceptionHandler();
