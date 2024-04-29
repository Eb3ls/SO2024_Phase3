#include "syscall.h"

extern void passUpOrDie(unsigned int relatedConst);
extern void scheduling();

extern struct list_head pcbFree_h;

void SYS1(state_t* p_state){
    pcb_t* dest_pcb = (pcb_t*)p_state->reg_a1;
    unsigned int payload = p_state->reg_a2;

    // Check if the destination process exists
    if (dest_pcb->p_location == NOTLOCATED_LOCATION || dest_pcb == NULL){
        // If the process does not exist, return DEST_NOT_EXIST
        p_state->reg_v0 = DEST_NOT_EXIST;
        return;
    }

    // If the destination process exists, we can create the message
    msg_t* msg = allocMsg();
    if (msg == NULL){
        // If we cannot allocate the message, return MSGNOGOOD
        p_state->reg_v0 = MSGNOGOOD;
        return;
    }
    msg->m_sender = current_process;
    msg->m_payload = payload;
    
    // Insert the message in the destination process inbox
    insertMessage(&dest_pcb->msg_inbox, msg);

    // If the destination process was waiting for a message from us (or from anyone)
    // we have to insert it in the readyQueue
    int isWaitingForAnyone = (dest_pcb->p_s.reg_a1 == ANYMESSAGE);
    int isWaitingForUs = (dest_pcb->p_s.reg_a1 == (unsigned int)current_process);

    if ((isWaitingForAnyone || isWaitingForUs) && dest_pcb->p_location == WAITINGRECV_LOCATION) {
        insertInList(dest_pcb, READYQUEUE_LOCATION);
        dest_pcb->p_s.reg_v0 = (unsigned int)current_process;
    }

    // Return 0 to the sender to indicate that the message was sent successfully
    p_state->reg_v0 = 0;
}



void SYS2(state_t* p_state){
    pcb_t* sender = (pcb_t*)p_state->reg_a1;
    unsigned int payload = p_state->reg_a2;

    if (sender == ANYMESSAGE) {
        sender = NULL;
    }

    msg_t* msg = popMessage(&current_process->msg_inbox, sender);

    if (msg == NULL){
        // If there are no messages in the inbox, we have to wait for one
        // We have to save the state of the process and insert it in the waitingRecvQueue
        copyState(&current_process->p_s, (state_t*) BIOSDATAPAGE);

        // Update the process time
        current_process->p_time = current_process->p_time + (TIMESLICE - getTIMER());

        current_process->p_location = WAITINGRECV_LOCATION;

        scheduling();
    }
    else{
        // If there is a message in the inbox
        if (payload != 0){
            // If the payload is not empty, we have to copy the payload in the payload pointer
            *(unsigned int*)payload = msg->m_payload;
        }
        p_state->reg_v0 = (unsigned int) msg->m_sender;
        freeMsg(msg);
    }
}



void SYSCALLHandler(){
    state_t* state = (state_t*) BIOSDATAPAGE;
    if((state->status & USERPON) == USERPON){
        // If the caller is a user process, it cannot make a SYSCALL
        // Set the Cause.excCode register to RI
        // Correction from tutors
        state->cause = state->cause & CLEAREXECCODE;
        state->cause = state->cause | (PRIVINSTR << CAUSESHIFT);
        passUpOrDie(GENERALEXCEPT);
    }
    else{
        // If the caller is a kernel process, we can execute the SYSCALL
        unsigned int action = state->reg_a0;
        if(action == SENDMESSAGE){
            SYS1(state);
        }
        else if(action == RECEIVEMESSAGE){
            SYS2(state);
        }
        returnToFlow();
    }
}
