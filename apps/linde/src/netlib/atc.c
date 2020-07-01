/**
 * Copyright (c) 2018 Rainbonic Technology Corporation. 
 */
#include <stdio.h>
#include <zephyr.h>
#include <uart.h>
#include <stdlib.h>
#include <string.h>

#include "atc.h"
#include "atd.h"

#include "config.h"
#include "my_misc.h"
#include "rtc.h"

//#define ATC_DEBUG
/* AT command types */
#define ATT_NORM		0
#define ATT_BAUD		7
#define ATT_SPEC		1
#define ATT_SEND		2
#define ATT_RECV		3
#define ATT_S1			4
#define ATT_SV_OK		5
#define ATT_OK_SV		6
#define ATT_FTPSIZE     8

/* AT command response timeout value */
#define AT_EXPIRE_RSP		500	
#define AT_EXPIRE_CHAR		100
#define AT_EXPIRE_ATCMD     5000

#define ATR_AT			"AT\r"	/* only used when ATE1 */
#define ATR_TAG			"\r\n"
#define ATR_CMD_ERROR		"\r\n+CMS ERROR:"
#define ATR_CME_ERROR		"\r\n+CME ERROR:"
#define ATR_OK			"\r\nOK\r\n"
#define ATR_CONNECT		"\r\nCONNECT\r\n"
#define ATR_CLOSED		"\r\nCLOSED\r\n"
#define ATR_CPIN		"\r\n+CPIN:"
#define ATR_CSQ			"\r\n+CSQ:"
#define ATR_ERROR		"\r\nERROR\r\n"
#define ATR_IPR			"\r\n+IPR:"
#define ATR_SHUT_OK		"\r\nSHUT OK\r\n"

/* RSSI value requirement, from AT+CSQ */
#define ATC_RSSI_MIN		9
#define ATC_CMDSIZE		64
#define ATC_BUFSIZE		512   // send data buf size

struct at_cmd_desc {
	u16_t		idx;
	u16_t		type;
	char		*cmd;
	char		*exp_ack0;
	char		*exp_ack1;
	u32_t		tout_ms;
	u32_t		retry_time;
};

struct at_cmd_info {
	struct k_sem	done_sem;
	struct k_mutex	mutex;
	u32_t		timeout;
	u16_t		cmd;
	char		cmdstr[ATC_CMDSIZE];
	u8_t		steps_required;
	u8_t		step;
	u8_t		status;
	u8_t		err_code;
	u32_t		retry;
	u16_t		resp_len;
	u16_t 		data_len;
	char		resp_buf[ATC_RESPBUFSIZE];
	char		data_buf[ATC_BUFSIZE];
};

char atc_gsn[ATC_GSN_MAXLEN];
char atc_ccid[ATC_CCID_MAXLEN];

static struct k_timer		at_timer;
static struct at_cmd_info	cmd_info;
static u32_t			atc_rssi;
static u32_t			atc_baud;
static u8_t			atc_trsp_mode;
static s8_t			atc_cpin_ready;
static s8_t			atc_baud_set;
static u32_t        ftpsize;
static u32_t		atc_cgreg;
u32_t 				atc_cgatt;

static struct
{
	uint32_t ts;
	uint32_t max_val;
}g_rssi;

static struct at_cmd_desc at_cmd_table[] = {
	{ATC_AT,	ATT_BAUD,	"AT",		ATR_OK,		NULL,		200,	3},
	{ATC_AT_W,	ATT_S1,		"AT&W",		ATR_OK,		NULL,		1000,	3},
	{ATC_ATE,	ATT_S1,		"ATE",		ATR_OK,		NULL,		1000,	3},
	{ATC_IPR,	ATT_SV_OK,	"AT+IPR",	ATR_IPR,	ATR_OK,		1000,	3},
	{ATC_CFUN,	ATT_NORM,	"AT+CFUN",	"OK",		NULL,		1000,	3},

	{ATC_CPIN,	ATT_S1,		"AT+CPIN",	ATR_OK,		NULL,		10000,	3},
	{ATC_GSN,	ATT_SV_OK,	"AT+GSN",	NULL,		ATR_OK,		1000,	3},
	{ATC_CCID,	ATT_SV_OK,	"AT+CCID",	NULL,		ATR_OK,		2000,	3},
	{ATC_CSQ,	ATT_SV_OK,	"AT+CSQ",	"\r\n+CSQ:",	ATR_OK,		1000,	60},
	
	{ATC_CREG,	ATT_SV_OK,	"AT+CREG",	"\r\n+CREG:",	ATR_OK,		1000,	3},
	{ATC_CGREG,	ATT_SV_OK,	"AT+CGREG",	"\r\n+CGREG:",	ATR_OK,		1000,	3},
	{ATC_COPS,	ATT_S1,		"AT+COPS",	"+COPS:",	ATR_OK,		1000,	3},
	{ATC_CGATT,	ATT_SV_OK,	"AT+CGATT",	"\r\n+CGATT:",	ATR_OK,	       10000,	3},

	{ATC_CIPMODE,	ATT_S1,		"AT+CIPMODE",	ATR_OK,		ATR_OK,		1000,	3},
	{ATC_CSTT,	ATT_S1,		"AT+CSTT",	ATR_OK,		NULL,		1000,	3},
	{ATC_CIICR,	ATT_S1,		"AT+CIICR",	ATR_OK,		NULL,	       60000,	3},
	{ATC_CIFSR,	ATT_SPEC,	"AT+CIFSR",	NULL,		NULL,		1000,	3},

	//{ATC_CIPSTART,	ATT_SPEC,	"AT+CIPSTART",	ATR_OK,		ATR_CONNECT,	13000,	3},
	{ATC_CIPSTART,	ATT_SPEC,	"AT+CIPSTART",	ATR_OK,		ATR_CONNECT,	30000,	3},
	{ATC_CIPSHUT,	ATT_S1,		"AT+CIPSHUT",	ATR_SHUT_OK,	NULL,		1000,	3},
	{ATC_CIPCLOSE,	ATT_S1,		"AT+CIPCLOSE",	ATR_OK,		NULL,		1000,	3},
	{ATC_CIPSEND,	ATT_S1,		"AT+CIPSEND",	"OK",		NULL,		1000,	3},

	{ATC_SAPBR,	ATT_S1,		"AT+SAPBR",	ATR_OK,		NULL,		10000,	3},
	{ATC_SAPBR21,	ATT_SV_OK,	"AT+SAPBR",	"\r\n+SAPBR:",	"OK",		10000,	3},
	{ATC_FTP,	ATT_S1,		"AT+FTP",	ATR_OK,		NULL,		10000,	3},
	{ATC_FTPGET,	ATT_OK_SV,	"AT+FTPGET",	"OK",		"\r\n+FTPGET:",		10000,	3},
	{ATC_FTPGET2,	ATT_RECV,	"AT+FTPGET",	"\r\n+FTPGET:",	"\r\nOK\r\n",		10000,	3},
	{ATC_FTPPUT,	ATT_OK_SV,	"AT+FTPPUT",	"OK",		"\r\n+FTPPUT:",	10000,	3},
	{ATC_FTPPUT2,	ATT_SEND,	"AT+FTPPUT",	"\r\n+FTPPUT:",	"\r\nOK\r\n",	10000,	3},
	{ATC_FTPSIZE,	ATT_FTPSIZE,	"AT+FTPSIZE",	"",		"",		10000,	3}, 
        
    {ATC_CIPSHUT2 ,	ATT_S1,		"AT+CIPSHUT",	ATR_OK,	ATR_SHUT_OK,		3000,	3},
	{ATC_PING,	    ATT_SV_OK,  "AT+CIPPING",	"\r\n+CIPPING",	ATR_OK, 10000,	3},
};

static void dump_at_str(const char *title, const char *str)
{
#ifdef ATC_DEBUG
	const char *p = str;

	printk("%s: {", title);
	while (*p) {
		switch (*p) {
		case '\r':
			printk("[CR]");
			break;
		case '\n':
			printk("[LF]");
			break;
		default:
			printk("%c", *p);
		}
		p++;	
	}
	printk("}\n");
#endif
}

static int send_command(const char *str)
{
	return atd_write(str, strlen(str), 1);
}

static int send_data(const char *buf, size_t len)
{
	return atd_write(buf, len, 0);
}

static int status_set(struct at_cmd_info *info, int status)
{
#ifdef ATC_DEBUG	
	printk("Set status to ");
	switch (status) {
	case ATS_NONE:
		printk("ATS_NONE\n");
		break;
	case ATS_WAIT:
		printk("ATS_WAIT\n");
		break;
	case ATS_DELAY:
		printk("ATS_DELAY\n");
		break;
	case ATS_RETRY:
		printk("ATS_RETRY\n");
		break;
	case ATS_OKAY:
		printk("ATS_OKAY\n");
		break;
	case ATS_ERROR:
		printk("ATS_ERROR\n");
		break;
	case ATS_TOUT:
		printk("ATS_TOUT\n");
		break;
	default:
		printk("%s: Unknown status %d.\n", __func__, status);
	}
#endif	
	k_mutex_lock(&info->mutex, K_FOREVER);
	info->status = status;
	if (status == ATS_OKAY)
		info->step++;
	k_mutex_unlock(&info->mutex);
	
	return status;
}

static void status_init(struct at_cmd_info *info)
{
	k_mutex_lock(&info->mutex, K_FOREVER);
	info->step = 0;
	info->status = ATS_WAIT;
	k_mutex_unlock(&info->mutex);
}

static char *skip_one_param(const char *rsp)
{
	int step = 0;
	char *p = (char *)rsp;
	
	while (*p) {
		switch (*p) {
		case '\"':
			step++;
			break;
			
		case ',':
			if (step > 1) {
				p++;
				break;
			} else {
				p++;
				return p;
			}

		case ' ':
		default:
			p++;
		}
	}
	return p;
}

static int read_number(const char *str, u32_t *num)
{
	int found = 0;
	signed char c;
	u32_t v = 0;
	int i;

	for (i = 0; i < strlen(str); i++) {
		c = str[i];
		if ((c >= '0') && (c <= '9')) {
			if (found == 0)
				found = 1;
			v *= 10;
			v += c - '0';
		} else if (c == ',')
			break;
		else if (found == 1) 
			break;
	}

	if (found) {
		*num = v;
		return 1;
	}
	
	return -1;
}

static int read_string(const char *str, char *param_str)
{
	int step = 0;
	char c;
	char *p_src = (char *)str;
	char *p_dst = param_str;
	int retval = 0;

	while (*p_src) {
		c = *p_src;
		if (c == '\"') {
			if (step == 0)
				step = 1;
			else if (step == 1) {
				step = 2;
				retval = 1;
				break;
			}
		} else if (step == 1) {
			*p_dst++ = c;
		}
		p_src++;
	}

	switch (step) {
	case 0:
		return 0;
	case 1:
	default:
		retval = -1;
		/* fall through */
	case 2:
		*p_dst = '\0';
		return retval;
	}
}

#define CHAR_OF_SN(c)	((((c) >= '0') && ((c) <= '9'))		\
			|| (((c) >= 'a') && ((c) <= 'z'))	\
			|| (((c) >= 'A') && ((c) <= 'Z')))
/**
 * @brief	Parse the string for SN
 *
 * @return	  0 - if no sn is found
 *		> 0 - number of bytes parsed, and SN string is stored in sn_ptr
 */


static int parse_sn(const char *str, char *sn_ptr, size_t len)
{
	int  s = 0;
	char *p_src = (char *)str;
	char c;
	int cnt = 0;

	while (*p_src) {
		c = *p_src++;
		if ((s == 0) && (c == '\r')) {
			s = 1;
		} else if ((s == 1) && (c == '\n')) {
			s = 2;		} else if ((s == 2) && CHAR_OF_SN(c)) {
			sn_ptr[cnt++] = c;
		} else
			break;
		if (cnt >= len)
			break;
	}
	if (cnt > 0) 
		sn_ptr[cnt] = '\0';
	
	return cnt;
}

static int parse_one_param(const char *rsp, const char *patten, int idx, u32_t *param_num, char *param_str)
{
	char *p, *next;
	int i;

	if ((p = strstr(rsp, patten)) == NULL)
		return -1;

	p += strlen(patten);
	for (i = 0; i < idx; i++)
		p = skip_one_param(p);

	if (param_num != NULL)
		return read_number(p, param_num);

	if (param_str != NULL)
		return read_string(next, param_str);
	
	return 0;
}

static int atr_match(const char *buf, int len, const char *pat)
{
	int plen = strlen(pat);
	int rc = 0;

	if (plen <= len) {
		if (!strncmp(buf, pat, plen))
			rc = plen;
	}
	return rc;
}

static int check_baud(struct at_cmd_info *info, char *data, int len)
{
	int rc;

	if ((rc = atr_match(data, len, ATR_AT)) > 0)
		return rc;

	rc = atr_match(data, len, ATR_OK);
	if (rc > 0) {
		status_set(info, ATS_OKAY);
		return rc;
	}
	
	return (data[0] != '\r') ? 1 : 0;
		return 1;
}

static int handle_s1(struct at_cmd_info *info, const char *exp_ack0, char *data, int len)
{
	char *p;
	int rc = 0;

	switch (info->cmd) {
	case ATC_ATE:
		if ((p = strstr(data, exp_ack0)) != NULL)
			rc = p - data + strlen(ATR_OK);
		break;
	case ATC_CPIN:
		rc = atr_match(data, len, ATR_ERROR);
		if (rc > 0) {
			atc_cpin_ready = 0;
			break;
		}
		rc = atr_match(data, len, ATR_OK);
		break;
	default:
		rc = atr_match(data, len, exp_ack0);
	}
	if (rc > 0)
		status_set(info, ATS_OKAY);
	
	return rc;
}

static int handle_sv_ok(struct at_cmd_info *info, const char *exp_ack0, char *data, int len)
{
	char	*p;
	int	pos;
	int	rc = 0;

	if (exp_ack0 != NULL) {
		rc = atr_match(data, len, ATR_OK);
		if (rc > 0) {
			status_set(info, ATS_OKAY);
			return rc;
		}
		rc = atr_match(data, len, exp_ack0);
		if (rc == 0)
			return 0;
	}

	switch (info->cmd) {
	case ATC_IPR:
		rc = parse_one_param(data, "+IPR:", 0, &atc_baud, NULL);
		if (rc == 0)
			atc_baud = -1;
		break;
	case ATC_CSQ:
		rc = parse_one_param(data, "+CSQ:", 0, &atc_rssi, NULL);
		break;
	case ATC_CGREG:
		rc = parse_one_param(data, "+CGREG:", 1, &atc_cgreg, NULL);
		break;
	case ATC_CGATT:
		rc = parse_one_param(data, "+CGATT:", 0, &atc_cgatt, NULL);
		break;
	case ATC_GSN:
		rc = parse_sn(data, atc_gsn, ATC_GSN_MAXLEN);
        if(15 == strlen(atc_gsn))
        {
            memset(gSysconfigBackup.devId,0,sizeof(gSysconfigBackup.devId));
            strncpy(gSysconfigBackup.devId, atc_gsn, sizeof(gSysconfigBackup.devId));
            print_log("IMEI:[%s].\n", gSysconfigBackup.devId);
        }
		if (!rc)
			return 0;
		break;
	case ATC_CCID:
		rc = parse_sn(data, atc_ccid, ATC_CCID_MAXLEN);
		if (!rc)
			return 0;
		break;
	}
	pos = rc;
	p = strstr(&data[pos], "\r\nOK\r\n");
	if (p == NULL)
		return 0;

	pos = p - data;
	rc = pos + 6;
	status_set(info, ATS_OKAY);
	
	return rc;
}

static int handle_ok_sv(struct at_cmd_info *info, const char *exp_ack1, char *data, int len)
{
	int pos, rc;
	char *p;

	if (info->step == 0) {
		rc = atr_match(data, len, "\r\nOK\r\n");
		if (rc > 0) {
			status_set(info, ATS_OKAY);
			return rc;
		} else
			return 0;
	}

	rc = atr_match(data, len, exp_ack1);
	if (rc > 0) {
		pos = rc;
		p = strstr(&data[pos], "\r\n");
		if (p == NULL)
			return 0;
		pos = p - data;
		rc = pos + 2;
		status_set(info, ATS_OKAY);
	}
	return rc;
}

static int handle_ftpput(struct at_cmd_info *info, const char *data, int len)
{
	u32_t dlen;
	int pos;
	int rc = 0;

	if (info->step == 0) {
		rc = atr_match(data, len, "\r\n+FTPPUT:");
		if (rc == 0)
			return 0;
		if (parse_one_param(info->resp_buf, "+FTPPUT:", 1, &dlen, NULL) < 1)
			return 0;
		status_set(info, ATS_OKAY);
		send_data(info->data_buf, info->data_len);
		return len;
	}

	if (info->step == 1) {
		rc = atr_match(data, len, "\r\nOK\r\n");
		if (rc == 0)
			return 0;
		pos = rc;
		rc = atr_match(&data[pos], len - pos, "\r\n+FTPPUT:");
		if (rc == 0)
			return 0;
		status_set(info, ATS_OKAY);
		rc = len;
	}

	return rc;
}

int __attribute__((weak)) move_ftpdata(void *srcbuf, int len) {
    return 0;
}

void __attribute__((weak)) set_ftpdone(void) {
    return ;
}

static int handle_ftpget(struct at_cmd_info *info, const char *data, int len)
{
	u32_t dlen;
	int pos;
	int rc = 0;
    
	if (info->step == 0) {
		rc = atr_match(data, len, "\r\n+FTPGET: 2,");
		if (rc == 0)
			return 0;
		if (parse_one_param(info->resp_buf, "+FTPGET:", 1, &dlen, NULL) < 1)
			return 0;
		status_set(info, ATS_OKAY);
        
        while ((data[rc] != '\n') && (rc<len)) rc++;

		move_ftpdata(&data[rc+1], dlen);
		return len;
    }

	return rc;
}

static int handle_ftpsize(struct at_cmd_info *info, const char *data, int len)
{
	int pos;
	int rc = 0;
        
    rc = atr_match(data, len, "\r\nOK");
    if (rc == 0)
        return 0;
    if (parse_one_param(info->resp_buf, "+FTPSIZE: 1,0,", 0, &ftpsize, NULL) < 1)
        return 0;
    status_set(info, ATS_OKAY);
    rc = len;
    
	return rc;
}

u32_t get_ftpsize(void) 
{
    return ftpsize;
}

static int check_cifsr(struct at_cmd_info *info, char *data, int len)
{
	char *p;
	int pos;

	if (atr_match(data, len, ATR_TAG) == 0)
		return 0;

	p = strstr(&data[2], ATR_TAG);
	pos = p - data;
	status_set(info, ATS_OKAY);
	return pos + 2;
}

static int check_cipstart(struct at_cmd_info *info, char *data, int len)
{
	int rc;

	if (info->step == 0) {
		rc = atr_match(data, len, ATR_OK);
		if (rc == 0) 
			return 0;
		status_set(info, ATS_OKAY);
		return rc;
	}
	rc = atr_match(data, len, ATR_CONNECT);
	if (rc == 0)
		return 0;
	status_set(info, ATS_OKAY);
	atc_trsp_mode = 1;
    // print_log("atc trsp mode = 1111111111111111111111\n");
	
	return rc;
}

static int handle_spec_cmd(struct at_cmd_info *info, char *data, int len)
{
	switch (info->cmd) {
	case ATC_CIFSR:
		return check_cifsr(info, data, len);
	case ATC_CIPSTART:
		return check_cipstart(info, data, len);
	default:
		printk("Unsupported AT command %d.\n", info->cmd);
		return 0;
	}
}

/**
 * @brief	Handle unsolicited result code
 */
static int handle_urc(const char *data, int len)
{
	char *p;
	int rc;

	rc = atr_match(data, len, "\r\nRDY\r\n");
	if (rc > 0) {
		atc_baud_set = 1;
		return rc;
	}
	
	rc = atr_match(data, len, "\r\n+CFUN: 1\r\n");
	if (rc > 0)
		return rc;
	
	rc = atr_match(data, len, "\r\n+CPIN:");
	if (rc > 0) {
		if (strstr(data, "+CPIN: READY") != NULL)
			atc_cpin_ready = 1;
		else
			atc_cpin_ready = -1;
		p = strstr(&data[2], "\r\n");
		return (p != NULL) ? (p - data) + 2 : 0;
	}
	
	rc = atr_match(data, len, "\r\nCall Ready\r\n");
	if (rc > 0)
		return rc;

	rc = atr_match(data, len, "\r\nSMS Ready\r\n");
	if (rc > 0)
		return rc;

	return 0;
}

static void handle_response(struct at_cmd_info *info)
{
	struct at_cmd_desc *desc = &at_cmd_table[info->cmd];
	int pos, rest;
	int len1, len2;
	int i;

	info->resp_buf[info->resp_len] = '\0';
#if 1
    print_log("DMA Recv:[ %s ],len:[%d]\n",info->resp_buf,info->resp_len);
#endif
	pos = 0;
	rest = info->resp_len;
	while (rest) {
		len1 = handle_urc(&info->resp_buf[pos], rest);
		if (len1 > 0) {
			pos += len1;
			rest -= len1;
			continue;
		}
		
		switch (desc->type) {
		case ATT_BAUD:
			len2 = check_baud(info, &info->resp_buf[pos], rest);
			break;
			
		case ATT_SPEC:
			len2 = handle_spec_cmd(info, &info->resp_buf[pos], rest);
			break;
			
		case ATT_S1:
			len2 = handle_s1(info, desc->exp_ack0, &info->resp_buf[pos], rest);
			break;

		case ATT_SV_OK:
			len2 = handle_sv_ok(info, desc->exp_ack0, &info->resp_buf[pos], rest);
			break;

		case ATT_OK_SV:
			len2 = handle_ok_sv(info, desc->exp_ack1, &info->resp_buf[pos], rest);
			break;

		case ATT_SEND:
			len2 = handle_ftpput(info, &info->resp_buf[pos], rest);
			break;

		case ATT_RECV:
			len2 = handle_ftpget(info, &info->resp_buf[pos], rest);
			break;

        case ATT_FTPSIZE:
			len2 = handle_ftpsize(info, &info->resp_buf[pos], rest);
			break;
            
        default:
			printk("%s: AT command type %d NOT implemented.\n", __func__, desc->type);
			len2 = rest;
		}
		if (len2 == 0)
			break;

		pos += len2;
		rest -= len2;
	}
	if (rest) {
		for (i = 0; i < rest; i++) {
			info->resp_buf[i] = info->resp_buf[pos + i];
		}
		info->resp_len = rest;
	} else{
        info->resp_len = 0;
    }

}

static void at_timer_func(struct k_timer *timer)
{
	struct at_cmd_info *info = &cmd_info;
	int done = 0;
	int bytes;
	int status;
	int step;

	if (atc_trsp_mode) {
		k_timer_start(timer, 500, 0);
        // print_log("in trsp mode.\n");
		return;
	}
    //print_log("not in trsp mode.............................\n");
	bytes = atd_read(&info->resp_buf[info->resp_len], ATC_RESPBUFSIZE - info->resp_len);
	if (bytes > 0) {
		info->resp_len += bytes;
		goto out;
	} else if (info->resp_len > 0) {
		handle_response(info);
        info->resp_len = 0;
	}

	step = info->step;
	status = info->status;
	if (status != ATS_NONE) {
		if ((status == ATS_OKAY) && (step == info->steps_required))
		{
			done = 1;
		}
		else if (status == ATS_ERROR)
		{
			done = 1;
		}
		else if (k_uptime_get_32() > info->timeout) {
			status_set(info, ATS_TOUT);
			done = 1;
		}
		if (done)
		{
			k_sem_give(&info->done_sem);
		}
	}
out:
	k_timer_start(timer, 100, 0);
}

int atc_cmd(int cmd, char *param, int steps, const char *data, int data_len)
{
    // print_log("atc cmd start ...............\n");
    atd_clean();
	struct at_cmd_info *info = &cmd_info;
	struct at_cmd_desc *desc = &at_cmd_table[cmd];
	int status;

    info->resp_len = 0;
   
	info->cmd = cmd;
	strncpy(info->cmdstr, desc->cmd, ATC_CMDSIZE - 1);
	strncat(info->cmdstr, param, ATC_CMDSIZE - strlen(desc->cmd));
	info->steps_required = steps;
	info->retry = 0;
	if (data_len != 0) {
		info->data_len = data_len;
		memcpy(info->data_buf, data, data_len);
	}
	info->timeout = k_uptime_get_32() + desc->tout_ms;
	status_init(info);
    print_log("send command [ %s ]\n",info->cmdstr);
    k_sem_take(&info->done_sem, K_NO_WAIT);
	send_command(info->cmdstr);
    // print_log("take (info->done_sem)\n");
	k_sem_take(&info->done_sem, K_FOREVER);
    //k_sem_take(&info->done_sem, 5000);
	status = info->status;
	info->status = ATS_NONE;
    print_log("atc_cmd return [%d] \n",status);
	return status;
}

/*
 * External functions
 */
int atc_open(u32_t timeout_ms)
{
	int rc;
	u32_t timeout;
	
	u32_t start, now;
	start = k_uptime_get_32();
	k_sleep(K_SECONDS(5));
    print_log("start atc_baud_set...\n");
	if (atc_baud_set == 0) {
		do {
			rc = atc_cmd(ATC_AT, "", 1, NULL, 0);
            now = k_uptime_get_32();
            if ((now - start) > (AT_EXPIRE_ATCMD + 30000))
                return -1;
		} while (rc == ATS_TOUT);
		if (atc_cmd(ATC_ATE, "0&W", 1, NULL, 0) != ATS_OKAY)
			return -1;
		if (atc_cmd(ATC_IPR, "=115200", 1, NULL, 0) != ATS_OKAY)
			return -1;
		if (atc_cmd(ATC_AT_W, "", 1, NULL, 0) != ATS_OKAY)
			return -1;
	}
#if 0
	/**
	 * define to 1 to test auto-baud command sequence
	 */
	else {
 		if (atc_cmd(ATC_IPR, "=0", 1, NULL, 0) != ATS_OKAY)
			return -1;
		if (atc_cmd(ATC_AT_W, "", 1, NULL, 0) != ATS_OKAY)
			return -1;
		if (atc_cmd(ATC_ATE, "1&W", 1, NULL, 0) != ATS_OKAY)
			return -1;
		if (atc_cmd(ATC_ATE, "0", 1, NULL, 0) != ATS_OKAY)
			return -1;
	}
#endif	

#if 0	
	if (atc_cmd(ATC_CFUN, "", 1, NULL, 0) != ATS_OKAY)
		return -1;
#endif
	timeout = k_uptime_get_32() + timeout_ms;

	if (atc_cpin_ready == 0) {
		if (atc_cmd(ATC_CPIN, "?", 1, NULL, 0) != ATS_OKAY)
		{
			return -1;
		}
	}
	if (atc_cpin_ready == -1)
		return -1;
	if (atc_cmd(ATC_GSN, "", 1, NULL, 0) != ATS_OKAY)
	{
		return -1;
	}
	if (atc_cmd(ATC_CCID, "", 1, NULL, 0) != ATS_OKAY)
	{
		return -1;
	}
	if(200 == gSysconfig.canType)
	{
		for(int i = 0; i<5;i++)
		{
			timeout += 1000;
			do {
				k_sleep(K_MSEC(1000));
				atc_cmd(ATC_CSQ, "", 1, NULL, 0);
				if (k_uptime_get_32() > timeout) 
					return -1;
			} while (atc_rssi < ATC_RSSI_MIN);
			if(g_rssi.max_val < atc_rssi)
			{
				g_rssi.max_val = atc_rssi;
				g_rssi.ts = getTimeStamp();
			}
		}
	}
	else
	{
		do {
			k_sleep(K_MSEC(1000));
			atc_cmd(ATC_CSQ, "", 1, NULL, 0);
			if (k_uptime_get_32() > timeout) 
				return -1;
		} while (atc_rssi < ATC_RSSI_MIN);
		g_rssi.max_val = atc_rssi;
		g_rssi.ts = getTimeStamp();
	}
	char buf[64] = {0};
	if(g_rssi.max_val >= 22)
	{
		sprintf(buf, "timestamp:%d CSQ:%d PASS_1.\n",g_rssi.ts,g_rssi.max_val);
	}
	else
	{
		sprintf(buf, "timestamp:%d CSQ:%d FAILED_0.\n",g_rssi.ts,g_rssi.max_val);
	}
	serverSendErrLog(buf);
	print_log("SEND to SERVER:%s",buf);

#ifdef ATC_DEBUG	
	now = k_uptime_get_32();
	printk("%s: %u.%03u seconds passed\n", __func__,
		(now - start) / 1000, (now - start) % 1000);
#endif	
	if (atc_cmd(ATC_CREG, "?", 1, NULL, 0) != ATS_OKAY)
		return -1;
	do{
		if (k_uptime_get_32() > timeout) 
				return -1;
		k_sleep(1000);
		if (atc_cmd(ATC_CGREG, "?", 1, NULL, 0) != ATS_OKAY)
			return -1;
	}while(atc_cgreg!=1 && atc_cgreg!=5);
		
	return 0;
}

int atc_reset(void)
{
    print_log("atc reset ........................\n");
    atc_trsp_mode = 0;
    atc_cpin_ready = 0;
    atc_baud_set = 0;
	return 0;
}

int atc_close(void)
{
	return 0;
}

int atc_trsp_send(const char *data, size_t len)
{
	if (!atc_trsp_mode)
		return -EIO;
	
	return atd_write(data, len, 0);
}

int atc_trsp_recv(char *buf, size_t len)
{
	int rc = 0;
	
	if (!atc_trsp_mode)
		return -EIO;

	rc = atd_read(buf, len);
    //print_log("want to read ..........%d, have read ..............%d\n",len,rc);
    /******************
    if(rc>0)
    {
        print_log("rc:%d atd read:::[\n",rc);
        for (int i=0;i<rc;i++)
        {
         printk("0x%02x ",buf[i]);
        }

         print_log("]\n");
    }
    ********************/
#if 0	
	if (atr_match(buf, rc, ATR_CLOSED)) {
		atc_trsp_mode = 0;
	}
#endif	
	return rc;
}

int atc_init(int resp_buf_size)
{
	if (atd_init() != 0)
		return -1;

	k_mutex_init(&cmd_info.mutex);
	k_sem_init(&cmd_info.done_sem, 0, 1);
	k_timer_init(&at_timer, at_timer_func, NULL);
	k_timer_start(&at_timer, 100, 0);
	atc_trsp_mode = 0;
	
	return 0;
}

/*
int atc_test(void)
{
    int rc;
	u32_t start, now;
	start = k_uptime_get_32();

	do {
		rc = atc_cmd(ATC_AT, "", 1, NULL, 0);
        now = k_uptime_get_32();
        if ((now - start) > AT_EXPIRE_ATCMD)
            return 1;
	} while (rc == ATS_TOUT);
	
    return 0;
        
}
*/

uint8_t *atcGetIMEI(uint8_t* outIMEI)
{
    if(15 == strlen(atc_gsn))
    {
        if(outIMEI)
        {
            strncpy(outIMEI, atc_gsn, strlen(atc_gsn));
            print_log("111111111\n");
        }
        return atc_gsn;
    }
    else
    {
        if(outIMEI)
        {
            strncpy(outIMEI, gSysconfig.devId, strlen(gSysconfig.devId));
            print_log("222\n");
        }
        return gSysconfig.devId;
    }
}

uint8_t *atcGetCCID(uint8_t *outCCID)
{
    if(20 == strlen(atc_ccid))
    {
        if(outCCID)
        {
            strncpy(outCCID,atc_ccid,strlen(atc_ccid));
        }
        return atc_ccid;
    }
    else
    {
        return NULL;
    }
    
}

int8_t actGetLastCsq(uint8_t *csq_0, uint8_t *csq_1)
{
	if(g_rssi.max_val >=9)
	{
		*csq_0 = g_rssi.max_val;
		*csq_1 = 0;
		return 0;
	}

	return -1;
}

uint32_t getAtcCGATT(void)
{
	return atc_cgatt;
}