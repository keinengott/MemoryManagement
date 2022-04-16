#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

void queueInit(Queue *q)
{
    q->first = 0;
    q->last = QUEUESIZE-1;
    q->count = 0;
}

void enqueue(Queue *q, int x)
{
    if (q->count >= QUEUESIZE) printf("Warning: queue overflow enqueue x=%d\n",x);
    else {
        q->last = (q->last+1) % QUEUESIZE;
        q->q[ q->last ] = x;    
        q->count = q->count + 1;
    }
}

int peek(Queue *q) {
    int x;

    if (q->count <= 0) printf("Warning: empty queue peek.\n");
    else x = q->q[ q->first ];

    return x;
}

int dequeue(Queue *q)
{
    int x;

    if (q->count <= 0) printf("Warning: empty queue dequeue.\n");
    else
    {
        x = q->q[ q->first ];
        q->first = (q->first+1) % QUEUESIZE;
        q->count = q->count - 1;
    }

    return x;
}


bool empty(Queue *q)
{
    if (q->count <= 0) return true;
    else return false;
}

char* getQueue(Queue *q)
{
    char* out = malloc(255);

    sprintf(out, " ");
    int i = q->first;

    while (i != q->last)
    {
        sprintf(out + strlen(out), "%2d ", q->q[i]);
        i = (i+1) % QUEUESIZE;
    }

    sprintf(out + strlen(out), "%2d \n",q->q[i]);

    return out;
}

void printQueue(Queue *q)
{
    int i = q->first;
    
    while (i != q->last)
    {
        printf("%2d ", q->q[i]);
        i = (i+1) % QUEUESIZE;
    }

    printf("%2d \n",q->q[i]);
}

