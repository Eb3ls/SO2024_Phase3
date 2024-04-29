#include "sst.h"

void sst_entry_point() {
    SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);
}
