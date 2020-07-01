#ifndef MY_BYTEQUEUE_H
#define MY_BYTEQUEUE_H

#include <kernel.h>

typedef struct {
  uint8_t *buffer_;
  int index_;
  int size_;
}myByteQueue_t;

bool byteQueueInit(myByteQueue_t *q,int size);
void byteQueueDestroy(myByteQueue_t *q);

void byteQueueReset(myByteQueue_t *q);
void byteQueuePush(myByteQueue_t *q,uint8_t b);
void byteQueuePop(myByteQueue_t *q);
bool byteQueueFull(myByteQueue_t *q);

#endif // MY_BYTEQUEUE_H
