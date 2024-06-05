#include "../phase2/globals.h"

pcb_t* create_process(state_t *s, support_t *supp);
support_t* getSupportStruct();
unsigned int doIOSupportLevel(unsigned int command_address, unsigned int command_value);
unsigned int doIOFlash(unsigned int asid, unsigned int vpn, unsigned int command, unsigned int data0);
unsigned int doIOTerminal(unsigned int asid, unsigned int command, char* msg);
void int_to_string(int num, char* str);
