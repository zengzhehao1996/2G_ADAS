#include "fifo.h"
#include "util_fifo.h"
#include "msg_structure.h"
#include "my_misc.h"

static myFifo_t g_fifoCanErr = 
{
    .elementSize = sizeof(canErrFIFO_t),
    .semStart = 0,
    .semEnd = 1
};

static myFifo_t g_fifoCanWorkHour = 
{
    .elementSize = sizeof(canWorkHourFIFO_t),
    .semStart = 0,
    .semEnd = 1
};
    static myFifo_t g_fifoRfid = 
{
    .elementSize = sizeof(rfidFIFO_t),
    .semStart = 0,
    .semEnd = 4
};

static myFifo_t g_fiforfid2Ctrl = 
{
    .elementSize = sizeof(rfid2CtrlFIFO_t),
    .semStart = 0,
    .semEnd = 1
};

static myFifo_t g_fifoCan = 
{
    .elementSize = sizeof(canFIFO_t),
    .semStart = 0,
    .semEnd = 2
};

static myFifo_t g_fifoOverSpeed = 
{
    .elementSize = sizeof(overSpeedFIFO_t),
    .semStart = 0,
    .semEnd = 1
};

static myFifo_t g_fifoGps = 
{
    .elementSize = sizeof(gpsFIFO_t),
    .semStart = 0,
    .semEnd = 1
};
static myFifo_t g_fifoDrvRerod = 
{
    .elementSize = sizeof(drvRcrdFifo_t),
    .semStart = 0,
    .semEnd = 2
};
static myFifo_t g_fifoAppendRFIDack = 
{
    .elementSize = sizeof(rfidAckFIFO_t),
    .semStart = 0,
    .semEnd = 1
};

static myFifo_t g_fifoCarryData = 
{
    .elementSize = sizeof(carryFIFO_t),
    .semStart = 0,
    .semEnd = 1

};

static myFifo_t g_speedLimitData = 
{
    .elementSize = sizeof(speedLimitFIFO_t),
    .semStart = 0,
    .semEnd = 1
};

static myFifo_t g_fifoFactryV14Data = 
{
    .elementSize = sizeof(factryV14FIFO_t),
    .semStart = 0,
    .semEnd = 1
};

static myFifo_t g_autoCanData = 
{
    .elementSize = sizeof(autoCanFIFO_t),
    .semStart = 0,
    .semEnd = 1
};

static myFifo_t g_unlockCause = 
{
    .elementSize = sizeof(unlockCauseFIFO_t),
    .semStart = 0,
    .semEnd = 1
};

static myFifo_t g_autoFotaCause = 
{
    .elementSize = sizeof(autoFotaFIFO_t),
    .semStart = 0,
    .semEnd = 1
};

bool fifoInit()
{

//2.init can err fifo
    if(!myFifoInit(&g_fifoCanErr))
    {
        err_log("init g_fifoCanErr failed\n");
        return false;
    }  
//3.init can work hour fifo
    if(!myFifoInit(&g_fifoCanWorkHour))
    {
        err_log("init g_fifoCanErr failed\n");
        return false;
    } 
//4.init rfid fifo
    if(!myFifoInit(&g_fifoRfid))
    {
        err_log("init g_fifoCanErr failed\n");
        return false;
    } 

//5.init rfid2ctrl fifo
    if(!myFifoInit(&g_fiforfid2Ctrl))
    {
        err_log("init g_fifoCanErr failed\n");
        return false;
    } 

//6.init can fifo
    if(!myFifoInit(&g_fifoCan))
    {
        err_log("init g_fifoCanErr failed\n");
        return false;
    } 

//7. init overspeed fifo
    if(!myFifoInit(&g_fifoOverSpeed))
    {
        err_log("init g_fifoOverSpeed failed\n");
        return false;
    }

    // init gps fifo
    if(!myFifoInit(&g_fifoGps))
    {
        err_log("init g_fifoGps failed\n");
        return false;
    }
    //init g_fifoDrvRerod
    if(!myFifoInit(&g_fifoDrvRerod))
    {
        err_log("init g_fifoDrvRerod failed\n");
        return false;
    }
    //init g_fifoAppendRFIDack
    if(!myFifoInit(&g_fifoAppendRFIDack))
    {
        err_log("init g_fifoAppendRFIDack failed\n");
        return false;
    }

    //init g_fifoCarryData
    if(!myFifoInit(&g_fifoCarryData))
    {
        err_log("init g_fifoCarryData failed\n");
        return false;
    }

    //init g_speedLimitData
    if(!myFifoInit(&g_speedLimitData))
    {
        err_log("init g_speedLimitData failed\n");
        return false;
    }

    //init g_fifoFactryV14Data
    if(!myFifoInit(&g_fifoFactryV14Data))
    {
        err_log("init g_speedLimitData failed\n");
        return false;
    }
    
    //init g_autoCanFifo
    if(!myFifoInit(&g_autoCanData))
    {
        err_log("init g_autoCanData failed\n");
        return false;
    }
    
    // init unlock cause
    if(!myFifoInit(&g_unlockCause))
    {
        err_log("init g_unlockCause failed\n");
        return false;
    }

    if(!myFifoInit(&g_autoFotaCause))
    {
        err_log("init g_autoFotaCause failed\n");
        return false;
    }

    return true;
}

bool canErrFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoCanErr,msgPtr))
    {
        //err_log("can err fifo send err\n");
        return false;
    }
    return true;

}
bool canErrFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoCanErr, msgPtr))
    {
        //err_log("can err fifo rcv err\n");
        return false;
    }
    return true;
}

bool canWorkHourFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoCanWorkHour,msgPtr))
    {
        //err_log("can work hour fifo send err\n");
        return false;
    }
    return true;
}
bool canWorkHourFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoCanWorkHour, msgPtr))
    {
        //err_log("can work hour fifo rcv err\n");
        return false;
    }
    return true;

}

bool rfid2CtrlFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fiforfid2Ctrl,msgPtr))
    {
        //err_log("g_fiforfid2Ctrl fifo send err");
        return false;
    }
    return true;

}
bool rfid2CtrlFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fiforfid2Ctrl, msgPtr))
    {
        //err_log("g_fiforfid2Ctrl fifo rcv err\n");
        return false;
    }
    return true;

}

bool rfid2Sim868FifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoRfid,msgPtr))
    {
        //err_log("rfid2Sim868FifoSend fifo send err");
        return false;
    }
    return true;

}
bool rfid2Sim868FifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoRfid, msgPtr))
    {
        //err_log("rfid2Sim868FifoSend fifo rcv err\n");
        return false;
    }
    return true;

}


bool canFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoCan,msgPtr))
    {
        //err_log("canFifoSend fifo send err\n");
        return false;
    }
    return true;

}
bool canFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoCan, msgPtr))
    {
        //err_log("canFifoRcv fifo rcv err\n");
        return false;
    }
    return true;

}


bool overspeedFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoOverSpeed,msgPtr))
    {
        //err_log("overspeedFifoSend fifo send err\n");
        return false;
    }
    return true;

}

bool overspeedFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoOverSpeed, msgPtr))
    {
        //err_log("overspeedFifoRcv fifo rcv err\n");
        return false;
    }
    return true;

}

bool gpsFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoGps,msgPtr))
    {
        //err_log("gpsFIFO fifo send err\n");
        return false;
    }
    return true;
}

bool gpsFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoGps, msgPtr))
    {
        //err_log("gps fifo rcv err\n");
        return false;
    }
    return true;
}

bool DrvRecordFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoDrvRerod,msgPtr))
    {
        //err_log("g_fifoDrvRerod fifo send err\n");
        return false;
    }
    return true;
}
bool DrvRecordFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoDrvRerod, msgPtr))
    {
        //err_log("gps fifo rcv err\n");
        return false;
    }
    return true;
}

bool appendRFIDFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoAppendRFIDack,msgPtr))
    {
        //err_log("g_fifoAppendRFIDack fifo send err\n");
        return false;
    }
    return true;
}

bool appendRFIDFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoAppendRFIDack, msgPtr))
    {
        //err_log("appendrfidack fifo rcv err\n");
        return false;
    }
    return true;
}

bool carryDataFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoCarryData,msgPtr))
    {
        return false;
    }
    return true;

}
bool carryDataFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoCarryData, msgPtr))
    {        
        return false;
    }
    return true;

}

bool speedLimitFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_speedLimitData,msgPtr))
    {
        return false;
    }
    return true;
}

bool speedLimitFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_speedLimitData, msgPtr))
    {        
        return false;
    }
    return true;
}

bool factryTestV14FifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_fifoFactryV14Data,msgPtr))
    {
        return false;
    }
    return true;
}

bool factryTestV14FifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_fifoFactryV14Data, msgPtr))
    {        
        return false;
    }
    return true;
}

bool autoCanResultFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_autoCanData,msgPtr))
    {
        return false;
    }
    return true;

}
bool autoCanResultFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_autoCanData, msgPtr))
    {        
        return false;
    }
    return true;

}

bool unlockCauseFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_unlockCause,msgPtr))
    {
        return false;
    }
    return true;

}

bool unlockCauseFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_unlockCause, msgPtr))
    {        
        return false;
    }
    return true;

}

bool autoFotaFifoSend(char *msgPtr)
{
    if(!myFifoSend(&g_autoFotaCause,msgPtr))
    {
        return false;
    }
    return true;
}

bool autoFotaFifoRcv(char *msgPtr)
{
    if(!myFifoRcv(&g_autoFotaCause, msgPtr))
    {        
        return false;
    }
    return true;
}
