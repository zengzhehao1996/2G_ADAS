#include "hw_version.h"
#include "tm_stm32f4_otp.h"
#include "my_misc.h"

#define DEV_HARD_BLOCK   0   /* use 0 ( 0 to 15 ) */
#define DEV_VERSION_BITS 4
#if 0
 void versionWrite()
 {
     uint8_t a = 1;
     uint8_t b = 3;
     uint8_t c = 5;
     uint8_t d = 0;
     char *buf = "201931";
     hardVersionOtpWrite(DEV_HARD_BLOCK,DEV_VERSION_BITS,a,b,c,d);
     hardInfoOtpWrite(DEV_HARD_BLOCK,DEV_VERSION_BITS,strlen(buf),buf);
     hardOtpBlockLock(DEV_HARD_BLOCK);
 }
#endif
uint8_t g_hw_major=0;
uint8_t g_hw_minor=0;
uint8_t g_hw_tiny =0;
uint8_t g_hw_patch=0;
uint8_t g_hw_text[DEV_TEXT_SIZE]={0};

bool getHardVersion(uint8_t *pa, uint8_t *pb, uint8_t *pc, uint8_t *pd)
{
    hardOtpReadVersion(DEV_HARD_BLOCK, pa, pb, pc, pd);
    getHardInfo(g_hw_text);
    return true;
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