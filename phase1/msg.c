#include "./headers/msg.h"

static msg_t msgTable[MAXMESSAGES];
LIST_HEAD(msgFree_h);

void initMsgs() {
    // Initialize the msgFree list to contain all the elements of the msgTable array. (head inserted)
    for(int i = 0; i < MAXMESSAGES; i++){
        list_add(&msgTable[i].m_list, &msgFree_h);
    }
}

void freeMsg(msg_t *m) {
    // Add the element pointed to by p to the msgFree list. (head inserted)
    list_add(&m->m_list, &msgFree_h);
}

msg_t *allocMsg() {
    // Dequeue an element from msgFree and return a pointer to such element.
    // If the msgFree list is empty, return NULL.
    // !!! After dequeuing, initialize all the fields of the MSG_T to NULL.
    if(list_empty(&msgFree_h)){
        return NULL;
    }
    struct msg_t *m = container_of(msgFree_h.next, msg_t, m_list);
    list_del(msgFree_h.next);
    INIT_LIST_HEAD(&m->m_list);
    m->m_sender = NULL;
    m->m_payload = 0;

    return m;
}

void mkEmptyMessageQ(struct list_head *head) {
    INIT_LIST_HEAD(head);
}

int emptyMessageQ(struct list_head *head) {
    return list_empty(head);
}

void insertMessage(struct list_head *head, msg_t *m) {
    list_add_tail(&m->m_list, head);
}

void pushMessage(struct list_head *head, msg_t *m) {
    list_add(&m->m_list, head);
}

msg_t *popMessage(struct list_head *head, pcb_t *p_ptr) {
    // If the msgQ is empty, return NULL.
    // If p_ptr is NULL, return the first message in the queue.
    // If p_ptr is not NULL, return the first message from the queue that has p_ptr as sender.
    if(emptyMessageQ(head)){
        return NULL;
    }
    if(p_ptr == NULL){
        struct msg_t *toReturn = container_of(head->next, msg_t, m_list);
        list_del(head->next);
        return toReturn;
    }
    msg_t *m;
    list_for_each_entry(m, head, m_list){
        if(m->m_sender == p_ptr){
            list_del(&m->m_list);
            return m;
        }
    }
    return NULL;
}

msg_t *headMessage(struct list_head *head) {
    if (emptyMessageQ(head)) {
        return NULL;
    }
    return container_of(head->next, msg_t, m_list);
}
