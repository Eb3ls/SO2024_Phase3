#include "./headers/pcb.h"

static pcb_t pcbTable[MAXPROC];
LIST_HEAD(pcbFree_h);
static int next_pid = 1;

void initPcbs() {
    // Initialize the pcbFree list to contain all the elements of the pcbTable array. (head inserted)
    for (int i = 0; i < MAXPROC; i++){
        list_add(&pcbTable[i].p_list, &pcbFree_h);
    }
}

void freePcb(pcb_t *p) {
    // Add the element pointed to by p to the pcbFree list. (head inserted)
    list_add(&p->p_list, &pcbFree_h);
}

pcb_t *allocPcb() {
    // Dequeue an element from pcbFree and return a pointer to such element. 
    // If the pcbFree list is empty, return NULL.
    // !!! After dequeuing, initialize all the fields of the PCB to NULL.
    if (list_empty(&pcbFree_h)) {
        return NULL;
    }
    struct pcb_t *pcb = container_of(pcbFree_h.next, pcb_t, p_list);
    list_del(pcbFree_h.next);

    INIT_LIST_HEAD(&pcb->p_list);
    pcb->p_parent = pcb;
    INIT_LIST_HEAD(&pcb->p_child);
    INIT_LIST_HEAD(&pcb->p_sib);

    // Initialize the processor state
    pcb->p_s.entry_hi = 0;
    pcb->p_s.cause = 0;
    pcb->p_s.status = 0;
    pcb->p_s.pc_epc = 0;
    for (int i = 0; i < STATE_GPR_LEN; i++) {
        pcb->p_s.gpr[i] = 0;
    }
    pcb->p_s.hi = 0;
    pcb->p_s.lo = 0;
    pcb->p_time = 0;
    INIT_LIST_HEAD(&pcb->msg_inbox);

    pcb->p_supportStruct = NULL;

    pcb->p_pid = next_pid;
    next_pid++;

    pcb->p_location = NOTLOCATED_LOCATION;

    return pcb;
}

void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head) {
    // Return TRUE if the queue whose head is pointed to by head is empty. Return FALSE otherwise.
    return list_empty(head);
}

void insertProcQ(struct list_head *head, pcb_t *p) {
    // Insert the PCB pointed by p into the process queue whose head pointer is pointed to by head.
    list_add_tail(&p->p_list, head);
}

pcb_t *headProcQ(struct list_head *head) {
    // Return a pointer to the first PCB from the process queue whose head is pointed to by head.
    // This does not remove the PCB from the process queue.
    // Return NULL if the process queue is empty.
    if (emptyProcQ(head)) {
        return NULL;
    }
    return container_of(head->next, pcb_t, p_list);
}

pcb_t *removeProcQ(struct list_head *head) {
    // Remove the first element from the process queue whose head is pointed to by head.
    if (emptyProcQ(head)) {
        return NULL;
    }
    struct pcb_t *pcb = container_of(head->next, pcb_t, p_list);
    list_del(head->next);
    return pcb;
}

pcb_t *outProcQ(struct list_head *head, pcb_t *p) {
    // Remove the PCB pointed to by p from the process queue whose head pointer is pointed to by head. 
    // If the desired entry is not in the indicated queue (an error condition), return NULL;
    // otherwise, return p.
    struct list_head *pos = head;
    while (pos->next != head) {
        if (pos->next == &p->p_list) {
            list_del(pos->next);
            return p;
        }
        pos = pos->next;
    }
    return NULL;
}

int emptyChild(pcb_t *p) {
    return list_empty(&p->p_child);
}

void insertChild(pcb_t *prnt, pcb_t *p) {
    // Make the PCB pointed to by p a child of the PCB pointed to by prnt.
    list_add_tail(&p->p_sib, &prnt->p_child);
    p->p_parent = prnt;
}

pcb_t *removeChild(pcb_t *p) {
    // Make the first child of the PCB pointed to by p no longer a child of p.
    // Return NULL if p_child list is empty.
    // Otherwise, return a pointer to this removed first child PCB.
    if (emptyChild(p)) {
        return NULL;
    }
    struct pcb_t *pcb = container_of(p->p_child.next, pcb_t, p_sib);
    list_del(p->p_child.next);
    return pcb;
}

pcb_t *outChild(pcb_t *p) {
    // Make the PCB pointed to by p no longer the child of its parent.
    // If the PCB pointed to by p has no parent, return NULL; 
    // otherwise, return p.
    if(p->p_parent != p){
        p->p_parent = p;
        list_del(&p->p_sib);
        return p;
    }
    return NULL;
}
