#define _GNU_SOURCE

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "phase3/sysSupport.h"

int process_count;
int softBlockCount;
struct list_head readyQueue;
struct list_head blockedPCBs[SEMDEVLEN - 1];
struct list_head waitingForClock;
passupvector_t* passupvector;
pcb_t* ssi_pcb;
pcb_t* test_pcb;
pcb_t* current_process;
pcb_t* sst_pcb[9];

static pcb_t fake_ssi;
static pcb_t fake_test;
static pcb_t fake_sst;
static int returned_parent_pid;
static int get_pid_pending;
static int get_pid_requests;
static int completion_messages;
static int ssi_termination_requests;
static int ssi_termination_waits;
static int sst_termination_requests;
static int sst_termination_waits;
static int ordering_failures;
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
    (void)arg3;

    if (number == (unsigned int)SENDMESSAGE &&
        arg1 == (unsigned int)(uintptr_t)ssi_pcb) {
        ssi_payload_t* payload = (ssi_payload_t*)(uintptr_t)arg2;
        if (payload->service_code == GETPROCESSID) {
            get_pid_pending = 1;
            get_pid_requests++;
        }
        else if (payload->service_code == TERMPROCESS) {
            if (completion_messages != 1) {
                ordering_failures++;
            }
            ssi_termination_requests++;
        }
        return 0;
    }

    if (number == (unsigned int)RECEIVEMESSAGE &&
        arg1 == (unsigned int)(uintptr_t)ssi_pcb) {
        if (get_pid_pending) {
            *(int*)(uintptr_t)arg2 = returned_parent_pid;
            get_pid_pending = 0;
        }
        else {
            ssi_termination_waits++;
        }
        return 0;
    }

    if (number == (unsigned int)SENDMESSAGE &&
        arg1 == (unsigned int)(uintptr_t)test_pcb) {
        if (ssi_termination_requests != 0) {
            ordering_failures++;
        }
        completion_messages++;
        return 0;
    }

    if (number == (unsigned int)SENDMESSAGE &&
        arg1 == (unsigned int)(uintptr_t)sst_pcb[3]) {
        ssi_payload_t* payload = (ssi_payload_t*)(uintptr_t)arg2;
        if (payload->service_code == TERMPROCESS) {
            sst_termination_requests++;
        }
        return 0;
    }

    if (number == (unsigned int)RECEIVEMESSAGE &&
        arg1 == (unsigned int)(uintptr_t)sst_pcb[3]) {
        sst_termination_waits++;
    }

    return 0;
}

static void reset_observations(void) {
    get_pid_pending = 0;
    get_pid_requests = 0;
    completion_messages = 0;
    ssi_termination_requests = 0;
    ssi_termination_waits = 0;
    sst_termination_requests = 0;
    sst_termination_waits = 0;
    ordering_failures = 0;
}

static void* run_tests(void* unused) {
    (void)unused;
    support_t support;
    memset(&support, 0, sizeof(support));
    support.sup_asid = 3;

    reset_observations();
    returned_parent_pid = test_pcb->p_pid;
    programTrapHandler(&support);
    CHECK(get_pid_requests == 1, "SST trap identifies its parent");
    CHECK(completion_messages == 1, "SST trap sends exactly one completion");
    CHECK(ssi_termination_requests == 1, "SST trap requests SSI teardown once");
    CHECK(ssi_termination_waits == 1, "SST trap waits for SSI teardown");
    CHECK(sst_termination_requests == 0, "SST trap does not message itself");
    CHECK(ordering_failures == 0, "SST completion precedes SSI teardown");

    reset_observations();
    returned_parent_pid = test_pcb->p_pid + 1;
    programTrapHandler(&support);
    CHECK(get_pid_requests == 1, "U-proc trap identifies its parent");
    CHECK(sst_termination_requests == 1, "U-proc delegates termination to its SST");
    CHECK(sst_termination_waits == 1, "U-proc waits for its SST");
    CHECK(completion_messages == 0, "U-proc does not impersonate SST completion");
    CHECK(ssi_termination_requests == 0, "U-proc does not bypass its SST");
    CHECK(ordering_failures == 0, "U-proc path has no misplaced completion");

    return NULL;
}

int main(void) {
    ssi_pcb = &fake_ssi;
    test_pcb = &fake_test;
    test_pcb->p_pid = 42;
    sst_pcb[3] = &fake_sst;

    size_t stack_size = 1024 * 1024;
    void* stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap low host stack");
        return 1;
    }

    pthread_attr_t attr;
    pthread_t thread;
    CHECK(pthread_attr_init(&attr) == 0, "initialize thread attributes");
    CHECK(pthread_attr_setstack(&attr, stack, stack_size) == 0,
          "install low host stack");
    CHECK(pthread_create(&thread, &attr, run_tests, NULL) == 0,
          "create abnormal completion test thread");
    CHECK(pthread_join(thread, NULL) == 0,
          "join abnormal completion test thread");
    pthread_attr_destroy(&attr);
    munmap(stack, stack_size);

    if (failures != 0) {
        return 1;
    }
    puts("abnormal completion tests passed");
    return 0;
}
