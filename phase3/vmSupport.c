#include "vmSupport.h"

unsigned int swap_pool_address_base = RAMSTART + (32 * PAGESIZE);
swap_t swapTable[2 * UPROCMAX];

void initSwapStruct() {
    for (int i = 0; i < 2 * UPROCMAX; i++) {
        swapTable[i].sw_asid = -1;
        swapTable[i].sw_pageNo = -1;
        swapTable[i].sw_pte = NULL;
    }
}

void swapMutex_entry_point() {
    SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
    pcb_t* sender;
    while (1) {
        sender = (pcb_t*) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
        SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
        SYSCALL(RECEIVEMESSAGE, (unsigned int) sender, 0, 0);
    }
}
