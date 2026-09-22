#define _GNU_SOURCE

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "phase3/sst_utils.h"
#include "phase3/utils_phase3.h"

int process_count;
int softBlockCount;
struct list_head readyQueue;
struct list_head blockedPCBs[SEMDEVLEN - 1];
struct list_head waitingForClock;
passupvector_t* passupvector;
pcb_t* ssi_pcb = (pcb_t*)(uintptr_t)0x2000;
pcb_t* test_pcb;
pcb_t* current_process;

enum capture_device {
    CAPTURE_PRINTER,
    CAPTURE_TERMINAL,
};

static enum capture_device capture_device;
static unsigned char captured[32];
static size_t captured_count;
static int completion_count;
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
        arg1 == (unsigned int)(uintptr_t)ssi_pcb && arg2 != 0) {
        ssi_payload_t* payload = (ssi_payload_t*)(uintptr_t)arg2;
        ssi_do_io_t* request = (ssi_do_io_t*)payload->arg;
        unsigned int value;

        if (capture_device == CAPTURE_PRINTER) {
            unsigned int data_address = START_DEVREG + ((6 - 3) * 0x80) + 0x8;
            value = *(volatile unsigned int*)(uintptr_t)data_address;
        }
        else {
            value = request->commandValue >> 8;
        }

        if (captured_count < sizeof(captured)) {
            captured[captured_count++] = (unsigned char)value;
        }
        return 0;
    }

    if (number == (unsigned int)RECEIVEMESSAGE &&
        arg1 == (unsigned int)(uintptr_t)ssi_pcb) {
        *(unsigned int*)(uintptr_t)arg2 = 1;
        return 0;
    }

    if (number == (unsigned int)SENDMESSAGE) {
        completion_count++;
    }

    return 0;
}

static void expect_bytes(const char* name, enum capture_device device, int via_sst,
                         char* input, int length, const unsigned char* expected,
                         size_t expected_count) {
    pcb_t* sender = (pcb_t*)(uintptr_t)0x3000;

    capture_device = device;
    captured_count = 0;
    completion_count = 0;
    memset(captured, 0xFF, sizeof(captured));

    if (via_sst) {
        sst_print_t payload = {
            .length = length,
            .string = input,
        };
        if (device == CAPTURE_PRINTER) {
            writePrinter_sst(sender, 1, &payload);
        }
        else {
            writeTerminal_sst(sender, 1, &payload);
        }
    }
    else if (device == CAPTURE_PRINTER) {
        doIOPrinter(1, PRINTCHR, input, length);
    }
    else {
        doIOTerminal(1, TRANSMITCHAR, input, length);
    }

    if (captured_count != expected_count ||
        memcmp(captured, expected, expected_count) != 0) {
        fprintf(stderr, "FAIL: %s emitted %zu bytes; expected %zu\n",
                name, captured_count, expected_count);
        for (size_t i = 0; i < captured_count; i++) {
            fprintf(stderr, "  byte %zu: 0x%02x\n", i, captured[i]);
        }
        failures++;
    }
    CHECK(completion_count == via_sst, "SST sends exactly one completion response");
}

static void* run_tests(void* unused) {
    (void)unused;

    char bounded[] = {'A', 'B', 'C', 'X'};
    const unsigned char abc[] = {'A', 'B', 'C'};
    expect_bytes("bounded non-terminated printer buffer", CAPTURE_PRINTER, 0,
                 bounded, 3, abc, sizeof(abc));

    char embedded_nul[] = {'Q', '\0', 'R'};
    const unsigned char q_nul_r[] = {'Q', '\0', 'R'};
    expect_bytes("embedded NUL printer payload", CAPTURE_PRINTER, 1,
                 embedded_nul, 3, q_nul_r, sizeof(q_nul_r));

    char early_nul[] = {'1', '2', '\0', '3', '4'};
    const unsigned char first_four[] = {'1', '2', '\0', '3'};
    expect_bytes("declared terminal length", CAPTURE_TERMINAL, 1,
                 early_nul, 4, first_four, sizeof(first_four));

    const unsigned char none[] = {0};
    expect_bytes("zero terminal length", CAPTURE_TERMINAL, 0,
                 bounded, 0, none, 0);
    expect_bytes("negative printer length", CAPTURE_PRINTER, 1,
                 bounded, -7, none, 0);

    return NULL;
}

int main(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    uintptr_t register_page = START_DEVREG & ~((uintptr_t)page_size - 1);
    void* registers = mmap((void*)register_page, (size_t)page_size,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                           -1, 0);
    if (registers == MAP_FAILED) {
        perror("mmap device register page");
        return 1;
    }

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
          "create print test thread");
    CHECK(pthread_join(thread, NULL) == 0, "join print test thread");
    pthread_attr_destroy(&attr);

    munmap(stack, stack_size);
    munmap(registers, (size_t)page_size);

    if (failures != 0) {
        return 1;
    }
    puts("print length tests passed");
    return 0;
}
