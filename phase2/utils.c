#include "globals.h"



void returnToFlow(){
    state_t* state = (state_t*) BIOSDATAPAGE;
    state->pc_epc += WORDLEN;
    state->status = state->status | IECON; // Non so se è giusto, anzi
    LDST(state);
}



void copyState(state_t* new_state, state_t* src_state){
    new_state->entry_hi = src_state->entry_hi;
    new_state->cause = src_state->cause;
    new_state->status = src_state->status;
    new_state->pc_epc = src_state->pc_epc;
    for (int i = 0; i < STATE_GPR_LEN; i++) {
        new_state->gpr[i] = src_state->gpr[i];
    }
    new_state->hi = src_state->hi;
    new_state->lo = src_state->lo;
}



void removeFromList(pcb_t* pcb, signed short int location){
    if (location == READYQUEUE_LOCATION) {
        outProcQ(&readyQueue, pcb);
    }
    else if (location == WAITINGCLOCK_LOCATION) {
        outProcQ(&waitingForClock, pcb);
    }
    else if (location >= 0 && location < SEMDEVLEN) {
        outProcQ(&blockedPCBs[location], pcb);
    }
    pcb->p_location = NOTLOCATED_LOCATION;
}



void insertInList(pcb_t* pcb, signed short int pcb_location){
    if (pcb_location == READYQUEUE_LOCATION) {
        insertProcQ(&readyQueue, pcb);
    }
    else if (pcb_location == WAITINGCLOCK_LOCATION) {
        insertProcQ(&waitingForClock, pcb);
    }
    else if (pcb_location >= 0 && pcb_location < SEMDEVLEN) {
        insertProcQ(&blockedPCBs[pcb_location], pcb);
    }
    pcb->p_location = pcb_location;
}
