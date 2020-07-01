#include "cache_box.h"
#include "cache_opt.h"
#include "my_misc.h"
#include <string.h>

#define BOX_LOOP_COUNT 7
static uint8_t can_name[BOX_LOOP_COUNT][5] = { "can7","can1", "can2", "can3", 
                                               "can4","can5", "can6"};
static uint8_t msg_name[BOX_LOOP_COUNT][9] = { "message7","message1", "message2", "message3",
                                               "message4","message5", "message6" };
static uint8_t g_curr_pos = 0;

static void setCurrPos(uint8_t pos);

static int getCanFilePos(void)
{
    bool curr_dirty = true;

    for(int i = g_curr_pos; curr_dirty || g_curr_pos != i; 0==i ? i=6:--i)
    {
        if(0 < getFileSize(can_name[i]))
        {
            return i;
        }
        else
        {
            if(i==g_curr_pos)
            {
                curr_dirty = false;
            }
        }
    }
    return -1;
}

static int getMsgFilePos(void)
{
    bool curr_dirty = true;

    for(int i = g_curr_pos; curr_dirty || g_curr_pos != i; 0==i ? i=6 :--i)
    {
        if(0 < getFileSize(msg_name[i]))
        {
            return i;
        }
        else
        {
            if(i==g_curr_pos)
            {
                curr_dirty = false;
            }
        }
        
    }
    return -1;
}

bool pushCanToCache(uint8_t* data, int len, uint8_t day, uint8_t week)
{
    bool    ret;
    int     file_day;
    uint8_t pos = week%7;
    uint8_t cur_file_name[1]={1}; //nop

    if(!data || len <= 0 || day < 1 || day > 31 || week > 7)
    {
        err_log("Para Error day:%d  week:%d\n");
        return false;
    }

    /* update current pos */
    setCurrPos(pos);

    strncpy(cur_file_name, cur_file_name,sizeof(cur_file_name)); //nop
    file_day = getFileLastDate(can_name[pos]);
    // print_log("can Head:[0x%02x]\n",data[0]);
    printk("file %s day is %d(cur day :%d)\n", can_name[pos], file_day, day);
    if(file_day != day)
    {
        deleteCacheFile(can_name[pos]);
    }
    printk("push to file %s\n", can_name[pos]);
    ret = pushToCacheFixed(can_name[pos], data, len);
    if(false == ret)
    {
        return ret;
    }
    ret = updateFileLastDate(can_name[pos], day);

    return ret;
}

bool popCanFromCache(uint8_t* data, int len)
{
    static int pos = -1;
    bool       ret;

    do
    {
        if(-1 == pos)
        {
            pos = getCanFilePos();
        }

        if(-1 != pos)
        {
            printk("pop from file %s\n", can_name[pos]);
            ret = popFromCacheFixed(can_name[pos], data, len);
            if(false == ret)
            {
                deleteCacheFile(can_name[pos]);
                pos = -1;
            }
            else
            {
                return ret;
            }
        }
        else
        {
            return false;
        }
        k_sleep(1);
    } while(1);
}

bool pushMessageToCache(uint8_t* data, int len, uint8_t day, uint8_t week)
{
    bool    ret;
    int     file_day;
    uint8_t pos = week%7;
    uint8_t cur_file_name[9]={0};

    if(!data || len <= 0 || day < 1 || day > 31 || week > 7)
    {
        err_log("Para Error day:[%d],week:[%d].\n",day,week);
        return false;
    }

    /* update curr pos */
    setCurrPos(pos);

    strncpy(cur_file_name, msg_name[pos],sizeof(cur_file_name));
    file_day = getFileLastDate(cur_file_name);
    if(file_day != day)
    {
        deleteCacheFile(cur_file_name);
    }
    print_log("file %s day is %d(cur day :%d)\n", cur_file_name, file_day, day);
    ret = pushToCacheUnfixed(cur_file_name, data, len);
    if(false == ret)
    {
        return ret;
    }
    ret = updateFileLastDate(cur_file_name, day);

    return ret;
}

int popMessageFromCache(uint8_t* data, int len)
{
    static int pos = -1;
    int        ret;

    do
    {
        if(-1 == pos)
        {
            pos = getMsgFilePos();
        }

        if(-1 != pos)
        {
            print_log("pop from pos:[%d]\n",pos);
            ret = popFromCacheUnfixed(msg_name[pos], data, len);
            print_log("ret:%d\n", ret);
            if(ret <= 0)
            {
                deleteCacheFile(msg_name[pos]);
                pos = -1;
            }
            else
            {
                return ret;
            }
        }
        else
        {
            return 0;
        }
        k_sleep(1);
    } while(1);
}

void deleteAllCache(void)
{
    for(int i = 1; i <= 7; i++)
    {
        deleteCacheFile(can_name[i]);
        deleteCacheFile(msg_name[i]);
    }
    print_log("Delete ALL Cache.\n");
}

static void setCurrPos(uint8_t pos)
{
    if (0 <= pos && pos <= BOX_LOOP_COUNT - 1)
    {
        g_curr_pos = pos;
    }
}