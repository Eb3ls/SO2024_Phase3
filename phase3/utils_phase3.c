#include "utils_phase3.h"

pcb_t* create_process(state_t* s, support_t* supp) {
    pcb_t* p;
    ssi_create_process_t ssi_create_process = {
        .state = s,
        .support = supp,
    };
    ssi_payload_t payload = {
        .service_code = CREATEPROCESS,
        .arg = &ssi_create_process,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p), 0);
    return p;
}

support_t* getSupportStruct() {
    support_t* support_struct;
    ssi_payload_t getsup_payload = {
        .service_code = GETSUPPORTPTR,
        .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&support_struct), 0);
    return support_struct;
}

unsigned int doIOtoFlash(unsigned int command_address, unsigned int command_value) {
    unsigned int status;
    ssi_do_io_t do_io = {
        .commandAddr = (memaddr*)command_address,
        .commandValue = command_value,
    };
    ssi_payload_t payload = {
        .service_code = DOIO,
        .arg = &do_io,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
    return status;
}

void int_to_string(int num, char* str) {
    int i = 0;
    int isNegative = 0;

    /* Handle 0 explicitly, otherwise empty string is printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // In standard itoa(), negative numbers are handled only with base 10.
    // Otherwise numbers are considered unsigned.
    if (num < 0) {
        isNegative = 1;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % 10;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / 10;
    }

    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void printToTerm(char* msg) {
    unsigned int* base = (unsigned int*)(0x10000254);
    unsigned int* command = base + 3;
    unsigned int status;

    while (*msg != EOS) {
        unsigned int value = PRINTCHR | (((unsigned int)*msg) << 8);
        ssi_do_io_t do_io = {
            .commandAddr = command,
            .commandValue = value,
        };
        ssi_payload_t payload = {
            .service_code = DOIO,
            .arg = &do_io,
        };
        SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
        SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
        msg++;
    }
}
