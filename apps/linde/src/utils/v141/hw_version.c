#include "hw_version.h"
#include "tm_stm32f4_otp.h"
#include "my_misc.h"

#define DEV_HARD_BLOCK   0   /* use 0 ( 0 to 15 ) */
#define DEV_VERSION_BITS 4

void versionWrite()
{
    uint8_t a = 1;
    uint8_t b = 4;
    uint8_t c = 1;
    uint8_t d = 0;
    char *buf = "201844";
    hardVersionOtpWrite(DEV_HARD_BLOCK,DEV_VERSION_BITS,a,b,c,d);
    hardInfoOtpWrite(DEV_HARD_BLOCK,DEV_VERSION_BITS,strlen(buf),buf);
    hardOtpBlockLock(DEV_HARD_BLOCK);
}

uint8_t g_hw_major=0;
uint8_t g_hw_minor=0;
uint8_t g_hw_tiny =0;
uint8_t g_hw_patch=0;
uint8_t g_hw_text[DEV_TEXT_SIZE]={0};

bool getHardVersion(uint8_t *pa, uint8_t *pb, uint8_t *pc, uint8_t *pd)
{
    if (0==hardOtpReadVersion(DEV_HARD_BLOCK, pa, pb, pc, pd))
    {
        getHardInfo(g_hw_text);
        return true;
    }
    else
    {
        return false;
    }

}
/* used after getHardversion() */
void copyHardVersion(uint8_t *pa, uint8_t *pb, uint8_t *pc, uint8_t *pd)
{
    *pa = g_hw_major;
    *pb = g_hw_minor;
    *pc = g_hw_tiny;
    *pd = g_hw_patch;
}

uint8_t getHardInfo(char *buf)
{
    return hardOtpReadInfo(DEV_HARD_BLOCK,DEV_VERSION_BITS,buf);
}

void print_hw_version()
{
    print_log("\tHard version: [ %d.%d.%d.%d %s ]\n",g_hw_major,g_hw_minor,g_hw_tiny,g_hw_patch,g_hw_text);
}