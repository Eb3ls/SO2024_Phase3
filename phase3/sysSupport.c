#include "sysSupport.h"
#include "utils_phase3.h"

extern pcb_t* swap_mutex_pcb;
static int roundRobinPick = 0;
extern swap_t swapTable[2 * UPROCMAX];
extern unsigned int swap_pool_address_base;

void TLBInvalidHandler(support_t* support_struct){
    unsigned int entryHi = support_struct->sup_exceptState[0].entry_hi;

    SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);

    unsigned int vpn = ((entryHi & GETPAGENO) >> VPNSHIFT);

    if (vpn > 31){
        vpn = 31;
    }

    unsigned int asid = support_struct->sup_asid;

    // If the entry is occupied and dirty, swap it out
    if (swapTable[roundRobinPick].sw_asid != -1 && (((swapTable[roundRobinPick].sw_pte)->pte_entryLO & DIRTYON) == DIRTYON)){
        unsigned int current_processor_status = ((state_t*) BIOSDATAPAGE)->status;
        setSTATUS(ALLOFF);

        swapTable[roundRobinPick].sw_pte->pte_entryLO = swapTable[roundRobinPick].sw_pte->pte_entryLO & (!VALIDON);
        TLBCLR();

        setSTATUS(current_processor_status);

        // Bisogna scrivere la pagina in memoria secondaria
        // Bisogna capire bene come sostituire correttamente la pagina
        // Verosimilmente bisogna prendere l'indirizzo del device, scrivere in DATA0 il dato
        // da scrivere e poi inviare con l'SSI il comando per scrivere il dato
    }

    // Se siamo arrivati qui significa che quella entry è libera, quindi possiamo 
    // caricare quello che ci pare
    // Leggiamo dal device la pagina da caricare

    // Troviamo l'indirizzo della pagina da rimuovere dalla memoria
    unsigned int return_position = swap_pool_address_base + (roundRobinPick * PAGESIZE);

    // Eseguiamo una DOIO tramite l'SSI per avviare la lettura della pagina
    unsigned int exit_status = doIOFlash(asid, vpn, FLASHREAD, return_position);

    // Se la lettura ha presentato un errore, stampiamo un messaggio di errore e terminiamo
    if (exit_status != 1){
        doIOTerminal(asid, PRINTCHR, "Error reading from flash\n");
        programTrapHandler();
    }

    // Se siamo arrivati qui, la lettura è andata a buon fine e la pagina è stata caricata
    // nella swap pool
    // A questo punto aggiorniamo l'entry della swap table con i nuovi valori (della nuova pagina caricata)
    // Questa parte deve essere eseguita in modo atomico senza interruzioni, quindi salviamo
    // lo stato del processore, disabilitiamo le interruzioni, aggiorniamo la swap table e
    // ripristiniamo lo stato del processore con SETSTATUS()

    unsigned int current_processor_status = ((state_t*) BIOSDATAPAGE)->status;
    setSTATUS(ALLOFF);

    swapTable[roundRobinPick].sw_asid = asid;
    swapTable[roundRobinPick].sw_pageNo = vpn;
    swapTable[roundRobinPick].sw_pte = &(support_struct->sup_privatePgTbl[vpn]);

    // Adesso dobbiamo aggiornare la entry della pagina privata del processo

    // Settiamo l'entryHI della pagina
    // Questa riga è ridondante, ma la lascio per chiarezza
    // support_struct->sup_privatePgTbl[vpn].pte_entryHI = (entryHi >> VPNSHIFT) << VPNSHIFT | (asid << ASIDSHIFT);

    // Settiamo l'entryLO della pagina
    // Da specifiche dobbiamo settare i bit di validità e dirty
    // Inoltre dobbiamo fornire la parte più significativa (PFN) dell'indirizzo fisico della pagina
    // in memoria, per far si che il TLB possa tradurre correttamente l'indirizzo virtuale
    // in indirizzo fisico
    // ESEMPIO: Indirizzo fisico in swap pool: 0x20020030 => PFN = 0x20020 e VPN = 0x030
    // Quindi puliamo l'indirizzo fisico da i 12 bit meno significativi
    unsigned int pfn = (((unsigned int)return_position) >> 12);
    support_struct->sup_privatePgTbl[vpn].pte_entryLO = (pfn << 12) | DIRTYON | VALIDON;

    // Settiamo la entry della TLB
    TLBCLR();
    setENTRYHI(support_struct->sup_privatePgTbl[vpn].pte_entryHI);
    setENTRYLO(support_struct->sup_privatePgTbl[vpn].pte_entryLO);
    TLBWR();

    setSTATUS(current_processor_status);

    // Aumentiamo il round robin pick per la prossima volta
    roundRobinPick = (roundRobinPick + 1) % (2 * UPROCMAX);

    SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
    LDST(&(support_struct->sup_exceptState[0]));
}

void pageFaultHandler(){
    support_t* support_struct = getSupportStruct();
    switch ((support_struct->sup_exceptState[0].cause & GETEXECCODE) >> CAUSESHIFT){
        case TLBINVLDL:
            TLBInvalidHandler(support_struct);
            break;
        case TLBINVLDS:
            TLBInvalidHandler(support_struct);
            break;
        default:
            programTrapHandler();
            break;
    }
}

void SYSCALLExceptionHandler(support_t* support_struct){
    state_t* state = &(support_struct->sup_exceptState[1]);
    unsigned int v0_code;
    if(state->reg_a0 == SENDMSG){
        if (state->reg_a1 == PARENT){
            v0_code = SYSCALL(SENDMESSAGE, (unsigned int)current_process->p_parent, (unsigned int)state->reg_a2, 0);
        }
        else{
            v0_code = SYSCALL(SENDMESSAGE, (unsigned int)state->reg_a1, (unsigned int)state->reg_a2, 0);
        }
    }
    else if(state->reg_a0 == RECEIVEMSG){
        if (state->reg_a1 == PARENT){
            v0_code = SYSCALL(RECEIVEMESSAGE, (unsigned int)current_process->p_parent, (unsigned int)state->reg_a2, 0);
        }
        else{
            v0_code = SYSCALL(RECEIVEMESSAGE, (unsigned int)state->reg_a1, (unsigned int)state->reg_a2, 0);
        }
    }
    current_process->p_supportStruct->sup_exceptState[1].reg_v0 = v0_code;
    current_process->p_supportStruct->sup_exceptState[1].pc_epc += 4;
    LDST(&(current_process->p_supportStruct->sup_exceptState[1]));
}

void programTrapHandler() {
    int parent_pid;

    ssi_payload_t payload = {
        .service_code = GETPROCESSID,
        .arg = (void*)1,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&parent_pid), 0);

    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };
    if (parent_pid != test_pcb->p_pid) {
        // Se entriamo qui significa che il padre è un SST
        // Dobbiamo trovare un modo per killare sia il processo che il padre
        // notificando il test
    }
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

void generalExceptionHandler() {
    support_t* support_struct = getSupportStruct();
    switch ((support_struct->sup_exceptState[1].cause & GETEXECCODE) >> CAUSESHIFT) {
    case SYSEXCEPTION:
        SYSCALLExceptionHandler(current_process->p_supportStruct);
        break;
    default:
        programTrapHandler();
        break;
    }
}
