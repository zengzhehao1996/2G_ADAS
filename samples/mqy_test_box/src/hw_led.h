#ifndef HW_LED_H
#define HW_LED_H

#ifdef __cplusplus
extern "C" {
#endif

void led_on(void);
void led_off(void);
void led_init(void);
extern unsigned char led_stat;

#ifdef __cplusplus
}
#endif
#endif // HW_LED_H
