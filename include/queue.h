#include <stdbool.h>

#define QUEUESIZE 18

struct Queue {
        int q[QUEUESIZE+2];		        /* body of queue */
        int first;                      /* position of first element */
        int last;                       /* position of last element */
        int count;                      /* number of queue elements */
};
typedef struct Queue Queue;

void queueInit(Queue *q);
void enqueue(Queue *q, int x);
int dequeue(Queue *q);
bool empty(Queue *q);
void printQueue(Queue *q);
int peek(Queue *q);
char* getQueue(Queue *q);
