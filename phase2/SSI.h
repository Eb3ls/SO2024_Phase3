#include "globals.h"
#include "scheduler.h"

void SSI_function_entry_point();
void createProcess(struct pcb_t* sender, ssi_create_process_t* arg);
void terminateProcess(struct pcb_t* sender, struct pcb_t* arg);
void doIO(struct pcb_t* sender, ssi_do_io_t* arg);
void getTime(struct pcb_t* sender);
void waitForClock(struct pcb_t* sender);
void getSupportData(struct pcb_t* sender);
void getProcessId(struct pcb_t* sender, void* arg);
