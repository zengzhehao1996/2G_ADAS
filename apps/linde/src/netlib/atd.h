#ifndef __ATD_H__
#define __ATD_H__

extern int atd_init(void);
extern int atd_read(char *buf, int len);
extern void atd_clean(void);
extern int atd_write(const char *buf, int len, int is_cmd);

#endif /* __ATD_H__ */
