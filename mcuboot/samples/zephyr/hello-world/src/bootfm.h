#ifndef __BOOTFM_H__
#define __BOOTFM_H__

extern int bootfm_init(void);
extern int bootfm_flash_read(uint32_t off, void *dst, uint32_t len);
extern int bootfm_flash_write(uint32_t off, const void *src, uint32_t len);
extern int bootfm_check_magic(uint32_t off);
extern int bootfm_set_confirmed(void);

#endif /* __BOOTFM_H__ */
