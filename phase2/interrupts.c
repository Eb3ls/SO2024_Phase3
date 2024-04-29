#include "interrupts.h"

void interruptHandler(){
    unsigned int cause = getCAUSE();

    if((cause & LOCALTIMERINT) == LOCALTIMERINT){
        pltInterruptHandler();
    }
    else if((cause & TIMERINTERRUPT) == TIMERINTERRUPT){
        pseudoClockInterruptHandler();
    }
    else if((cause & DISKINTERRUPT) == DISKINTERRUPT){
        nonTimerInterruptHandler(3, cause);
    }
    else if((cause & FLASHINTERRUPT) == FLASHINTERRUPT){
        nonTimerInterruptHandler(4, cause);
    }
    else if((cause & PRINTINTERRUPT) == PRINTINTERRUPT){
        nonTimerInterruptHandler(6, cause);
    }
    else if((cause & TERMINTERRUPT) == TERMINTERRUPT){
        nonTimerInterruptHandler(7, cause);
    }
}

void nonTimerInterruptHandler(int line, int cause){
    int devNo;

    unsigned int word = *(unsigned int*) (0x10000040 + (line - 3) * 0x04);

    if((word & DEV0ON) == DEV0ON){
        devNo = 0;
    }
    else if((word & DEV1ON) == DEV1ON){
        devNo = 1;
    }
    else if((word & DEV2ON) == DEV2ON){
        devNo = 2;
    }
    else if ((word & DEV3ON) == DEV3ON){
        devNo = 3;
    }
    else if((word & DEV4ON) == DEV4ON){
        devNo = 4;
    }
    else if((word & DEV5ON) == DEV5ON){
        devNo = 5;
    }
    else if((word & DEV6ON) == DEV6ON){
        devNo = 6;
    }
    else if ((word & DEV7ON) == DEV7ON){
        devNo = 7;
    }
    else{
        return;
    }
    
    unsigned int devAddrValue = 0x10000054 + ((line - 3) * 0x80) + (devNo * 0x10);

    unsigned int device_status;
    if(line == 7){
        termreg_t* devAddrBase = (termreg_t*) devAddrValue;
        device_status = devAddrBase->transm_status;
        devAddrBase->transm_command = ACK;
    }
    else{
        dtpreg_t* devAddrBase = (dtpreg_t*) devAddrValue; 
        device_status = devAddrBase->status;
        devAddrBase->command = ACK;
    }
    
    msg_t* msg = allocMsg();
    msg->m_sender = ssi_pcb;
    msg->m_payload = device_status;

    int index = 8 * (line - 3) + devNo;  

    pcb_t* unblockedPcb = removeProcQ(&blockedPCBs[index]);
    if(unblockedPcb != NULL){
        // If the PCB has been removed from the blocked process queue
        // then we insert it into the ready process queue
        insertInList(unblockedPcb, READYQUEUE_LOCATION);
        pushMessage(&unblockedPcb->msg_inbox, msg);
        softBlockCount--;
        unblockedPcb->p_s.reg_v0 = device_status;
    }

    if(current_process != NULL){
        state_t* oldState = (state_t*) BIOSDATAPAGE;
        LDST(oldState);
    }
    else{
        scheduling();
    }
}

void pltInterruptHandler(){
    current_process->p_time = current_process->p_time + (TIMESLICE - getTIMER());
    setTIMER(TIMESLICE);
    copyState(&current_process->p_s, (state_t*) BIOSDATAPAGE);
    insertInList(current_process, READYQUEUE_LOCATION);
    scheduling();
}

void pseudoClockInterruptHandler(){
    LDIT(PSECOND);
    while(!emptyProcQ(&waitingForClock)){
        pcb_t* unblockedPcb = removeProcQ(&waitingForClock);
        msg_t* msg = allocMsg();
        msg->m_sender = ssi_pcb;
        msg->m_payload = 0;
        insertInList(unblockedPcb, READYQUEUE_LOCATION);
        pushMessage(&unblockedPcb->msg_inbox, msg);
        softBlockCount--;
    }

    if(current_process != NULL){
        state_t* oldState = (state_t*) BIOSDATAPAGE;
        LDST(oldState);
    }
    else{
        scheduling();
    }
}
