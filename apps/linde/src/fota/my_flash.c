#include "my_flash.h"
#include "my_misc.h"

#include <kernel.h>
#include <flash.h>

#define FLASH_OFFSET_MIN 0x60000
#define FLASH_FOTA_BLOCK_SIZE 0x20000
#define FLASH_EMPTY_CODE 0xFF
#define FLASH_BLOCK_TEST_SIZE 16

struct device* g_flash_dev = NULL;

K_MUTEX_DEFINE(g_mutex_flash_dev);

#ifdef _USE_FLASH_ /* Reserved */
static bool isMemEqual(uint8_t* dst,uint8_t*src, int len);
static bool getEmptyBlock(my_flash_block_t * ptr_block,int * out_index);
static bool myFlashIsEmpty(uint8_t * data, int len);
static uint32_t getBlockOffset(my_flash_block_t * ptr_block, int index);
#endif

bool initFlash(void)
{
    //step1. init device
    if(g_flash_dev)
    {
        warning_log("It's already been initialized.\n");
        return true;
    }
    g_flash_dev = device_get_binding(FLASH_DEV_NAME);
    if(g_flash_dev == NULL)
    {
        err_log("Init Flash FAILED!!!\n");
        return false;
    }
    print_log("Init Flash OK.\n");
    return true;
}

bool myFlashWrite(int offset, void* data, int len)
{
    int rc = 0;

    if(g_flash_dev == NULL)
    {
        err_log("None pointer flash device found.\n");
        return false;
    }
    if(data == NULL || len <= 0)
    {
        err_log("Invalid data pointer or data length\n");
        return false;
    }

    if(0 != k_mutex_lock(&g_mutex_flash_dev, 50))
    {
        err_log("Don't GET FLASH MUTEX.\n");
        return false;
    }

    //step1.read data bytes
    rc = flash_write_protection_set(g_flash_dev, false);
    if(rc)
    {
        err_log("Turn OFF flash write protection FAILED.\n");
        goto END;
    }
    print_log("!!!!!!!!!!offset:0x%x\n", offset);
    rc = flash_write(g_flash_dev, offset, data, len);
    print_log("write len:%d\n", len);
    flash_write_protection_set(g_flash_dev, true);
    print_log("flash write ret:%d\n", rc);

END:
    k_mutex_unlock(&g_mutex_flash_dev);
    return (rc == 0);
}

bool myFlashRead(int offset, void* data, int len)
{
    bool val = false;
    if(g_flash_dev == NULL)
        return false;
    if(data == NULL || len <= 0)
        return false;

    if(0 != k_mutex_lock(&g_mutex_flash_dev, 50))
    {
        err_log("Don't GET FLASH MUTEX.\n");
        return false;
    }

    //step1.read data bytes
    memset(data, 0, len);
    if(flash_read(g_flash_dev, offset, data, len))
    {
        val = false;
        goto END;
    }

    val = true;

END:
    k_mutex_unlock(&g_mutex_flash_dev);
    return val;
}

bool myFlashErase(int offset)
{
    struct flash_pages_info info;

    if(offset < FLASH_OFFSET_MIN)
    {
        err_log("Erasing address is illegal!\n");
        return false;
    }

    if(0 != k_mutex_lock(&g_mutex_flash_dev, 100))
    {
        err_log("Don't GET FLASH MUTEX.\n");
        return false;
    }

    if(flash_get_page_info_by_offs(g_flash_dev, offset, &info))
    {
        err_log("Fail to get page info\n");
    }
    print_log("$$$$ ERASE!! Page offset 0x%x, size is 0x%x $$$$\n",
              info.start_offset, info.size);

    flash_write_protection_set(g_flash_dev, false);
    int rc = flash_erase(g_flash_dev, info.start_offset, info.size);
    flash_write_protection_set(g_flash_dev, true);

END:
    k_mutex_unlock(&g_mutex_flash_dev);
    return (rc == 0);
}

int bootfm_set_confirmed(void)
{
    u32_t readout;
    int   ret;

    if((ret = bootfm_check_magic(FLASH_AREA_IMAGE_0_OFFSET)) != 0)
        return -3;

    if(!myFlashWrite(FLASH_AREA_IMAGE_0_OFFSET + FLASH_AREA_IMAGE_0_SIZE - 16,
                     img_magic, 16))
        return -1;

    if((ret = boot_write_img_confirmed()) != 0)
        return -5;

    if((ret =
            flash_read(g_flash_dev,
                       FLASH_AREA_IMAGE_0_OFFSET + FLASH_AREA_IMAGE_0_SIZE - 24,
                       &readout, sizeof(readout)))
       != 0)
        return -6;

    if((readout & 0xff) != 1)
        return -7;

    return 0;
}

int bootfm_check_magic(uint32_t off)
{
    u32_t readout[ARRAY_SIZE(img_magic)];
    int   ret;

    if((ret = flash_read(g_flash_dev,
                         off + FLASH_AREA_IMAGE_0_SIZE - sizeof(img_magic),
                         &readout, sizeof(img_magic)))
       != 0)
        return -1;

    if(memcmp(img_magic, readout, sizeof(img_magic)) != 0)
        return -1;

    return 0;
}

static int my_fota_boot_erase_img_bank(uint32_t bank_offset)
{
    int rc;

    rc = flash_write_protection_set(g_flash_dev, false);
    if(rc)
    {
        return rc;
    }

    rc = flash_erase(g_flash_dev, bank_offset, FOTA_FLASH_BANK_SIZE);
    if(rc)
    {
        return rc;
    }

    rc = flash_write_protection_set(g_flash_dev, true);

    return rc;
}

#ifdef _USE_FLASH_ /* Reserved */
bool myFlashBlockInit(my_flash_block_t * ptr_block, uint32_t offset, uint32_t block_size)
{
  if(ptr_block==NULL)return false;

  struct flash_pages_info info;
  //Find the starting address of the block
  if(flash_get_page_info_by_offs(g_flash_dev, offset,&info)){
    err_log("Fail to get page info\n");
    return false;
  }

  memset((uint8_t*)ptr_block,0,sizeof(my_flash_block_t));
  ptr_block->start_offset = info.start_offset;
  ptr_block->total_size = info.size;
  ptr_block->block_size = block_size;
  ptr_block->block_number = ptr_block->total_size / block_size;
  print_log("Init flash block:\n"
            "\tstart offset: 0x%x\n"
            "\ttotal size  : %d\n"
            "\tblock size  ::%d\n"
            "\tblock number: %d\n",
            ptr_block->start_offset,
            ptr_block->total_size,
            ptr_block->block_size,
            ptr_block->block_number);

  return true;
}

void myFlashBlockAlloc(my_flash_block_t * ptr_block,uint32_t *ptr_offset,int* ptr_index)
{
  int index=0;
  uint32_t  offset=0;
  
  if(!getEmptyBlock(ptr_block,&index)){
    err_log(">>>>>Fail to malloc empty index. ERASE THE WHOLE PAGE!!!!\n");
    myFlashErase(ptr_block->start_offset);
    index = 0;
  }
  
  offset = getBlockOffset(ptr_block,index);

  if(ptr_index) {*ptr_index = index;}
  if(ptr_offset){*ptr_offset = offset;}
}

bool myFlashBlockGetLastRecord(my_flash_block_t * ptr_block,uint32_t *ptr_offset,int* ptr_index)
{
  int index=0;
  uint32_t offset=0;

  bool ret = false;

  if(ptr_block==NULL)return false;
  
  if(getEmptyBlock(ptr_block,&index))
  {//there is empty block
    if(index>0){
      //if the first block is not empty
      index = index - 1;
      ret = true;
    }
  }else
  {//there is no empty block, so the whole area should be full
    index = ptr_block->block_number-1;
    ret = true;
  }
  
  offset =  getBlockOffset(ptr_block,index);

  if(ptr_index) {*ptr_index = index;}
  if(ptr_offset){*ptr_offset = offset;}

  return ret;
}

bool myFlashBlockWrite(my_flash_block_t * ptr_block,uint8_t* data, int len)
{
  if(data==NULL || len<=0){
    err_log("Invalid data pointer or data length\n");
    return false;
  }
  if(g_flash_dev==NULL){
    err_log("Invalid flash device pointer found.\n");
    return false;
  }
  if(ptr_block->start_offset<FLASH_OFFSET_MIN){
    err_log("Invalid flash block structure found.\n");
    return false;
  }

  bool ret=false;
  int index=0;
  uint32_t off=0;
  uint8_t* src_buffer=NULL;
  //step1. check if record exists
  if(!myFlashBlockGetLastRecord(ptr_block,&off, &index))
  {//if record address doesn't exist, write data to this offset directly
    err_log("Fail to find available flash record.\n");
    ret =  myFlashWrite(off,data,len);
    goto END;
  }

  //step2. malloc buffer
  src_buffer = (uint8_t*)k_malloc(len);
  if(src_buffer==NULL){
    err_log("Fail to malloc %d bytes.\n",len);
    return false;
  }

  //step3.load original data
  if(!myFlashRead(off,src_buffer,len)){
    err_log("Fail to read flash %d bytes.\n",len);
    ret =  false;
    goto END;
  }

  //step4.compare data
  if(isMemEqual(src_buffer, data, len)){
    print_log("The flash data equals!\n");
    ret = true;
    goto END;
  }

  //step5.new block
  off=ptr_block->start_offset;
  myFlashBlockAlloc(ptr_block,&off, NULL);

  //step6.write to block
  ret = myFlashWrite(off,data,len);
  if(ret){
    print_log("Write to flash OK.\n");
  }else{
    print_log("Write to falsh FAILED!\n");
  }
  
END:
  k_free(src_buffer);
  return ret;
}

bool myFlashBlockRead(my_flash_block_t * ptr_block,uint8_t* data,int len)
{
  if(data==NULL || len<=0)return false;
  if(g_flash_dev==NULL)return false;
    
  int index=0;
  uint32_t off=0;
  uint8_t* src_buffer=NULL;

  //step0. clear data
  memset(data,0,len);

  //step1. check if record exists
  if(!myFlashBlockGetLastRecord(ptr_block,&off, &index) && index==0)
  {//if record address doesn't exist, write data to this offset directly
    return false;
  }

  //step2. read data
  return myFlashRead(off,data,len);
}

static bool isMemEqual(uint8_t* dst,uint8_t*src, int len)
{
  return (0==memcmp(dst,src,len));
}

static uint32_t getBlockOffset(my_flash_block_t * ptr_block, int index)
{
  if(index<0 || ptr_block==NULL)return 0;
  uint32_t offset = index*ptr_block->block_size+ptr_block->start_offset;
  return offset;
}

static bool getEmptyBlock(my_flash_block_t * ptr_block,int * out_index)
{
  uint8_t ptr_buffer[FLASH_BLOCK_TEST_SIZE];
  int empty_index=0;
  bool found=false;

  if(ptr_block==NULL){
    err_log(">>>>>>>>>>>>>>>>>>>>>BLOCK is empty!\n");
    return false;
  }

  //step2.traversal all flash blocks 
  for(int i=0;i<ptr_block->block_number;++i){
    //step1.compute offset
    uint32_t off = getBlockOffset(ptr_block,i);
    
    //step2.load data from flash
    memset(ptr_buffer,0,FLASH_BLOCK_TEST_SIZE);

    if(flash_read(g_flash_dev, off, ptr_buffer, FLASH_BLOCK_TEST_SIZE)){
      err_log("Fail to read bytes from [0x%x]\n",off);
      continue;
    }

    //step3.check empty
    if(myFlashIsEmpty(ptr_buffer, FLASH_BLOCK_TEST_SIZE)){
      empty_index = i;
      found = true;
      break;
    }
  }
  if(found){
    *out_index = empty_index;
    print_log("Offset[0x%x] Empty flash index is:%d\n",ptr_block->start_offset,empty_index);
  }

END:

  return found;  
}

static bool myFlashIsEmpty(uint8_t * data, int len)
{
  if(data==NULL ||len<=0)return false;
  for(int i=0;i<len;++i){
    if(data[i]!=FLASH_EMPTY_CODE){
      return false;
    }
  }
  return true;
}
#endif