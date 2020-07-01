#ifndef __ATCIP_H__
#define __ATCIP_H__

#define ATCIP_MODE_NORMAL	0
#define ATCIP_MODE_TRANSPARENT	1

extern int atcip_init(int mode);
extern int atcip_open(const char *host, int port, unsigned int timeout_ms);
extern int atcip_close(void);
extern int atcip_send(const char *buf, size_t len);
extern int atcip_recv(char *buf, size_t len, unsigned int timeout_ms);

#endif /* __ATCIP_H__ */
