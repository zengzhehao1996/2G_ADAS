#include "thread_file.h"
#include "file.h"
#include "config.h"
#include "my_misc.h"
#include "hw_power.h"
#include "sempho.h"

#define THREAD_FILE_STACK_SIZE 2048
K_THREAD_STACK_DEFINE(g_thread_file_stack, THREAD_FILE_STACK_SIZE);
struct k_thread g_file_thread;
k_tid_t g_file_thread_id=0;
static void threadFileEntry(void * p);

static uint32_t gLastFileThread;

/* extern function in here define  **************************************/
extern int checkSaveRFIDList(void);
extern int checkSaveConfig2File(void);
extern bool hwPowerDisable(void);
extern void checkOffline(void);
extern void checkFotaOperate(void);
extern void checkFOTA(void);
extern void initLastRfidFile(void);
extern void readLastRfidFile(void);
extern void readPowerOffFile(void);
extern uint8_t readRfidListFile(void);
extern uint8_t writeLastRfidFile (void);
extern void checkTmpReadRFID(void);
extern void checkTmpWriteRFID(void);
extern void deleteTmpRFIDFile(void);
extern void checkSaveSeatTypeConfig2File(void);
extern void loadSpeedLimitConfig(void);
extern void checkSpeedLimitSaveState(void);
/************************************************************************/

bool startThreadFile()
{
  g_file_thread_id = k_thread_create(&g_file_thread, g_thread_file_stack, THREAD_FILE_STACK_SIZE,
                             (k_thread_entry_t) threadFileEntry,
                             NULL, NULL, NULL, K_PRIO_COOP(5), 0, 0);
  if(g_file_thread_id==0){
    print_log("Fail to create file thread.\n");
    return false;
  }
  print_log("Create file thread Id:[ %p ]; Stack:[ %p ]; Size:[ %d ]\n", g_file_thread_id,
            g_thread_file_stack, THREAD_FILE_STACK_SIZE);
  return true;
}

void stopThreadFile()
{
    if(g_file_thread_id != 0)
    {
        k_thread_abort(g_file_thread_id);
        g_file_thread_id = 0;
    }
}

void detect_thread_file(uint32_t ts)
{
    if((ts>gLastFileThread)
        &&(ts-gLastFileThread>THREAD_LOOP_TIME_5SEC))
    {
        warning_log("Restart file thread. ++++++++++++++++++\n");
        stopThreadFile();
        startThreadFile();
        gLastFileThread = k_uptime_get_32();
    }
}

static void checkPowerOffMsg()
{
    if(semTakePowerOff())
    {
        print_log("recv power OFF..................................\n");
        k_sched_lock();
        powerOffFile_t tmp = {0};
        tmp.ts = getTimeStamp();   
        getRTC(&tmp.rtc);
        if(!writePowerOffState(&tmp,sizeof(tmp)))
        {
            warning_log("Write Failed!!!.\n");
        }
        else
        {
            print_log("Write poweroff Ok!\n");
        }

        // k_sleep(100);
        //step1.unmount the filesystem.
        unmountFs();

        hwPowerDisable();
        k_sched_unlock();
        // k_sleep(500);
    }
}

#if 0 // if test case set to 1
#include "cache_box.h"
static void test_case_offline_file(void)
{
    char buf[128] = "hello linux\n";
    char read_buf[128];
    int ret=0;
    for(int i=0;i<7;i++)
    {
        for(int j=0;j<3;j++)
        {
            pushMessageToCache(buf,strlen(buf),i,i);
        }
    }

    do{
        memset(read_buf,0,sizeof(0));
        ret = popMessageFromCache(read_buf,sizeof(read_buf));
        print_log("%s",read_buf);
    }while(ret>0);

    print_log("done.\n");
    while(1);
}
#endif

static bool first_start = true;

static void threadFileEntry(void * p)
{
    if(first_start)
    {
        /* step init file */
            if(!fileInit())
            {
                err_log("fileInit failed");
                return;
            }

            /* step load config */
            if(loadConfig())
            {
                print_log("load config ok.\n");
                printConfig(&gSysconfig);
            }
            else
            {
                print_log("load config failed. Then reset it\n");
                resetConfig();
                gConfigFlag = true; //config bad
            }
            /* step load seat type config*/
            if(loadSeatTypeConfig())
            {
                print_log("load  seat type config ok.\n");
                print_log("seat type is [%d]...\n",gSeatTypeConfig.seatType);
            }
            else
            {
                print_log("load  seat type config failed. Then \n");
            }

            /* step check fota*/
            checkFOTA();
            
            //check pressure sensor wether setup
            checkPressAvail();
            
            initLastRfidFile();
            readLastRfidFile();

            readPowerOffFile();
            readRfidListFile();

            /*step delete tmp file*/
            deleteTmpRFIDFile();

            #if 0 /* support in the future */
            loadSpeedLimitConfig();
            #endif

            /* step tall main loop */
            semGiveInitFsOk();

            first_start = false;
    }

    while(1)
    {
        gLastFileThread = k_uptime_get_32();

        checkSaveRFIDList();
        checkSaveConfig2File();    
        checkOffline();
        writeLastRfidFile();
        if(semTakeRfidListUpdata())
        {
            readRfidListFile();
        }

        checkFotaOperate();
        checkTmpWriteRFID();
        checkTmpReadRFID();

        #if 0 /* support in the future */
        checkSpeedLimitSaveState();
        #endif

        checkPowerOffMsg();     /* At the end of the check list */
        checkSaveSeatTypeConfig2File();
        k_sleep(10);
    }

    /* It should never work here. */
    g_file_thread_id = 0;
    warning_log("Quit out of FILE_THREAD\n");
}

