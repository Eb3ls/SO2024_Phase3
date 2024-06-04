#include "../phase2/globals.h"

pcb_t* create_process(state_t *s, support_t *supp);
support_t* getSupportStruct();
unsigned int doIOtoFlash(unsigned int command_address, unsigned int command_value);
void int_to_string(int num, char* str);
void printToTerm(char* msg);