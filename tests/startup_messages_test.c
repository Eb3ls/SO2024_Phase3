#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "phase3/vmSupport.h"

extern void test(void);

int process_count;
int softBlockCount;
struct list_head readyQueue;
struct list_head blockedPCBs[SEMDEVLEN - 1];
struct list_head waitingForClock;
passupvector_t* passupvector;
pcb_t* ssi_pcb;
pcb_t* test_pcb;
pcb_t* current_process;

static pcb_t fake_ssi;
static pcb_t fake_test;
static pcb_t created[9];
static int create_count;
static int completion_receives;
static int termination_sends;
static int ordering_failure;
static int mode;
static unsigned int first_syscall;
static unsigned int first_arg1;
static jmp_buf mutex_started;

enum {
    MODE_MUTEX,
    MODE_TEST,
};

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
    (void)arg2;
    (void)arg3;

    if (mode == MODE_MUTEX) {
        first_syscall = number;
        first_arg1 = arg1;
        longjmp(mutex_started, 1);
    }

    if (number == (unsigned int)RECEIVEMESSAGE && arg1 == ANYMESSAGE) {
        completion_receives++;
    }
    if (number == (unsigned int)SENDMESSAGE &&
        arg1 == (unsigned int)(uintptr_t)ssi_pcb) {
        termination_sends++;
        if (completion_receives != UPROCMAX) {
            ordering_failure = 1;
        }
    }
    return 0;
}

unsigned int STST(void* state) {
    memset(state, 0, sizeof(state_t));
    return 0;
}

pcb_t* create_process(state_t* state, support_t* support) {
    (void)state;
    (void)support;
    if (create_count >= (int)(sizeof(created) / sizeof(created[0]))) {
        return NULL;
    }
    return &created[create_count++];
}

void pageFaultHandler(void) {}
void generalExceptionHandler(void) {}
void sst_entry(void) {}

int main(void) {
    ssi_pcb = &fake_ssi;
    test_pcb = &fake_test;

    mode = MODE_MUTEX;
    if (setjmp(mutex_started) == 0) {
        swapMutex_entry_point();
    }
    CHECK(first_syscall == (unsigned int)RECEIVEMESSAGE,
          "swap mutex waits for a lock request before sending");
    CHECK(first_arg1 == ANYMESSAGE, "swap mutex initially accepts any requester");

    mode = MODE_TEST;
    test();
    CHECK(create_count == UPROCMAX + 1, "test creates one mutex and eight SSTs");
    CHECK(completion_receives == UPROCMAX,
          "test waits for all eight U-proc completion messages");
    CHECK(termination_sends == 1, "test requests its own termination once");
    CHECK(!ordering_failure, "termination follows all eight completions");

    if (failures != 0) {
        return 1;
    }
    puts("startup message tests passed");
    return 0;
}
