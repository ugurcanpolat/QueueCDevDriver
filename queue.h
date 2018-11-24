#include <linux/slab.h> // kmalloc(), kfree()
#include <linux/string.h> // strcpy()

#define EENQ 1

struct node {
    char* data;
    struct node *next;
};

struct queue {
    struct node *front;
    struct node *back;
};

struct node* new_string(char* text) {
    struct node *new_data = (struct node*) kmalloc(sizeof(struct node), GFP_KERNEL);
    unsigned int len;
     
    if (!new_data)
        return NULL;
    
    memset(new_data, 0, sizeof(struct node));
    
    len = strlen(text) + 1;
    
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

struct queue* new_queue(void) {
    struct queue *q = (struct queue*) kmalloc(sizeof(struct queue), GFP_KERNEL);
    
    if (!q)
        return NULL;
    
    memset(q, 0, sizeof(struct queue));
    
    q->front = q->back = NULL;
    return q;
}

unsigned int enqueue(struct queue* q, char* text) {
    struct node* temp = new_string(text);
    unsigned int retval = 0;
    
    if (!temp)
        return EENQ;
    
    if (q->front == NULL) {
        q->front = q->back = temp;
        return retval;
    }
    
    q->back->next = temp;
    q->back = temp;
    return retval;
}

char* dequeue(struct queue* q) {
	struct node* dequeued_node;
	char* text;
	
    if (q->front == NULL)
        return NULL;
    
    dequeued_node = q->front;
    
    if (q->front->next == NULL)
        q->front = q->back = NULL;
    else
        q->front = q->front->next;
    
    text = dequeued_node->data;
    
    kfree(dequeued_node);

    return text;
}


