#include "util_fifo.h"
#include "my_misc.h"

bool myFifoInit(myFifo_t *ptr)
{
    if(ptr == NULL)
    {
        err_log("myFifoInit failed! ptr is NULL\n");
        return false;
    }
    // init fifo    
    k_fifo_init(&ptr->fifoHandle);
    //init mux
    k_mutex_init(&ptr->fifoMux);
    //init sem
    k_sem_init(&ptr->semHandle,ptr->semStart,ptr->semEnd);
    return true; 
}
bool myFifoSend(myFifo_t *handlePtr,char *msgPtr)
{
    if(handlePtr == NULL || msgPtr == NULL)
    {
        //err_log("formal parameter is NULL\n");
        return false;
    }
    if(k_sem_count_get(&handlePtr->semHandle) >= handlePtr->semEnd)
    {
       // err_log("fifo is full !!\n");
        return false;
    }
    char *msgPtrTmp = k_malloc(handlePtr->elementSize);
    if(msgPtrTmp == NULL)
    {
        err_log("in myFifoSend fun,k_malloc failed\n");
        return false;
    }
    memcpy(msgPtrTmp, msgPtr, handlePtr->elementSize);
    if(k_mutex_lock(&handlePtr->fifoMux,20) != 0)
    {
        err_log("in fifoSend fun,mutex lock failed\n");
        k_free(msgPtrTmp);
        return false;
    }
    k_fifo_put(&handlePtr->fifoHandle, msgPtrTmp);
    k_mutex_unlock(&handlePtr->fifoMux);
    k_sem_give(&handlePtr->semHandle);
    return true;
}
bool myFifoRcv(myFifo_t *handlePtr, char *msgPtr)
{
    if(handlePtr == NULL || msgPtr == NULL)
    {
        err_log(" in myFifoRcv fun,formal parameter is NULL\n");
        return false;
    }
    if(k_sem_take(&handlePtr->semHandle,0))
    {
        //err_log("sempha is not come\n ");
        return false;
    }
    char * msgTmp = NULL;
    if(k_mutex_lock(&handlePtr->fifoMux,20) != 0)
    {
        err_log("in fifoSend fun,mutex lock failed\n");
        return false;
    }
    msgTmp =k_fifo_get(&handlePtr->fifoHandle, 0);
    k_mutex_unlock(&handlePtr->fifoMux);
    if(msgTmp == NULL)
    {
        //err_log("fifo is empty\n");
    }
    memcpy(msgPtr,msgTmp,handlePtr->elementSize);
	k_free(msgTmp);
    
    return true;
}


