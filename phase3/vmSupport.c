#include "vmSupport.h"

swap_t swapTable[2 * UPROCMAX];

void initSwapStruct() {
    for (int i = 0; i < 2 * UPROCMAX; i++) {
        swapTable[i].sw_asid = -1;
        swapTable[i].sw_pageNo = -1;
    }
}

void swapMutex_entry_point() {
    SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);
}
