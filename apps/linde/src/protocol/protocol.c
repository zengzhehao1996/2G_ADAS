#include "protocol.h"
#include "my_misc.h"
#include "my_bytequeue.h"
#include <string.h>

/* protocol magic number */
#define MAGIC_NUM1 0x48
#define MAGIC_NUM2 0x49
#define MAGIC_NUM3 0x43
#define MAGIC_NUM4 0x4a
#define MAGIC_NUM_LEN 4

//magic struct design
#pragma pack (1)
typedef struct message_head_s
{
    uint8_t magicNum1_;
    uint8_t magicNum2_;
    uint8_t magicNum3_;
    uint8_t magicNum4_;
    uint16_t protocolVersion_;
    uint16_t cmdId_;
    uint32_t len_;//this is the length of the total pack include head size.
                //e.g. len=HEAD_SIZE+PAYLOAD_SIZE
}messageHead_t;
#pragma pack()

static const int MESSAGE_HEAD_LEN = sizeof(messageHead_t);

#define MAX_RX_SIZE 2048                  /* equal to isr buffer */
static struct {
    uint8_t data_[MAX_RX_SIZE];
    uint16_t rxLen_;
}gParserRX;

static volatile uint16_t gRxPayloadSize;
static myByteQueue_t gHead;
static cpParserReady_t gReadyFptr=NULL;

static void cpParserPushByte(uint8_t data);
static bool paserCheckHead(uint8_t *pdata, int len);

uint32_t cpHeadSize(void)
{
    return sizeof(messageHead_t);
}

bool cpParserInit(cpParserReady_t fptr)
{
    /* step1. create head */
    if(false == byteQueueInit(&gHead,cpHeadSize()))
    {
        err_log("Can't create gHead queue.\n");
        return false;
    }

    /* step2.reset parser */
    cpParserReset();

    /* step3. set call back function */
    gReadyFptr = fptr;

    return true;
}

void cpParserReset(void)
{
    /* set receive data false */
    setRecvData(false);

    /* reset body */
    gRxPayloadSize = 0;
    memset(&gParserRX, 0, sizeof(gParserRX));

    /* reset head */
    byteQueueReset(&gHead);
}

static bool paserCheckHead(uint8_t *magic, int len)
{
    if(magic==NULL || len<MAGIC_NUM_LEN)
    {
        return false;
    }
    return ((magic[0]==MAGIC_NUM1)&&
            (magic[1]==MAGIC_NUM2)&&
            (magic[2]==MAGIC_NUM3)&&
            (magic[3]==MAGIC_NUM4));
}

static void cpParserPushByte(uint8_t data)
{
    if(false == isRecvData())
    {
        /* step1. push data to the head queue */
        byteQueuePush(&gHead,data);

        /* step2. return when queue is not full */
        if(false == byteQueueFull(&gHead))
        {
            return ;
        }

        /* step3. check head */
        if(true == paserCheckHead(gHead.buffer_,gHead.index_))
        {
            /* step3.1 read payload len */
            messageHead_t* p = (messageHead_t*)(gHead.buffer_);
            gRxPayloadSize = p->len_ - cpHeadSize();
            /* step3.2 read payload error */
            if(gRxPayloadSize > MAX_RX_SIZE)
            {
                cpParserReset();
                err_log("Read server Send payload ERROR.\n");
                return ;
            }

            /* step3.3 start receive payload */
            setRecvData(true);
            memset(&gParserRX, 0, sizeof(gParserRX));
        }
        else
        {
            /* step not find head pop byte */
            byteQueuePop(&gHead);
        }
    }
    else
    {
        /* step4 if is processing the payload */
        gParserRX.data_[gParserRX.rxLen_] = data;
        gParserRX.rxLen_++;
    }
    
    /* step5 receive ok and call back */
    
    if(gParserRX.rxLen_ >= gRxPayloadSize && isRecvData())
    {
        //print_log("rxLen:[%d] size:[%d]------------------------------\n",gParserRX.rxLen_,gRxPayloadSize);
        if(gReadyFptr)
        {
            messageHead_t *p = (messageHead_t*)gHead.buffer_;
            //print_log("cmdid:[%p] version:[%u] rxLen:[%u] \n",p->cmdId_,p->protocolVersion_,gParserRX.rxLen_);
            gReadyFptr(p->cmdId_, p->protocolVersion_, gParserRX.data_, gParserRX.rxLen_);
            //print_log("cmdid:[%p] version:[%u] rxLen:[%u] \n",p->cmdId_,p->protocolVersion_,gParserRX.rxLen_);
        }
        cpParserReset();
    }
}

void cpParserPushBytes(uint8_t *data, int len)
{
    print_log("len=%d\n",len);
    if(!data || len <=0)
    {
        return ;
    }
    for(int i=0;i<len;i++)
    {
        cpParserPushByte(data[i]);
    }
    //print_log("paser:[%d]............................\n",len);
}

/**
 * @param fill the message head of the buffer
 * @cmdid, the command id
 * @version, the new version ID
 * @sendLen, the length of payload, HEAD size not included
 * @data, Pointer to the data buffer
 * @size, the length of the buffer
 * @returns positive on success,negative on failed
 */
int  cpPaserFillHead(uint16_t cmdid,uint16_t version, uint16_t sendLen, uint8_t *data, int size)
{
    if(data==NULL || size<MESSAGE_HEAD_LEN){
        return -1;
    }

    messageHead_t * temp_ptr = (messageHead_t*)data;
    temp_ptr->cmdId_ = cmdid;
    temp_ptr->protocolVersion_ = version;
    temp_ptr->len_ = MESSAGE_HEAD_LEN + sendLen;
    temp_ptr->magicNum1_  = MAGIC_NUM1;
    temp_ptr->magicNum2_  = MAGIC_NUM2;
    temp_ptr->magicNum3_  = MAGIC_NUM3;
    temp_ptr->magicNum4_  = MAGIC_NUM4;
    return sizeof(messageHead_t);
}

static bool g_isRecvData = false; //only used for protocol
bool isRecvData(void)
{
    return g_isRecvData;
}
void setRecvData(bool val)
{
    g_isRecvData = val;
}
