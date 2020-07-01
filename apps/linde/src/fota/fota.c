#include "my_md5.h"
#include "fota.h"
#include "ff.h"
#include "my_flash.h"
#include "flash.h"
#include "my_misc.h"

K_MUTEX_DEFINE(g_fota_bin_mutex);

void deleteFotaFile(void)
{
    FIL      fp;
    uint32_t size;

    if(FR_OK != f_open(&fp, FOTA_FILE_PATH, FA_CREATE_ALWAYS | FA_WRITE))
    {
        err_log("FAIL TO LOAD FILE : %s\n", FOTA_FILE_PATH);
        k_mutex_unlock(&g_fota_bin_mutex);
        return;
    }
    size = f_size(&fp);
    f_close(&fp);
    f_unlink(FOTA_FILE_PATH);
    print_log("Delete file [%s],size [%u]\n", FOTA_FILE_PATH, size);
}

/* return file size */
int pushBytesToFotaFile(char* data, int len, uint32_t off)
{
    FIL      fp;
    int      written_len = 0;
    uint32_t size        = 0;
    int      val;
    if(data == NULL || len <= 0 || off < 0 || off > FOTA_TARGET_SIZE)
    {
        return -3;
    }

    if(0 != k_mutex_lock(&g_fota_bin_mutex, 50))
    {
        warning_log("No file lock obtained.\n");
        return -4;
    }
    print_log("Lock file [push]:%s.\n", FOTA_FILE_PATH);

    if(FR_OK != f_open(&fp, FOTA_FILE_PATH, FA_OPEN_ALWAYS | FA_WRITE))
    {
        err_log("FAIL TO LOAD FILE : %s\n", FOTA_FILE_PATH);
        val = -1;
        goto END;
    }

    f_lseek(&fp, off);
    f_write(&fp, data, len, &written_len);

    if(len != written_len)
    {
        val = -2;
        err_log("WRITE LEN ERROR.\n");
        goto END;
    }

    size = f_size(&fp);
    val  = (int)size;
    print_log("%s size is [%lu]\n", FOTA_FILE_PATH, size);

END:
    f_close(&fp);
    k_mutex_unlock(&g_fota_bin_mutex);
    print_log("UnLock file [push]:%s.\n", FOTA_FILE_PATH);
    print_log("Write [ %d / %d ] bytes at the %s file off location [ %d ].\n", written_len, len,
              FOTA_FILE_PATH, off);

    return val;
}

int getFotaFileSize(void)
{
    FRESULT fr;
    FILINFO fno;

    fr = f_stat(FOTA_FILE_PATH, &fno);
    switch(fr)
    {

        case FR_OK:
            return fno.fsize;
            break;

        case FR_NO_FILE:
            return 0;
            break;

        default:
            return 0;
            break;
    }
}

bool doFotaUpgrade(void)
{
    FIL      fp;
    int      read_len = 0;
    UINT     offset   = 0;
    char*    buffer;
    int      rc;
    uint32_t size = 0;

    //step1. malloc block buffer
    buffer = (uint8_t*)k_malloc(FOTA_FILE_BLOCK_SIZE);
    if(buffer == NULL)
    {
        err_log(">>>>>>Fail to malloc File block buffer.\n");
        return false;
    }

    //step2.open the file
    if(FR_OK != f_open(&fp, FOTA_FILE_PATH, FA_OPEN_ALWAYS | FA_READ))
    {
        err_log(">>>>>>FAIL TO LOAD FILE : %s\n", FOTA_FILE_PATH);
        goto END;
    }

    //step3. View file size
    size = f_size(&fp);
    print_log(">>>>>>>> FOTA file size : [ %u ].\n", size);

    //step3.setup flash device
    if(false == initFlash())
    {
        err_log(">>>>>>FAIL to setup flash device.\n");
        goto END;
    }
    else
    {
        print_log(">>>>>>>> Initialize flash device ok.\n");
    }

    //step4.erase flash
    if(!myFlashErase(FOTA_FLASH_ADDR))
    {
        print_log("erase block1 ok.\n");
    }
    if(!myFlashErase(FOTA_FLASH_ADDR2))
    {
        print_log("erase block2 ok.\n");
    }
    print_log(">>>>>>>> Erase 256k flash ok.\n");

    //step5. copy data from file to flash
    offset = 0;
    do
    {

        //step1. load data from file
        memset(buffer, 1, sizeof(FOTA_FILE_BLOCK_SIZE));
        //print_log("memset.\n");
        if(FR_OK != f_read(&fp, buffer, FOTA_FILE_BLOCK_SIZE, &read_len))
        {
            err_log("FAIL to read fota data.\n");
            goto END;
        }
        //print_log("read fota file size [ %d ].\n",read_len);

        //step2. write bytes to flash
        if(read_len != 0)
        {
            rc = myFlashWrite((uint32_t)(FOTA_FLASH_ADDR + offset), buffer, read_len);
            //print_log("write fota file [ %d ].\n", rc);
        }
        //step3. update offset
        offset += read_len;

        //step4. print log
        print_log(">>>>>LOADED [ %d / %d ]bytes from file. RC state is [%d]\n", read_len, offset,
                  rc);

    } while(read_len > 0);

END:
    f_close(&fp);
    if(buffer)
    {
        k_free(buffer);
    }  //relase buffer

    if(offset)
    {  //only if file copy operation done, reboot and flash file.
        //step3.do upgrade
        print_log("SET REQUEST UPGRADE FLAG\n");
        boot_request_upgrade(0);

        //step4.umount file system
        print_log("Unmount file system.\n");

        f_mount(0, "", 1);  // unmount fats

        //step5.reboot system
        print_log("Reboot system.\n");
        sys_reboot(0);
    }

    return true;
}

int fotaSolidify(void)
{
    int val = -1;

    /* step2. Mark the runing pack okay */
    val = bootfm_set_confirmed();

    if(0 == val)
    {
        print_log("VVVVVVVV Set myself okey. :-) VVVVVVVV\n");
    }
    else
    {
        print_log("XXXXXXXX Set myself FAILED. :-( XXXXXXXX\n");
    }

END:
    return val;
}

bool saveFotaCFG(fotaConfig_t* p)
{
    bool val         = false;
    int  written_len = 0;
    FIL  fp;

    if(!p)
    {
        return false;
    }

    if(FR_OK != f_open(&fp, FOTA_CFG_PATH, FA_OPEN_ALWAYS | FA_WRITE))
    {
        err_log("FAIL TO OPEN FILE: %s\n", FOTA_CFG_PATH);
        val = false;
        goto END;
    }

    f_write(&fp, p, sizeof(fotaConfig_t), &written_len);

    if(written_len != sizeof(fotaConfig_t))
    {
        err_log("@@@@@@save fota start cfg ERROR! save [%d] bytes.\n", written_len);
        val = false;
        goto END;
    }
    else
    {
        val = true;
        print_log("@@@@@@save fota start cfg. save [%d] bytes.\n", written_len);
    }

END:
    f_close(&fp);

    return val;
}

bool loadFotaCFG(fotaConfig_t* p)
{
    bool val      = false;
    int  read_len = 0;
    FIL  fp;

    if(!p)
    {
        return false;
    }

    if(FR_OK != f_open(&fp, FOTA_CFG_PATH, FA_OPEN_ALWAYS | FA_READ))
    {
        err_log(">>>>>>FAIL TO open FILE : %s\n", FOTA_CFG_PATH);
        val = false;
        goto END;
    }

    if(FR_OK != f_read(&fp, p, sizeof(fotaConfig_t), &read_len))
    {
        err_log("FAIL to read fota cfg.\n");
        val = false;
        goto END;
    }

    if(read_len != sizeof(fotaConfig_t))
    {
        val = false;
        err_log("READ fota start cfg error! read [%d] bytes.\n", read_len);
    }
    else
    {
        val = true;
        print_log("Read [%d] bytes to fota start cfg.\n", read_len);
    }

END:
    f_close(&fp);

    return val;
}

bool updateFotaCFG(uint8_t state)
{
    fotaConfig_t cfg;

    memset(&cfg, 0, sizeof(cfg));
    if(!loadFotaCFG(&cfg))
    {
        serverSendLog("FFFFFFFF load fota config FAILED. FFFFFFFF");
    }
    cfg.status = state;
    if(saveFotaCFG(&cfg))
    {
        print_log("SSSSSSSS save fota config ok. SSSSSSSS\n");
        serverSendLog("SSSSSSSS save fota config ok. SSSSSSSS");
        return true;
    }
    else
    {
        print_log("FFFFFFFF save fota config FAILED. FFFFFFFF\n");
        serverSendLog("FFFFFFFF save fota config FAILED. FFFFFFFF");
        return false;
    }
}

void deleteFotaCFG(void)
{
    FIL      fp;
    uint32_t size;
    if(FR_OK != f_open(&fp, FOTA_CFG_PATH, FA_CREATE_ALWAYS | FA_WRITE))
    {
        err_log("FAIL TO LOAD FILE : %s\n", FOTA_CFG_PATH);
    }
    size = f_size(&fp);
    f_close(&fp);
    f_unlink(FOTA_CFG_PATH);
    print_log("File :%s size:%u.\n", FOTA_CFG_PATH, size);
}

bool getFOTAfileMd5(md5_byte_t* outmd5)
{
    if(NULL == outmd5)
    {
        return false;
    }

    if(0 != k_mutex_lock(&g_fota_bin_mutex, 100))
    {
        warning_log("No get file mutex.\n");
        return false;
    }
    print_log("Lock file [get md5]:%s.\n", FOTA_FILE_PATH);
    md5File(FOTA_FILE_PATH, outmd5);
    k_mutex_unlock(&g_fota_bin_mutex);
    print_log("UnLock file [get md5]:%s.\n", FOTA_FILE_PATH);
    return true;
}