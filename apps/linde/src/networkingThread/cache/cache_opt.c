#include "cache_opt.h"
#include "my_misc.h"
#include "ff.h"

#define MAX_CACHE_SIZE (1 * 1024 * 1024)

bool pushToCacheFixed(uint8_t* filename, uint8_t* data, int len)
{
    FIL      flp;
    bool     val;
    uint32_t size       = 0;
    uint32_t write_size = 0;

    if(!filename || !data || len <= 0)
    {
        return false;
    }

    if(FR_OK != f_open(&flp, filename, FA_OPEN_ALWAYS | FA_WRITE | FA_READ))
    {
        err_log("Can't open file:%s.\n", filename);
        return false;
    }
    size = f_size(&flp);
    if(size + len > MAX_CACHE_SIZE)
    {
        val = false;
        goto END;
    }

    f_lseek(&flp, size);
    if(FR_OK == f_write(&flp, data, len, &write_size))
    {
        val = true;
        print_log("Push [%d] bytes to file %s.\n", write_size, filename);
    }
    else
    {
        val = false;
    }

END:
    f_close(&flp);
    return val;
}
bool popFromCacheFixed(uint8_t* filename, uint8_t* data, int len)
{
    FIL      flp;
    bool     val;
    uint32_t ops       = 0;
    uint32_t size      = 0;
    uint32_t read_size = 0;

    if(!filename || !data || len <= 0)
    {
        return false;
    }

    if(FR_OK != f_open(&flp, filename, FA_OPEN_ALWAYS | FA_WRITE | FA_READ))
    {
        err_log("Can't open file:%s.\n", filename);
        val = false;
        goto END;
    }

    size = f_size(&flp);
    print_log("%s file size:%d\n", filename, size);
    if(size < len)
    {
        val = false;
        goto END;
    }
    else
    {
        ops = size - len;
    }

    f_lseek(&flp, ops);
    if(FR_OK != f_read(&flp, data, len, &read_size))
    {
        val = false;
        goto END;
    }
    else
    {
        if(read_size != len)
        {
            val = false;
            goto END;
        }
        else
        {
            val = true;
        }
    }

    f_lseek(&flp, ops);
    if(FR_OK != f_truncate(&flp))
    {
        err_log("Truncate file %s Failed!\n", filename);
    }

END:
    f_close(&flp);
    if(val)
    {
        print_log("Pop [%d]Bytes from file %s.\n", read_size, filename);
    }

    return val;
}

bool pushToCacheUnfixed(uint8_t* filename, uint8_t* data, int len)
{
    FIL      flp;
    bool     val;
    uint32_t size       = 0;
    uint32_t write_size = 0;

    if(!filename || !data || len <= 0)
    {
        return false;
    }

    if(FR_OK != f_open(&flp, filename, FA_OPEN_ALWAYS | FA_WRITE | FA_READ))
    {
        err_log("Can't open file:%s.\n", filename);
        return false;
    }
    size = f_size(&flp);
    if(size + len > MAX_CACHE_SIZE)
    {
        val = false;
        goto END;
    }

    f_lseek(&flp, size);
    if(FR_OK != f_write(&flp, data, len, &write_size))
    {
        val = false;
        goto END;
    }
    /* write len */
    if(FR_OK == f_write(&flp, &len, sizeof(len), &write_size))
    {
        val = true;
    }
    else
    {
        val = false;
    }
        print_log("push len:%d\n",len);

END:
    f_close(&flp);
    return val;
}

int popFromCacheUnfixed(uint8_t* filename, uint8_t* data, int len)
{
    FIL      flp;
    int      val       = 0;
    uint32_t ops       = 0;
    uint32_t size      = 0;
    int      read_size = 0;
    int      data_len  = 0;

    if(!filename || !data || len <= 0)
    {
        return -1;
    }

    if(FR_OK != f_open(&flp, filename, FA_OPEN_ALWAYS | FA_WRITE | FA_READ))
    {
        err_log("Can't open file:%s.\n", filename);
        val = -1;
        goto END;
    }

    size = f_size(&flp);
    print_log("%s file size:%d\n", filename, size);
    if(size < sizeof(data_len))
    {
        val = -2;
        goto END;
    }
    else
    {
        ops = size - sizeof(data_len);
    }

    f_lseek(&flp, ops);
    print_log("len ops:%d\n",ops);
    if(FR_OK != f_read(&flp, &data_len, sizeof(data_len), NULL))
    {
        val = -3;
        goto END;
    }
    else
    {
        ops = size - (sizeof(data_len) + data_len);
    }
    print_log("data_len :%d\n",data_len);

    if(data_len > len)
    {
        val = -4;
        err_log("data len:[%d] ,read len : [%d]\n",data_len, len);
        goto END;
    }
    
    f_lseek(&flp, ops);
    // print_log("data ops:%d\n",ops);
    if(FR_OK != f_read(&flp, data, data_len, &read_size))
    {
        val = -5;
        goto END;
    }
    else
    {
        if(read_size == data_len)
        {
            val = read_size;
        }
        else
        {
            val = -6;
            goto END;
        }
    }

    f_lseek(&flp, ops);
    if(FR_OK != f_truncate(&flp))
    {
        err_log("Truncate file %s Failed!\n", filename);
    }

END:
    f_close(&flp);
    if(val > 0)
    {
        print_log("Pop [%d]Bytes from file %s.\n", val, filename);
    }

    return val;
}

/* return 0 is file not exist, -1 is error 1-31 is the day */
int getFileLastDate(const uint8_t *filename)
{
    int     day;
    FRESULT fr;
    FILINFO fno;
    fr = f_stat(filename, &fno);
    switch(fr)
    {
        case FR_OK:
            day = fno.fdate & 31;
            break;
        case FR_NO_FILE:
            day = 0;
            break;
        default:
            day = -1;
            break;
    }

    return day;
}

bool updateFileLastDate(uint8_t* filename, int day)
{
    FILINFO fno;

    fno.fdate = (WORD)day;

    if(FR_OK == f_utime(filename, &fno))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/* return ,0 is file not exist, >0 is file size, -1 is error */
int getFileSize(uint8_t* filename)
{
    int     size = 0;
    FRESULT fr;
    FILINFO fno;
    fr = f_stat(filename, &fno);
    switch(fr)
    {
        case FR_OK:
            size = fno.fsize;
            break;
        case FR_NO_FILE:
            size = 0;
            break;
        default:
            size = -1;
            break;
    }

    return size;
}

void deleteCacheFile(uint8_t *filename)
{
    FIL flp;

    if(NULL == filename)
    {
        err_log("paragma ERROR.\n");
        return ;
    }

    if(FR_OK != f_open(&flp, filename, FA_CREATE_ALWAYS | FA_WRITE | FA_READ))
    {
        err_log("Can't delete file:%s.\n", filename);
        return;
    }

    f_close(&flp);

    f_unlink(filename);

    print_log("DELETE file %s\n", filename);
    return;
}
