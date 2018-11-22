#include <linux/slab.h> // kmalloc()

typedef struct string_t {
    char* data;
    String *next;
} String;

typedef struct queue_t {
    String *front;
    String *back;
} Queue;

String* newString(char* text) {
    String *newData = (String*) kmalloc(sizeof(String), GFP_KERNEL);
    newData->data = text;
    newData->next = NULL;
    return newData;
}

Queue* newQueue() {
    Queue *q = (Queue*) kmalloc(sizeof(Queue), GFP_KERNEL);
    q->front = q->back = NULL;
    return q;
}

void enqueue(Queue* q, char* text) {
    String* temp = newString(text);
    
    if (q->front == NULL) {
        q->front = q->back = temp;
        return;
    }
    
    q->back->next = temp;
    q->back = temp;
}

String* dequeue(Queue* q) {
    if (q->front == NULL)
        return NULL;
    
    String* dequeuedNode = q->front;
    
    if (q->front->next == NULL)
        q->front = q->back = NULL;
    else
        q->front = q->front->next;

    return dequeuedNode;
}


