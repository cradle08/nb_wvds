#ifndef _QMC5883_H
#define _QMC5883_H

typedef void (*qmc5883_callback_t)(unsigned char* buf, unsigned char* tmp);

int qmc5883_init(void);
void qmc5883_sample(void);
void qmc5883_self_test(void);
void qmc5883_idle(void);
void qmc5883_sample_read(unsigned char gain);
void qmc5883_set_callback(qmc5883_callback_t cback);
int qmc5883_get_temperature(void);

#endif /* _QMC5883_H */
