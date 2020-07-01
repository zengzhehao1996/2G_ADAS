#ifndef __ATC_H__
#define __ATC_H__

#include "zephyr.h"

/* AT command status */
#define ATS_NONE		0
#define ATS_WAIT		1
#define ATS_DELAY		2
#define ATS_RETRY		3
#define ATS_OKAY		10
#define ATS_ERROR		11
#define ATS_TOUT		12

#define ATC_RESPBUFSIZE 1152  //same to DEST_BUFSIZE

enum {
	ATC_AT = 0,
	ATC_AT_W,
	ATC_ATE,
	ATC_IPR,
	ATC_CFUN,
	
	ATC_CPIN,
	ATC_GSN,
	ATC_CCID,
	ATC_CSQ,
	
	ATC_CREG,
	ATC_CGREG,
	ATC_COPS,
	ATC_CGATT,
	
	ATC_CIPMODE,
	ATC_CSTT,
	ATC_CIICR,
	ATC_CIFSR,
	
	ATC_CIPSTART,
	ATC_CIPSHUT,
	ATC_CIPCLOSE,
	ATC_CIPSEND,
	
	ATC_SAPBR,
	ATC_SAPBR21,
	ATC_FTP,
	ATC_FTPGET,
	
	ATC_FTPGET2,	/* For AT+FTPGET=2,x (x > 0) only */
	ATC_FTPPUT,
	ATC_FTPPUT2,	/* For AT+FTPPUT=2,x (x > 0) only */
    ATC_FTPSIZE,
    
    ATC_CIPSHUT2,
    ATC_PING,
} atcmd_name;

#define ATC_GSN_MAXLEN		20
#define ATC_CCID_MAXLEN		24

extern char atc_gsn[ATC_GSN_MAXLEN];
extern char atc_ccid[ATC_CCID_MAXLEN];

extern int atc_init(int resp_buf_size);
extern int atc_open(u32_t timeout_ms);
extern int atc_reset(void);
extern int atc_close(void);
extern int atc_cmd(int cmd, char *param, int steps, const char *data, int data_len);
extern int atc_test(void);
extern u32_t get_ftpsize(void);

/**
 * send() and recv() for TCP/IP transparent mode
 */
extern int atc_trsp_send(const char *data, size_t len);
extern int atc_trsp_recv(char *buf, size_t len);
uint8_t *atcGetIMEI(uint8_t* outIMEI);
uint8_t *atcGetCCID(uint8_t *outCCID);
int8_t actGetLastCsq(uint8_t *csq_0, uint8_t *csq_1);

#endif /* __ATC_H__ */
