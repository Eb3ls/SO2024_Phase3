#include "globals.h"
#include "scheduler.h"
#include "interrupts.h"
#include "syscall.h"
#include "SSI.h"

extern void test();
extern void SSI_function_entry_point();
extern void terminateProcess(pcb_t* pcb, pcb_t* arg);

// Declare global variables
int process_count;
int softBlockCount;
struct list_head readyQueue;
struct list_head blockedPCBs[SEMDEVLEN - 1];
passupvector_t* passupvector;
pcb_t* ssi_pcb;
pcb_t* test_pcb;
pcb_t* current_process;
struct list_head waitingForClock;

void uTLB_RefillHandler() {
    // Get the state of the current process
    state_t* state = (state_t*) BIOSDATAPAGE;

    // Get the entryHi register
    unsigned int entryHi = state->entry_hi;
    
    // Get VPN
    unsigned int vpn_number = ((entryHi & GETPAGENO) >> VPNSHIFT);

    if (vpn_number > 31){
        vpn_number = 31;
    }

    pteEntry_t* entry = &(current_process->p_supportStruct->sup_privatePgTbl[vpn_number]);
    // Non sappiamo perchè ma il campo entryHi, quando la richiesta viene fatta da un SST
    // non contiene l'ASID, quindi dobbiamo settarlo manualmente
    // setENTRYHI(entry->pte_entryHI);
    setENTRYHI(entryHi);
    setENTRYLO(entry->pte_entryLO);
    TLBWR();
    LDST(state);
}

void passUpOrDie(unsigned int relatedConst) {
    if (current_process->p_supportStruct == NULL) {
        terminateProcess(current_process, NULL);
    }
    else {
        copyState(&current_process->p_supportStruct->sup_exceptState[relatedConst], (state_t*) BIOSDATAPAGE);
        LDCXT(current_process->p_supportStruct->sup_exceptContext[relatedConst].stackPtr, current_process->p_supportStruct->sup_exceptContext[relatedConst].status, current_process->p_supportStruct->sup_exceptContext[relatedConst].pc);
    }
    scheduling();
}

void exceptionHandler(){
    state_t* state = (state_t*) BIOSDATAPAGE;
    unsigned int cause = getCAUSE();
    cause = (cause & GETEXECCODE) >> CAUSESHIFT;
    if(cause == IOINTERRUPTS) {
        interruptHandler(); 
    }
    else if(cause >= 1 && cause <= 3) {
        passUpOrDie(PGFAULTEXCEPT);
    }
    else if(cause == SYSEXCEPTION) {
        if((int)state->reg_a0 >= 1){
            passUpOrDie(GENERALEXCEPT);
        }
        else{
            SYSCALLHandler();
        }
    }
    else if(cause >= 4 && cause <= 12) {
        passUpOrDie(GENERALEXCEPT);
    }
    else {
        PANIC();
    }
}


int main(void) {

    passupvector = (passupvector_t*) PASSUPVECTOR;

    passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    passupvector->tlb_refill_stackPtr = KERNELSTACK;

    passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector->exception_stackPtr = KERNELSTACK;

    initPcbs();
    initMsgs();

    // Initialize variables
    process_count = 0;
    softBlockCount = 0;
    mkEmptyProcQ(&readyQueue);
    current_process = NULL;
    mkEmptyProcQ(&waitingForClock);

    // Initialize the blockedPCBs array
    for (int i = 0; i < SEMDEVLEN - 1; i++) {
        mkEmptyProcQ(&blockedPCBs[i]);
    }

    // Initialize the interval timer
    LDIT(PSECOND);

    // Allocate the first process
    ssi_pcb = allocPcb();
    insertInList(ssi_pcb, READYQUEUE_LOCATION);
    process_count++;
    // Set stack pointer of the process
    RAMTOP(ssi_pcb->p_s.reg_sp);
    // Set the PC
    ssi_pcb->p_s.pc_epc = (memaddr)SSI_function_entry_point;
    // For technical reasons we have to set the same value in the t9 register
    ssi_pcb->p_s.reg_t9 = (memaddr)SSI_function_entry_point;
    // Set the status register
    // Kernel mode enabled, interrupts enabled
    ssi_pcb->p_s.status = ALLOFF | IEPON | IMON; // Dovrebbero essere corretti (spero)




    // Allocate the second process
    test_pcb = allocPcb();
    insertInList(test_pcb, READYQUEUE_LOCATION);
    process_count++;
    // Set stack pointer of the process to RAMTOP - (2 * PAGESIZE)
    RAMTOP(test_pcb->p_s.reg_sp);
    test_pcb->p_s.reg_sp -= (2 * PAGESIZE);
    // Set the PC
    test_pcb->p_s.pc_epc = (memaddr)test;
    // For technical reasons we have to set the same value in the t9 register
    test_pcb->p_s.reg_t9 = (memaddr)test;
    // Set the status register
    // Kernel mode enabled, interrupts enabled, local timer enabled
    test_pcb->p_s.status = ALLOFF | IEPON | IMON | TEBITON;

    scheduling();

    return 0;
}
