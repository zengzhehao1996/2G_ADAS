#ifndef __GPD_H__
#define __GPD_H__

extern void (*gpd_tx_interrupt_callback)(void);

extern int gpd_init(void);
extern int gpd_read(char *buf, unsigned int len);
extern int gpd_send(const char *buf, unsigned int len);
extern int gpd_fini(void);

#endif /* __GPD_H__ */
