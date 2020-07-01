#ifndef _M_QUEUE_H_
#define _M_QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include "m_common.h"
#define ElementType int 
#define MAXSIZE 50

typedef struct {
    ElementType data[MAXSIZE];
    int front; 
    int rear; 
    int size; 
}Queue;

extern Queue* am_queue;

Queue* CreateQueue(char *name);

int IsFullQ(Queue* q);
void AddQ(Queue* q, ElementType item);
int IsEmptyQ(Queue* q);
ElementType DeleteQ(Queue* q);
void PrintQueue(Queue* q);
ElementType * GetQueueItems(Queue* q,int * qitems);

#endif
