#include "../phase2/globals.h"

void getTOD_sst(pcb_t* sender);
void terminate_sst();
void writePrinter_sst(pcb_t* sender, unsigned int asid, sst_print_t* payload);
void writeTerminal_sst(pcb_t* sender, unsigned int asid, sst_print_t* payload);
