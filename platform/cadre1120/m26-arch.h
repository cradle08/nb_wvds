#ifndef _M26_ARCH_H
#define _M26_ARCH_H

int m26_arch_init(void);
int m26_arch_send(uint8_t b);
void m26_arch_turnon(void);
void m26_arch_turnoff(void);
void m26_arch_poweroff(void);
void m26_arch_set_input(int (*input)(unsigned char c));

#endif /* _M26_ARCH_H */
