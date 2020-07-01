#include "fota_upgrade_thread.h"
#include "fota.h"
#include "my_flash.h"
#include "my_misc.h"
#include "smart_link_version.h"
#include "my_md5.h"
#include "server_interface.h"
#include "thread_led.h"
#include "thread_wdt.h"
#include "message_center_thread.h"
#include "config.h"
#include <string.h>

fotaConfig_t g_fota;
#define FOTA_LINE_LEN 1024
enum
{
    FOTA_NONE          = 0,
    FOTA_START_CMD     = 1,
    FOTA_STOP_CMD      = 2,
    FOTA_SAVE_LINE_CMD = 3,
    FOTA_SAVE_LINE_ACK = 4,
    FOTA_STOP_ACK      = 5,
    FOTA_START_ACK     = 6
};
static struct fota_mem_cache_s
{
    uint8_t buffer[FOTA_LINE_LEN];
    uint32_t pos;
    int32_t  ret;
    volatile uint8_t cmd;
}g_fota_cache;


#define FOTA_THREAD_SIZE 2048
K_THREAD_STACK_DEFINE(g_fotaThreadStack, FOTA_THREAD_SIZE);
static struct k_thread g_fotaThread;
static k_tid_t         g_fotaThreadId = 0;

static volatile uint32_t g_lock_last_time = 0;
static volatile uint32_t g_recv_len       = 0;
static volatile uint32_t g_last_recv_ts   = 0;
#define SAFE_INTERVAL (60 * 1000)      //60 sec
#define CONTINUE_INTERVAL (60 * 1000)  //60 sec

static bool startFOTAthread(void);
static void stopFOTAthread(void);
static void fotaThreadEntry(void);
static void checkWholeFotaFile(void);
static void triggerContinueDownload(uint32_t ts);
static void updateLockTime();
static int  getLockTime();

/*****************************************************/
static void operatorFotaStart(void);
static void operatorFotaStop(void);
static void operatorFotaSaveLine(void);
/*****************************************************/

static void fotaThreadEntry(void)
{
    uint32_t current_ts;
    uint8_t  auth = gSysconfig.authType;
    g_recv_len = getFotaFileSize();
    print_log("Receive FOTA bin size : [ %d ]\n",g_recv_len);

    /* step set fota led on*/
    thread_led_stat_set(LED_FOTA_SIGNAL, 1);

    while(1)
    {
        current_ts = k_uptime_get_32();

        updateLockTime();

        triggerContinueDownload(current_ts);

        checkWholeFotaFile();

        /* check and reboot */
        if(DONE == g_fota.status)
        {
            if(0 == auth)
            {
                thread_wdt_stop();
            }
            else
            {
                int interval = getLockTime();
                if(interval > 0 && interval > SAFE_INTERVAL)
                {
                    print_log("interval time:%d.................................\n", interval);
                    print_log("########## reboot ##########\n");
                    thread_wdt_stop();
                }
                else
                {
                    print_log("Delay lock car. :-)\n");
                    print_log("interval time:%d..................................\n", interval);
                }
            }
        }

        k_sleep(1000);
    }

    /* step set fota led off*/
    thread_led_stat_set(LED_FOTA_SIGNAL, 0);
}

static void triggerContinueDownload(uint32_t ts)
{
    if(false == isLoginOk() || g_fota.status != DOWNLOADING)
    {
        return;
    }
    if(ts > g_last_recv_ts && ts - g_last_recv_ts > CONTINUE_INTERVAL)
    {
        int val = getFotaFileSize();
        if(0 == val)
        {
            serverSendFotaStartACK(0);  //enable download
            print_log("Start download file offset:[0].\n");
        }
        else
        {
            serverSendFotaLineACK((uint32_t)val);
            print_log("Continue download file offset:[%u].\n", val);
        }
        g_last_recv_ts = k_uptime_get_32();
    }
}

static void checkWholeFotaFile(void)
{
    uint8_t curr_md5[MD5_SIZE];
    if(DOWNLOADING == g_fota.status && g_recv_len == g_fota.fotaSize)
    {
        print_log("FOTA file size right!\n");
        serverSendLog("FOTA file size right!");
        //step0 check md5.md5 not equal return
        if(md5File(FOTA_FILE_PATH, curr_md5) != 0)
        {
            err_log("Not get fota file md5.\n");
            return;
        }
        serverSendLog("During file md5!");
        print_log("During file md5.\n");
        if(0 != memcmp(curr_md5, g_fota.md5, sizeof(curr_md5)))
        {
            err_log("md5 check error.\n");

            print_log("\nServer computing MD5 is:\n");
            for(int i = 0; i < sizeof(curr_md5); i++)
            {
                printk("0x%02x ", g_fota.md5[i]);
            }
            printk("\n");
            printk("Local computing MD5 is:\n");
            for(int i = 0; i < sizeof(curr_md5); i++)
            {
                printk("0x%02x ", curr_md5[i]);
            }
            printk("\n");
            serverSendFotaEvent(FOTA_MD5_FILE_CHECK_ERROR);  //download ok,but check md5 error.
            deleteFotaFile();                                //delete bad file.
            g_recv_len = 0;
            print_log("========== Tell server check MD5 failed\n");
        }
        else
        {
            print_log("Md5 check ok.\n");
            serverSendLog("Md5 check ok.");
#if 1
            print_log("\nThe server send MD5 is:\n");
            for(int i = 0; i < sizeof(curr_md5); i++)
            {
                printk("0x%02x ", g_fota.md5[i]);
            }
            printk("\n");
            printk("Local computing MD5 is:\n");
            for(int i = 0; i < sizeof(curr_md5); i++)
            {
                printk("0x%02x ", curr_md5[i]);
            }
            printk("\n");
#endif
            //step1. Tell the server that the file download was successful.
            serverSendFotaEvent(FOTA_DOWNLOAD_CHECKMD5_OK);  //0,download and check ok
            print_log("Tell server that the file download was successful.\n");
            g_fota.status = DONE;
            updateFotaCFG(DONE);
            k_sleep(500);
            serverSendLog("Fota bin download DONE.\n");
        }
    }
}

static bool startFOTAthread()
{
    bool ret;

    if(0 != g_fotaThreadId)
    {
        return false;
    }

    print_log("start FOTA thread\n");
    g_fotaThreadId =
        k_thread_create(&g_fotaThread, g_fotaThreadStack, FOTA_THREAD_SIZE,
                        (k_thread_entry_t)fotaThreadEntry, NULL, NULL, NULL, K_PRIO_COOP(12), 0, 0);
    if(g_fotaThreadId != 0)
    {
        ret = true;
        print_log("Create FOTA THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_fotaThreadId,
                  g_fotaThreadStack, FOTA_THREAD_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create Thread FOTA Failed.\n\n");
    }

    return ret;
}

static void stopFOTAthread(void)
{
    if(0 != g_fotaThreadId)
    {
        k_thread_abort(g_fotaThreadId);
        g_fotaThreadId = 0;
    }
    deleteFotaCFG();
    deleteFotaFile();

    /* step set fota led off*/
    thread_led_stat_set(LED_FOTA_SIGNAL, 0);
}

static void updateLockTime()
{
    if(false == carIsLock())
    {
        g_lock_last_time = k_uptime_get_32();
        print_log("UUUUUUUU unlock.\n");
    }
    else
    {
        print_log("LLLLLLLL lock.\n");
    }
}

static int getLockTime()
{
    updateLockTime();
    uint32_t curr_ts = k_uptime_get_32();

    return (curr_ts - g_lock_last_time);
}

void checkFOTA(void)
{
    initFlash();

    if(false == loadFotaCFG(&g_fota))
    {
        k_sleep(100);
        /*try to load*/
        if(false == loadFotaCFG(&g_fota))
        {
            warning_log("load fota cfg failed.\n");
            return;
        }
    }

    if(DOWNLOADING == g_fota.status)
    {
        startFOTAthread();
    }

    //2. done to flash
    if(DONE == g_fota.status)
    {
        g_fota.status = SOLIDIFY;
        saveFotaCFG(&g_fota);
        doFotaUpgrade();
    }

    //3. solidify itself
    print_log("check FOTA: cfg:[%d.%d.%d.%d]   curr:[%d.%d.%d.%d]\n",g_fota.major,g_fota.minor,g_fota.tiny,g_fota.path,
                SOFT_VERSION_MAJOR,SOFT_VERSION_MINOR,SOFT_VERSION_TINY,SOFT_VERSION_PATCH);
    if(g_fota.major == SOFT_VERSION_MAJOR && g_fota.minor == SOFT_VERSION_MINOR
       && g_fota.tiny == SOFT_VERSION_TINY && g_fota.path == SOFT_VERSION_PATCH)
    {
        if(0 != fotaSolidify())
        { /* try to solidify */
            if(0 != fotaSolidify())
            {
                warning_log("XXXXXXXX Self Soldify FAILED :>( XXXXXXXX\n");
            }
            else
            {
                print_log("VVVVVVVV  Self Soldify OK! :-) VVVVVVVV \n");
                deleteFotaCFG();
                deleteFotaFile();
            }
        }
        else
        {
            print_log("VVVVVVVV  Self Soldify OK! :-) VVVVVVVV \n");
            deleteFotaCFG();
            deleteFotaFile();
        }
    }
    else
    {
        print_log("No bin to solidify.\n");
    }
}

#define FOTA_START_LEN 28
void doFotaStart(uint8_t* payload_data, uint32_t payload_len)
{
    //step1.Check parameter
    if(!payload_data)
    {
        return;
    }
    if(payload_len != FOTA_START_LEN)
    {
        serverSendFotaStartACK(1);  //disable download
        char buf[64] = { 0 };
        sprintf(buf, "Tell server that 0x301 send payload_len:[%d]", payload_len);
        serverSendLog(buf);
        return;
    }

    //step3.reset wtire fota file failed count
    //g_fota_write_failed_count = 0;

    //step1. save file md5 & file_size and software version
    memcpy((uint8_t*)&g_fota.md5, (uint8_t*)&payload_data[4], sizeof(g_fota.md5));

    print_log("Server send Fota file MD5:\n");
    for(int i = 0; i < sizeof(g_fota.md5); i++)
    {
        printk("0x%02x ", g_fota.md5[i]);
    }
    print_log("\n");

    //step2. update target_size;
    g_fota.fotaSize = *(unsigned int*)&payload_data[0];

    print_log("<<<<<<<< Payload len     : [ %u ]\n", payload_len);
    print_log("<<<<<<<< Fota line size  : [ %d ]\n", g_fota.fotaSize);
    print_log("<<<<<<<< Device Version  : [ %u.%u.%u.%u ]\n", *(uint8_t*)&payload_data[20],
              *(uint8_t*)&payload_data[21], *(uint8_t*)&payload_data[22],
              *(uint8_t*)&payload_data[23]);
    print_log("<<<<<<<< Software Version: [ %u.%u.%u.%u ]\n", *(uint8_t*)&payload_data[24],
              *(uint8_t*)&payload_data[25], *(uint8_t*)&payload_data[26],
              *(uint8_t*)&payload_data[27]);

    /* fill version */
    g_fota.major = *(uint8_t*)&payload_data[24];
    g_fota.minor = *(uint8_t*)&payload_data[25];
    g_fota.tiny  = *(uint8_t*)&payload_data[26];
    g_fota.path  = *(uint8_t*)&payload_data[27];

    g_fota.status = DOWNLOADING;

    /* tell file thread fota start */
    g_fota_cache.cmd = FOTA_START_CMD;
    // print_log("CCCCCCCCCCCCCCCCCCCC Tell file thread FOTA_START_CMD.\n");
}

/* used for file thread */
void checkFotaOperate(void)
{
    if(FOTA_NONE == g_fota_cache.cmd ||
        (FOTA_START_CMD != g_fota_cache.cmd &&
        FOTA_STOP_CMD != g_fota_cache.cmd &&
        FOTA_SAVE_LINE_CMD != g_fota_cache.cmd))
    {
        return ;
    }

    switch (g_fota_cache.cmd)
    {
        case FOTA_START_CMD:
            print_log("^^^^^^^^^^^^^^^^^^ FOTA start CMD.\n");
            operatorFotaStart();
            break;
        case FOTA_STOP_CMD:
            print_log("^^^^^^^^^^^^^^^^^^ FOTA stop CMD.\n");
            operatorFotaStop();
            break;
        case FOTA_SAVE_LINE_CMD:
            print_log("^^^^^^^^^^^^^^^^^^ FOTA save line CMD.\n");
            operatorFotaSaveLine();
            break;
        default:
            warning_log("file thread FOTA issue.\n");
            break;
    }
}

/* used for message center thread */
void checkFotaOperateACK(void)
{
    if(FOTA_NONE == g_fota_cache.cmd ||
        (FOTA_STOP_ACK != g_fota_cache.cmd 
        && FOTA_START_ACK != g_fota_cache.cmd
        && FOTA_SAVE_LINE_ACK != g_fota_cache.cmd))
    {
        return ;
    }

    switch (g_fota_cache.cmd)
    {
        case FOTA_START_ACK:
            {   
                // print_log("^^^^^^^^^^^^^ FOTA start ACK.\n");
                serverSendFotaStartACK(0);  //enable download
                g_last_recv_ts = k_uptime_get_32();
            }
            break;
        case FOTA_STOP_ACK:
            {
                // print_log("^^^^^^^^^^^^^^ FOTA stop ACK.\n");
                //Last step. tell server stop download successful
                serverSendFotaStopACK(0);
                print_log("======== Tell server that stop to download fota file OK. ========\n");
            }
            break;
        case FOTA_SAVE_LINE_ACK:
            {
                // print_log("^^^^^^^^^^^^^^ FOTA save line ACK.\n");
                if(g_fota_cache.ret > 0)
                {
                    serverSendFotaLineACK((uint32_t)g_fota_cache.ret);
                    // print_log("CCCCCCCCCCCCCCCCCC  send fota size to server:[%d]\n", g_fota_cache.ret);
                }
                else if(g_fota_cache.ret < 0)
                {
                    if(-1 == g_fota_cache.ret)
                    {  //TODO server_device_events(DEVICE_EVENTS_OPEN_ERROR,DEVICE_MESSAGE_FOTA_FILE);
                    }
                    if(-2 == g_fota_cache.ret)
                    {  //TODO server_device_events(DEVICE_EVENTS_WRITE_ERROR,DEVICE_MESSAGE_FOTA_FILE);
                    }
                    serverSendFotaEvent(FOTA_OPEN_FILE_ERROR);  //can't open fota file
                    print_log("TEll server fota file open failed.\n");
                }
                else
                {
                    print_log("CCCCCCCCCCCCCCC fota file size is [0] :-(\n");
                }
                
            }
            break;
        default:
            break;
    }

    g_fota_cache.cmd = FOTA_NONE;
}

void pushFotaBytes(uint8_t* payload_data, uint32_t payload_len)
{
    uint32_t block_len = 0;
    uint32_t off       = *(uint32_t*)&payload_data[16];
    block_len          = payload_len - 20;
    unsigned char local_line_md5[16];

    g_last_recv_ts = k_uptime_get_32();  //update last recv ts

    memcpy(local_line_md5, (unsigned char*)&payload_data[0], sizeof(local_line_md5));
    uint32_t val = 0;

    //step0. check of MD5
    md5_byte_t local_line_digest[16] = { 0 };
    md5CheckBlock(&payload_data[20], block_len, local_line_digest);
    if(0 != memcmp(local_line_digest, local_line_md5, sizeof(local_line_digest)))
    {
        serverSendFotaEvent(FOTA_MD5_LINE_CHECK_ERROR);  // tell server that the md5 line check failed.
        val = getFotaFileSize();
        if(0 == val)
        {
            //TODO server_device_events(DEVICE_EVENTS_WRITE_ERROR,DEVICE_MESSAGE_FOTA_FILE); // tell server that write fota file error
            serverSendFotaStartACK(0);  //tell server that the file is enable loaded
        }
        else
        {
            serverSendFotaLineACK(val);
            print_log("val:[ %lu ]\n", val);
        }
        print_log("======== ERROR: MD5 line check failed.\n");

#if 0
        // Keep this code for debugging
        print_log("\nServer line MD5 is:\n");
        for(int i=0;i<sizeof(local_line_md5);i++){
        printk("0x%02x ", local_line_md5[i]);
        }
        printk("\n");
        printk("Local line MD5 is:\n");
        for(int i=0;i<sizeof(local_line_digest);i++){
        printk("0x%02x ", local_line_digest[i]);
        }
        printk("\n");
        for(int i = 0; i<block_len;i++){
        printk("0x%02x ", payload_data[20+i]);
        }
        printk("\n");
#endif
        return;
    }

    //serverSendFotaEvent(FOTA_MD5_LINE_CHECK_OK);  // tell server that the md5 line check pass.
    print_log("Md5 check line pass.\n");

    int copy_len = block_len > sizeof(g_fota_cache.buffer) ? sizeof(g_fota_cache.buffer) : block_len;
    memcpy(g_fota_cache.buffer, (char*)&payload_data[20], copy_len);
    g_fota_cache.pos = off;
    g_fota_cache.ret = copy_len;
    g_fota_cache.cmd = FOTA_SAVE_LINE_CMD;
}

void doFotaSolidifyFirm(void)
{
    int val = fotaSolidify();
    serverSendFotaSolidify((uint32_t)val);
}

void doFotaStop(void)
{
    memset(&g_fota, 0, sizeof(g_fota));

    //step1. set led for fota stop and fota mode
    thread_led_stat_set(LED_FOTA_SIGNAL, 0);

    //step2. reset target size
    g_recv_len = 0;

    g_fota_cache.cmd = FOTA_STOP_CMD;
}

static void operatorFotaStart(void)
{
    if(saveFotaCFG(&g_fota))
    {
        print_log("SSSSSSSS save fota start state ok. SSSSSSSS\n");
        serverSendLog("SSSSSSSS save fota start state ok. SSSSSSSS");
    }
    else
    {
        print_log("FFFFFFFF save fota start state FAILED. FFFFFFFF\n");
        serverSendLog("FFFFFFFF save fota start state FAILED. FFFFFFFF");
    }

    deleteFotaFile();  // delete firmware file
    serverSendLog("DDDDDDDD delete fota file.bin DDDDDDDD");

    startFOTAthread();

    g_fota_cache.cmd = FOTA_START_ACK;
    // print_log("CCCCCCCCCCCCCCCCCCCCCCCCCCC tell message FOTA START ACK.\n");
}

static void operatorFotaStop(void)
{
    stopFOTAthread();

    //step3. remove fota file & cfg
    deleteFotaFile();
    serverSendLog("DDDDDDDD delete fota file.bin DDDDDDDD");
    deleteFotaCFG();
    serverSendLog("DDDDDDDD delete fota config file DDDDDDDD");

    //step4. reset fota downloading
    saveFotaCFG(&g_fota);

    g_fota_cache.cmd = FOTA_STOP_ACK;
    // print_log("CCCCCCCCCCCCCCCCCCCCCCCC Tell message FOTA STOP ACK.\n");
}

static void operatorFotaSaveLine(void)
{
    //step2. push bytes to local file
    int val = pushBytesToFotaFile(g_fota_cache.buffer, g_fota_cache.ret, g_fota_cache.pos);
    g_fota_cache.ret = val;
    g_recv_len = val;  //update current recv lenth

    g_fota_cache.cmd = FOTA_SAVE_LINE_ACK;
    // print_log("CCCCCCCCCCCCCCCCCCCCCCCC Tell message FOTA SAVE LINE ops:[%d] \n",val);
}