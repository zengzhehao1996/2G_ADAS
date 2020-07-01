#include <zephyr.h>
#include <stdio.h>
#include "atcip.h"
#include "atc.h"
#include "my_misc.h"

extern uint32_t getAtcCGATT(void);

int atcip_send(const char* buf, size_t len)
{
    return atc_trsp_send(buf, len);
}

int atcip_recv(char* buf, size_t len, u32_t timeout_ms)
{
    u32_t start_ms;
    int   rc;

    start_ms = k_uptime_get_32();
    do
    {
        k_sleep(K_MSEC(timeout_ms / 5));
        rc = atc_trsp_recv(buf, len);
        if(rc > 0)
        {
            break;
        }

        if(rc < 0)
            return 0;
    } while(k_uptime_get_32() < (start_ms + timeout_ms));
    return rc;
}

#define MAX_CONNECT_TCP 5
#define MIN_RECONNECT_30MIN (30*60*1000)
static void declareCIPSTART(uint8_t val)
{
    static uint8_t last = 0;
    static uint32_t last_ms = 0;
    uint32_t curr_ms = k_uptime_get_32();

    if(0 == last)
    {
        last_ms = curr_ms;
    }

    if(0 == val)
    {
        last = 0;
    }
    else
    {
        last += 1;
        print_log("IP error :[%d], diff:[%d]\n",last,curr_ms-last_ms);
    }
    
    if(last > MAX_CONNECT_TCP && curr_ms - last_ms >= MIN_RECONNECT_30MIN)
    {
        /* 重置服务器IP及端口 */
        resetServerIp();
    }
}

extern u32_t atc_cgatt;
/**
 * @brief	Establish TCP/IP connection to host:port.
 *
 * @param	host	        String of host's IP address
 * @param	port		Port number
 * @param	timeout_ms	timeout in millisecond
 *
 * @return	0 on failure; >0 otherwise
 */
int atcip_open(const char* host, int port, unsigned int timeout_ms)
{
	u32_t timeout;
	char param[64];

    k_sleep(2000);
    timeout = k_uptime_get_32() + timeout_ms;
    atc_reset();
    bool call_flag = true;
   
	while (k_uptime_get_32() < timeout) {
    	k_sleep(100);
		if (atc_cmd(ATC_CIPSHUT, "", 1, NULL, 0) != ATS_OKAY)
			continue;
        do{
            k_sleep(1000);
            if(k_uptime_get_32() > timeout)
            {
                return 0;
            }
            if (atc_cmd(ATC_CGATT, "?", 1, NULL, 0)  != ATS_OKAY)
                        continue;
        }while(1!=getAtcCGATT());
		if (atc_cmd(ATC_CSTT, "=\"CMNET\"", 1, NULL, 0)  != ATS_OKAY)
			continue;
		if (atc_cmd(ATC_CIICR,  "", 1, NULL, 0)  != ATS_OKAY)
			continue;
		if (atc_cmd(ATC_CIFSR,  "", 1, NULL, 0)  != ATS_OKAY)
			continue;

        timeout = k_uptime_get_32() + timeout_ms;
        sprintf(param, "=\"%s\",\"1\"", host);
        do{
            k_sleep(1000);
            if(k_uptime_get_32() > timeout)
            {
                return 0;
            }
        }while(atc_cmd(ATC_PING, param, 1, NULL, 0)  != ATS_OKAY);

		sprintf(param, "=\"TCP\",\"%s\",\"%d\"", host, port);
		if (atc_cmd(ATC_CIPSTART, param, 2, NULL, 0) != ATS_OKAY)
        {
            if(call_flag)
            {
                declareCIPSTART(1);
                warning_log("TCP connect TimeOut.\n");
                call_flag = false;
            }
            continue;
        }
        else
        {
            declareCIPSTART(0);
        }
	
        k_sleep(100);
        return 1;
    }

    return 0;
}

int atcip_close(void)
{
    return 0;
}

int atcip_init(int mode)
{
    char param[4];

    param[0] = '=';
    switch(mode)
    {
        case ATCIP_MODE_NORMAL:
            param[1] = '0';
            break;
        case ATCIP_MODE_TRANSPARENT:
            param[1] = '1';
            break;
        default:
            return -1;
    }
    param[2] = '\0';

    if(atc_cmd(ATC_CIPMODE, param, 1, NULL, 0) != ATS_OKAY)
        return -1;

    return 0;
}
