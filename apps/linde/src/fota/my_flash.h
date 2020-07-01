#ifndef __MY_FLASH_H__
#define __MY_FLASH_H__
#include <kernel.h>

#define FOTA_FLASH_ADDR 0x60000
#define FOTA_FLASH_ADDR2 0x80000
#define FOTA_FLASH_BANK_SIZE 0X40000
#define FOTA_FILE_BLOCK_SIZE 1024

#define FOTA_TARGET_SIZE 262144 //256k

#define BOOT_MAGIC_VAL_W0 0xf395c277
#define BOOT_MAGIC_VAL_W1 0x7fefd260
#define BOOT_MAGIC_VAL_W2 0x0f505235
#define BOOT_MAGIC_VAL_W3 0x8079b62c
#define BOOT_MAGIC_VALUES {BOOT_MAGIC_VAL_W0, BOOT_MAGIC_VAL_W1,\
			   BOOT_MAGIC_VAL_W2, BOOT_MAGIC_VAL_W3 }

static const uint32_t img_magic[4] = BOOT_MAGIC_VALUES;

typedef struct my_flash_block_s{
  uint32_t start_offset;
  uint32_t total_size;
  uint32_t block_size;
  uint32_t block_number;
}my_flash_block_t;


bool initFlash(void);
bool myFlashWrite(int offset, void* data, int len);
bool myFlashRead(int offset, void* data, int len);
bool myFlashErase(int offset);

#undef _USE_FLASH_ /* if use flash block ctrl function define it */
#ifdef _USE_FLASH_ /* Reserved */
bool myFlashBlockInit(my_flash_block_t * ptr_block, uint32_t offset, uint32_t block_size);
void myFlashBlockAlloc(my_flash_block_t * ptr_block,uint32_t *ptr_offset,int* ptr_index);
bool myFlashBlockGetLastRecord(my_flash_block_t * ptr_block,uint32_t *ptr_offset,int* ptr_index);
bool myFlashBlockWrite(my_flash_block_t * ptr_block,uint8_t* data, int len);
bool myFlashBlockRead(my_flash_block_t * ptr_block,uint8_t* data,int len);
#endif
int bootfm_set_confirmed(void);

#endif