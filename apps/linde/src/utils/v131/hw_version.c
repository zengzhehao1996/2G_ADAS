#include "hw_version.h"
#include "tm_stm32f4_otp.h"
#include "my_misc.h"
#include "smart_link_version.h"

#define DEV_HARD_BLOCK   0   /* use 0 ( 0 to 15 ) */
#define DEV_VERSION_BITS 4

uint8_t g_hw_major=0;
uint8_t g_hw_minor=0;
uint8_t g_hw_tiny =0;
uint8_t g_hw_patch=0;
uint8_t g_hw_text[DEV_TEXT_SIZE]={0};

bool getHardVersion(uint8_t *pa, uint8_t *pb, uint8_t *pc, uint8_t *pd)
{
    g_hw_major=DEV_VERSION_MAJOR;
    g_hw_minor=DEV_VERSION_MINOR;
    g_hw_tiny =DEV_VERSION_TINY;
    g_hw_patch=DEV_VERSION_PATCH;
    strncpy(g_hw_text,"201844",sizeof(g_hw_text));
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
    return 0;
}

void print_hw_version()
{
    print_log("\tHard version: [ %d.%d.%d.%d %s ]\n",g_hw_major,g_hw_minor,g_hw_tiny,g_hw_patch,g_hw_text);
}