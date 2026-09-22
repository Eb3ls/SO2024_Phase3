#include "../phase2/globals.h"

pcb_t* create_process(state_t *s, support_t *supp);
support_t* getSupportStruct();
unsigned int doIOSupportLevel(unsigned int command_address, unsigned int command_value);
unsigned int doIOFlash(unsigned int asid, unsigned int vpn, unsigned int command, unsigned int data0);
void doIOPrinter(unsigned int asid, unsigned int command, char* msg, int length);
void doIOTerminal(unsigned int asid, unsigned int command, char* msg, int length);
