#include "../headers/const.h"
#include "../headers/types.h"
#include "../headers/listx.h"

#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"

#include "/usr/include/umps3/umps/libumps.h"

// Global variables
extern int process_count;
extern int softBlockCount;
extern struct list_head readyQueue;
extern struct list_head blockedPCBs[SEMDEVLEN - 1];
extern passupvector_t* passupvector;
extern pcb_t* ssi_pcb;
extern pcb_t* test_pcb;
extern pcb_t* current_process;
extern struct list_head waitingForClock;

// Utils functions
extern void returnToFlow();
extern void copyState(state_t* new_state, state_t* src_state);
extern void removeFromList(pcb_t* pcb, signed short int location);
extern void insertInList(pcb_t* pcb, signed short int pcb_location);
