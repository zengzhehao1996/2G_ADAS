#include "my_bytequeue.h"
#include "my_misc.h"

/**
 * @brief initialise queue
 * @param q, pointer to myByteQueue struct.
 * @param size, queue size.
 * @return true on success; false on fail.
 */
bool byteQueueInit(myByteQueue_t *q,int size)
{
    if(q==NULL || size <= 0)
    {
        return false;
    }

    //step1. malloc the buffer
    q->buffer_ = (uint8_t*)k_malloc(size);

    if(q->buffer_==NULL){
        return false;
    }
    q->size_ = size;

    //step2. reset
    byteQueueReset(q);

    return true;
}

/**
 * @brief destory the queue
 * @param q, pointer to myByteQueue struct.
 * @return none.
 */
void byteQueueDestroy(myByteQueue_t *q)
{
    if(q==NULL || q->buffer_==NULL)
    {
        return;
    }

    k_free(q->buffer_);
    q->buffer_ = NULL;
    return;
}

/**
 * @brief reset the queue
 * @param q, pointer to myByteQueue struct.
 * @return none.
 */
void byteQueueReset(myByteQueue_t *q)
{
    if(q==NULL || q->buffer_==NULL)
    {
        return;
    }

    memset(q->buffer_,0,q->size_);
    q->index_ = 0;
}

/**
 * @brief push byte to queue
 * @param q, pointer to myByteQueue struct.
 * @param b, the byte to push.
 * @return none.
 */
void byteQueuePush(myByteQueue_t *q,uint8_t b)
{
    if(q==NULL ||q->buffer_==NULL)
    {
        return;
    }
    if (byteQueueFull(q))
    {
        return;
    }
    q->buffer_[q->index_++] = b;
}

/**
 * @brief pop byte from the queue
 * @param q, pointer to myByteQueue struct.
 * @return none.
 */
void byteQueuePop(myByteQueue_t *q)
{
    if(q==NULL || q->buffer_==NULL)
    {
        return;
    }
    if(q->index_<=0)
    {
        return;
    }
    memcpy(q->buffer_, q->buffer_ + 1, q->size_ - 1);
    q->index_ -= 1;
}

/**
 * @brief Determine whether the queue is full
 * @param q, pointer to myByteQueue struct.
 * @param size, queue size.
 * @return true on full; false on unfull.
 */
bool byteQueueFull(myByteQueue_t *q)
{
    if(q==NULL)
    {
        return false;
    }
    return (q->index_==q->size_);
}