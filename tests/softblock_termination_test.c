#include <stdio.h>

#include "phase2/SSI.h"

int process_count;
int softBlockCount;
struct list_head readyQueue;
struct list_head blockedPCBs[SEMDEVLEN - 1];
struct list_head waitingForClock;
passupvector_t* passupvector;
pcb_t* ssi_pcb;
pcb_t* test_pcb;
pcb_t* current_process;

static int failures;

#define CHECK(condition, message)                                                     \
    do {                                                                              \
        if (!(condition)) {                                                           \
            fprintf(stderr, "FAIL: %s (line %d)\n", (message), __LINE__);            \
            failures++;                                                              \
        }                                                                             \
    } while (0)

unsigned int SYSCALL(unsigned int number, unsigned int arg1,
                     unsigned int arg2, unsigned int arg3) {
    (void)number;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    return 0;
}

unsigned int LDST(void* state) {
    (void)state;
    return 0;
}

static void reset_queues(void) {
    mkEmptyProcQ(&readyQueue);
    mkEmptyProcQ(&waitingForClock);
    for (int i = 0; i < SEMDEVLEN - 1; i++) {
        mkEmptyProcQ(&blockedPCBs[i]);
    }
    process_count = 0;
    softBlockCount = 0;
}

static pcb_t* new_process_at(signed short int location) {
    pcb_t* process = allocPcb();
    CHECK(process != NULL, "allocate PCB");
    if (process == NULL) {
        return NULL;
    }
    insertInList(process, location);
    process_count++;
    return process;
}

static void test_clock_blocked(void) {
    reset_queues();
    pcb_t* process = new_process_at(WAITINGCLOCK_LOCATION);
    softBlockCount = 1;

    terminateProcess(process, NULL);

    CHECK(softBlockCount == 0, "clock-blocked termination balances softBlockCount");
    CHECK(emptyProcQ(&waitingForClock), "clock-blocked PCB is removed");
    CHECK(process_count == 0, "clock-blocked PCB is freed");
}

static void test_device_blocked(void) {
    const int device = SEMDEVLEN - 2;
    reset_queues();
    pcb_t* process = new_process_at(device);
    softBlockCount = 1;

    terminateProcess(process, NULL);

    CHECK(softBlockCount == 0, "device-blocked termination balances softBlockCount");
    CHECK(emptyProcQ(&blockedPCBs[device]), "device-blocked PCB is removed");
    CHECK(process_count == 0, "device-blocked PCB is freed");
}

static void test_nonblocked(void) {
    reset_queues();
    pcb_t* process = new_process_at(READYQUEUE_LOCATION);

    terminateProcess(process, NULL);

    CHECK(softBlockCount == 0, "ready PCB does not change softBlockCount");
    CHECK(emptyProcQ(&readyQueue), "ready PCB is removed");
    CHECK(process_count == 0, "ready PCB is freed");
}

static void test_message_blocked(void) {
    reset_queues();
    pcb_t* process = new_process_at(WAITINGRECV_LOCATION);

    terminateProcess(process, NULL);

    CHECK(softBlockCount == 0, "message-blocked PCB does not change softBlockCount");
    CHECK(process_count == 0, "message-blocked PCB is freed");
}

static void test_recursive_tree(void) {
    const int device = 31;
    reset_queues();
    pcb_t* parent = new_process_at(READYQUEUE_LOCATION);
    pcb_t* clock_child = new_process_at(WAITINGCLOCK_LOCATION);
    pcb_t* device_grandchild = new_process_at(device);
    insertChild(parent, clock_child);
    insertChild(clock_child, device_grandchild);
    softBlockCount = 2;

    terminateProcess(parent, NULL);

    CHECK(softBlockCount == 0,
          "recursive termination balances each blocked descendant once");
    CHECK(emptyProcQ(&readyQueue), "recursive parent is removed");
    CHECK(emptyProcQ(&waitingForClock), "recursive clock child is removed");
    CHECK(emptyProcQ(&blockedPCBs[device]), "recursive device child is removed");
    CHECK(process_count == 0, "recursive process tree is freed");
}

int main(void) {
    initPcbs();
    test_clock_blocked();
    test_device_blocked();
    test_nonblocked();
    test_message_blocked();
    test_recursive_tree();

    if (failures != 0) {
        return 1;
    }
    puts("soft-block termination tests passed");
    return 0;
}
