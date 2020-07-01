#include "batter_stat_alg.h"
#include "my_misc.h"

#define RANGE_80V_MAX 88000
#define RANGE_80V_MIN 78000
#define RANGE_48V_MAX 52000 
#define RANGE_48V_MIN 42000
#define RANGE_36V_MAX 37500
#define RANGE_36V_MIN 31500
#define RANGE_24V_MAX 25000
#define RANGE_24V_MIN 21000
#define RANGE_DEVIATION 5000

#define BAT_FILTER  5  // state can be changed  +-  5%

static float CheckBatteryState(float volt,uint32_t vmin,uint32_t vmax);
extern uint32_t averageBatVolt(uint32_t volt[], uint8_t num);

uint8_t getBatteryStat(uint32_t bat_vol)
{
  int state = 0;
  if(bat_vol <= (RANGE_24V_MAX+RANGE_DEVIATION))
  {
    if(bat_vol<RANGE_24V_MIN)
    {
        state = 0;
    }
    else if(bat_vol>RANGE_24V_MAX)
    {
        state = 100;
    }
    else
    {
        state = (bat_vol-RANGE_24V_MIN)*100/(RANGE_24V_MAX-RANGE_24V_MIN);
    }
  }
  else if(bat_vol <= (RANGE_36V_MAX+RANGE_DEVIATION))
  {
      if(bat_vol<RANGE_36V_MIN)
      {
            state = 0;
      }
      else if(bat_vol>RANGE_36V_MAX)
      {
            state = 100;
      }
      else
      {
            state = (bat_vol-RANGE_36V_MIN)*100/(RANGE_36V_MAX-RANGE_36V_MIN);
      } 
  }
  else if(bat_vol >= (RANGE_48V_MIN-RANGE_DEVIATION) && bat_vol <= (RANGE_48V_MAX+RANGE_DEVIATION))
  {
        if(bat_vol<RANGE_48V_MIN)
        {
              state = 0;
        }
        else if(bat_vol>RANGE_48V_MAX)
        {
              state = 100;
        }
        else
        {
            state = (bat_vol-RANGE_48V_MIN)*100/(RANGE_48V_MAX-RANGE_48V_MIN);
        }
  }
  else if(bat_vol>=RANGE_80V_MIN-RANGE_DEVIATION)
  {
        if(bat_vol<RANGE_80V_MIN)
        {
              state = 0;
        }
        else if(bat_vol>RANGE_80V_MAX)
        {
              state = 100;
        }
        else
        {
            state = (bat_vol-RANGE_80V_MIN)*100/(RANGE_80V_MAX-RANGE_80V_MIN);
        }
  }
  else
  {
        state = 100;
        print_log("Unknow range battery.!!!!!!!!!!\n");
  }
  if(state <= 0)
  {
        state = 100;
  }
  else if(state >= 100)
  {
    state = 100;
  }
  return state;
}

uint8_t getStatefromVolt(uint32_t volt, uint32_t vmin, uint32_t vmax)
{
    static uint32_t aui32VoltBuff[BAT_FILTER] = {0};
    static uint32_t ui32StartTimeStamp = 0;
    static uint32_t ui32VoltSum = 0;
    static uint32_t ui32VoltNums = 0;
    static uint8_t ui8VoltBuffNums = 0;
    static float f32LastState = 0.0f;

    uint32_t ui32CurTimeStamp = 0;
    uint32_t ui32Average = 0;
    float raw_state = 0.0f;
    float new_volt = 0.0f;
    static  uint8_t ui8Flag = 0;

    if(volt < (vmin-1000))
    {
        return (uint8_t)f32LastState;
    }
    
    if(0 == ui32StartTimeStamp)
    {
        ui32StartTimeStamp = k_uptime_get_32();
        ui8Flag = 1;
        // printf("ui32StartTimeStamp:%d\n", ui32StartTimeStamp);
    }
    //printf("volt:%d\n", volt);
    //step1.convert to volt
    //new_volt = getKalmanFilteredValue((kalmanparamt *)&kalVolt, (float)volt);
    new_volt = (float)volt;
    //backup volt value to five
    aui32VoltBuff[ui8VoltBuffNums++] = (uint32_t)new_volt;

    //printf("new_volt:%f\n", new_volt);
    
    if(ui8VoltBuffNums < BAT_FILTER)
    {
        return (uint8_t)f32LastState;
    }
    else
    {
        //get average volt
        ui32Average = averageBatVolt(aui32VoltBuff, BAT_FILTER);
        
        //printf("ui32Average:%d,ui8VoltBuffNums:%d\n", ui32Average,ui8VoltBuffNums);
        
        ui8VoltBuffNums = 0;
    }   

    //Get Volt Sum Value and Nums
    if(ui32Average > 0)
    {
        //printf("ui32Average :%d\n", ui32Average);
        ui32VoltSum += ui32Average;
        ui32VoltNums++;
    }

    ui32CurTimeStamp = k_uptime_get_32();

    //printf("ui32VoltNums:%d,ui8Flag:%d, time:%d\n", ui32VoltNums,ui8Flag ,(ui32CurTimeStamp - ui32StartTimeStamp));

    //every ten seconds, return a battery state once time
    if(((ui32CurTimeStamp - ui32StartTimeStamp) >= (2 * 60 * 1000)) || (1 == ui8Flag))
    {
        if(0 == ui32VoltNums)
        {
            //serverSendLog("Batteryvolt is 0");
            return (uint8_t)f32LastState;
        }
        
        raw_state = CheckBatteryState((float)(ui32VoltSum / ui32VoltNums), vmin, vmax);
        if(raw_state > 0)
        {
            //printf("raw_state :%f\n", raw_state);
            ui32VoltSum = 0;
            ui32VoltNums = 0;
            ui8Flag = 0;
            
            ui32StartTimeStamp = k_uptime_get_32();
            f32LastState = raw_state;
            
            //return (uint8_t)(min(max(raw_state,10.0f),100.0f));
            return (uint8_t)raw_state;
        }
        else
        {
            return (uint8_t)f32LastState;
        }
    }
    else
    {
        //not up to five seconds, cotinue to backup volt sum
        return (uint8_t)f32LastState;
    }
}

float CheckBatteryState(float volt,uint32_t vmin,uint32_t vmax)
{
    //int8_t ai8TempBuff[128] = {0};
    float raw_state = 0.0f;
static float f32LastState = 0.0f;
//static uint32_t ui32StartTimeStamp = 0;    
//    int ui32CurTimeStamp = 0;

    /*if(0 == ui32StartTimeStamp)
    {
        ui32StartTimeStamp = k_uptime_get_32();
        //printf("ui32StartTimeStamp:%d\n", ui32StartTimeStamp);
    }*/

    //printf("volt:%f,vmin:%d,vmax:%d\n", volt,vmin,vmax);
    if(volt < vmin)
    {
        volt = vmin;
    }
    
    if(volt > vmax)
    {
        volt = vmax;
    }

    //sprintf(ai8TempBuff, "Batteryvolt:%f",volt);
    
    //if time up to five seconds,convert to state
    //raw_state = (volt - (float)vmin)*100.0f /(float)(vmax-vmin);
    raw_state = 10.0f + (volt - (float)vmin) * (90.0f / (float)(vmax-vmin));

    //printf("raw_state:%f\n",raw_state);
    //sprintf(ai8TempBuff + strlen(ai8TempBuff), ",Real:%f,Last:%f",raw_state,f32LastState);

    //ui32CurTimeStamp = k_uptime_get_32();
    
    //if(raw_state>50)raw_state = raw_state+10.0f;
    //else if(raw_state<=50  && raw_state>=30)raw_state=raw_state+5.0f;
    if((f32LastState - 0.0f) < 1.0f)
    {
        //set init data
        f32LastState = raw_state;
    }
    else
    {
        //check value change rand
        if((f32LastState > raw_state) && ((f32LastState - raw_state) > 1.0f))
        {
            //if((ui32CurTimeStamp - ui32StartTimeStamp) >= (2 * 55 * 1000))
            //{
                //printf("time > 6\n");
                raw_state = f32LastState - 1.0f;
            //    ui32StartTimeStamp = k_uptime_get_32();
            //}
            //else
            //{
            //    raw_state = 0.0f;
            //}
            
        }
        else if((f32LastState < raw_state) && ((raw_state - f32LastState) > 1.0f))
        {
            //if((ui32CurTimeStamp - ui32StartTimeStamp) >= (4 * 1000))
            //if((ui32CurTimeStamp - ui32StartTimeStamp) >= 4)
            //{
                //printf("time > 4\n");
                raw_state = f32LastState + 1.0f;
                //ui32StartTimeStamp = k_uptime_get_32();
            //}
            //else
            //{
            //    raw_state = f32LastState;
            //}
        }

        f32LastState = raw_state;
    }

    //sprintf(ai8TempBuff + strlen(ai8TempBuff), ",Cur:%f\n", raw_state);
    //serverSendLog(ai8TempBuff);
    //printf("%s", ai8TempBuff);

    return raw_state; 
}

void getVotRange(uint32_t volt, uint32_t* vmin, uint32_t* vmax)
{
    uint32_t vminTmp = 0;
    uint32_t vmaxTmp = 0;
    if(volt >= RANGE_24V_MIN && volt<= RANGE_24V_MAX)//24v
    {
        vminTmp = RANGE_24V_MIN;
        vmaxTmp = RANGE_24V_MAX;
    }
    else if(volt >= RANGE_36V_MIN && volt<= RANGE_36V_MAX)//36v
    {
        vminTmp = RANGE_36V_MIN;
        vmaxTmp = RANGE_36V_MAX;
    }
    else if(volt >= RANGE_48V_MIN && volt<= RANGE_48V_MAX)//48v
    {
        vminTmp = RANGE_48V_MIN;
        vmaxTmp = RANGE_48V_MAX;
    }
    else if(volt >= RANGE_80V_MIN && volt<= RANGE_80V_MAX)//80v
    {
        vminTmp = RANGE_80V_MIN;
        vmaxTmp = RANGE_80V_MAX;
    }
    *vmin = vminTmp;
    *vmax = vmaxTmp;
}


