#ifndef __HW_VERSION_H__
#define __HW_VERSION_H__

#include <zephyr.h>

extern uint8_t g_hw_major;
extern uint8_t g_hw_minor;
extern uint8_t g_hw_tiny ;
extern uint8_t g_hw_patch;
extern uint8_t g_hw_text[];

bool getHardVersion(uint8_t *pa, uint8_t *pb, uint8_t *pc, uint8_t *pd);
uint8_t getHardInfo(char *buf);
void print_hw_version();
/* used after getHardversion() */
void copyHardVersion(uint8_t *pa, uint8_t *pb, uint8_t *pc, uint8_t *pd);

#endif /* __HW_VERSION_H__ */