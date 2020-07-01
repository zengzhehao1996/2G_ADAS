#include "my_file.h"
#include "my_misc.h"
#include "ff.h"

FATFS FatFs;

static bool g_fs_is_mounted = false;
static void setFsMount(bool val);

static void test_fatfs() {
    FIL file_;
    int ret;
    const char *test_buf = "hello word";
    char test_read[16] = { 0 };
    const char *test_path = "123.txt";
    //write somthing to test file system
    ret = f_open(&file_, test_path, FA_CREATE_ALWAYS | FA_READ | FA_WRITE);
    if (ret != FR_OK) {
        print_log("faild to open 123.txt \n");
        goto MK_FS;
    }
    f_puts(test_buf, &file_);
    f_close(&file_);
    //read somthing to test file system
    f_open(&file_, test_path, FA_READ);
    if (ret != FR_OK) {
        print_log("faild to open 123.txt \n");
        goto MK_FS;
    }
    f_gets(test_read, sizeof(test_read), &file_);
    f_close(&file_);
    //test filesystem ,if error mkfs it
    if (memcmp(test_read, test_buf, strlen(test_buf)) != 0) {
    MK_FS:
        print_log("fag_sys_var.sim868_alivetfs failed!! mkfs it...\n");
        if (f_mkfs("", 0, 0) == FR_OK) {
        print_log("Making fatfs on emmc ok!\n");
    #if 0
    f_mount(0, "", 1);
    while (1)
    ;
    #endif

        } else {
        print_log("Making fatfs on emmc fail!\n");
        setFsMount(false);
        return;
        }
    } else {
        print_log(" test fatfs ok !\n");
        setFsMount(true);
    }
}

void mountFS(void)
{
    //step1. emmc file system
    if (f_mount(&FatFs, "", 1) == FR_OK) 
    {
        print_log("====== Mount fatfs ok! ======\n");
        //test fatfs
        test_fatfs();

    } else {
        print_log("==== Mount fatfs fail!, try to make fatfs system on the emmc, please wait... =====\n");
        if (f_mkfs("", 0, 0) == FR_OK) {
            printk("Making fatfs on emmc ok!\n");
            setFsMount(true);
        } else {
            printk("Making fatfs on emmc fail!\n");
            setFsMount(false);
        }
    }
}

void onlyMountFs(void)
{
    if (f_mount(&FatFs, "", 1) == FR_OK) 
    {
        print_log("====== ReMount fatfs ok! ======\n");
        setFsMount(true);
    }
    else
    {
        err_log("XXXXXX ReMount fatfs FAILED! XXXXXX\n");
        setFsMount(false);
    }
}

void unmountFs(void)
{
    uint8_t ret=0;
    ret=f_mount(NULL, "", 1); //unmount fs
    print_log("UUUUUUUUUUUUUUUUU   umount Fs ............%d\n",ret);
    setFsMount(false);
    //k_sleep(100);
}

static void setFsMount(bool val)
{
    g_fs_is_mounted = val;
}

bool isFatfsMounted(void)
{
    return g_fs_is_mounted;
}

bool initFile(classFile_t*ps)
{
    if(!ps )
    {
        err_log("paragma ERROR.\n");
        return false;
    }  
    k_mutex_init(&(ps->mutex));
    return true;
}

bool writeDataToFile(classFile_t* ps, void *pdata, int len)
{
    FIL flp;
    bool val;
    int writeLen=0;

    if(NULL == ps || NULL == ps->filename || NULL == pdata || len<=0)
    {
        err_log("paragma ERROR ps:[%p],filename:[%s],pdata:[%p],len:[%d].\n",ps,ps->filename,pdata,len);
        return false;
    }

    if(0 != k_mutex_lock(&(ps->mutex), ps->timeOut)){
        err_log("Don't get file %s mutex!\n", ps->filename);
        return false;
    }

    k_sched_lock();

    if(FR_OK!=f_open(&flp,ps->filename,FA_CREATE_ALWAYS|FA_WRITE)){
        err_log("Can't open file:%s.\n",ps->filename);
        val = false;
        goto END;
    }
    if(FR_OK!=f_write(&flp,pdata,len,&writeLen)){
        val = false;
        goto END;
    }else{
        val = true;
        print_log("write file %s Ok!\n",ps->filename);
    }
    END:

    f_close(&flp);
    k_sched_unlock();
    k_mutex_unlock(&(ps->mutex));
    return val;
}

int  readDataFromFile(classFile_t* ps, void *pdata, int len)
{
    FIL flp;
    int  val;
    uint32_t read_size=0;

    if(NULL == ps || NULL == ps->filename || NULL == pdata || len<=0)
    {
        err_log("paragma ERROR.\n");
        return -1;
    }

    if(0 != k_mutex_lock(&(ps->mutex), ps->timeOut))
    {
        err_log("Don't get file %s mutex!\n", ps->filename);
        return -2;
    }

    k_sched_lock();
    if(FR_OK!=f_open(&flp,ps->filename,FA_READ))
    {
        val = -3;
        err_log("Can't open fileXXXXXXXXXXXXXXx:%s, open::%d.\n",ps->filename,val);
        goto END;
    }

    if(FR_OK!=f_read(&flp,pdata,len,&read_size))
    {
        val = -4;
        err_log("f read error file: %s\n",ps->filename);
        goto END;
    }
    else
    {
        val = read_size;
        print_log("vvvvvvvv....................read %d bytes from file %s.>>>>>>>>>>>\n", read_size, ps->filename);
    }


    END:
    f_close(&flp);
    k_sched_unlock();
    k_mutex_unlock(&(ps->mutex));
    
    if(val)
    {
        print_log("read %d bytes from file %s.\n", read_size, ps->filename);
    }

    return val;
}

void deleteFile(classFile_t* ps)
{
    FIL flp;

    if(NULL == ps)
    {
        err_log("paragma ERROR.\n");
        return;
    }

    if(FR_OK!=f_open(&flp,ps->filename,FA_CREATE_ALWAYS|FA_WRITE|FA_READ))
    {
        err_log("Can't delete file:%s.\n",ps->filename);
        return;
    }

    f_close(&flp);

    f_unlink(ps->filename);

    print_log("DELETE file %s\n", ps->filename);
    return;
}
