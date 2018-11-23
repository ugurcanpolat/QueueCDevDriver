#include <linux/slab.h> // kmalloc(), kfree()
#include <include/linux/string.h> // strcpy()

#define EENQ 1

typedef struct string_t {
    char* data;
    String *next;
} String;

typedef struct queue_t {
    String *front;
    String *back;
} Queue;

String* new_string(char* text) {
    String *new_data = (String*) kmalloc(sizeof(String), GFP_KERNEL);
    
    if (!new_data)
        return NULL;
    
    memset(new_data, 0, len);
    
    unsigned int len = strlen(text) + 1;
    
    new_data->data = (char*) kmalloc(len * sizeof(char), GFP_KERNEL);
    
    if (!new_data->data) {
        kfree(new_data);
        return NULL;
    }
    
    memset(new_data->data, '\0', len);
    
    strcpy(new_data->data, text);
    
    new_data->next = NULL;
    return new_data;
}

Queue* new_queue() {
    Queue *q = (Queue*) kmalloc(sizeof(Queue), GFP_KERNEL);
    
    if (!q)
        return NULL;
    
    memset(dev->data, 0, sizeof(Queue));
    
    q->front = q->back = NULL;
    return q;
}

unsigned int enqueue(Queue* q, char* text) {
    String* temp = new_string(text);
    
    if (!temp)
        return EENQ;
    
    unsigned int retval = 0;
    
    if (q->front == NULL) {
        q->front = q->back = temp;
        return retval;
    }
    
    q->back->next = temp;
    q->back = temp;
    return retval;
}

char* dequeue(Queue* q) {
    if (q->front == NULL)
        return NULL;
    
    String* dequeued_node = q->front;
    
    if (q->front->next == NULL)
        q->front = q->back = NULL;
    else
        q->front = q->front->next;
    
    char* text = dequeued_node->data;
    
    kfree(dequeued_node);

    return text;
}


