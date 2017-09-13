#ifndef _BUTTON_SENSOR_H
#define _BUTTON_SENSOR_H

#include "lib/sensors.h"

#define BUTTON1_PORT(reg)  P1##reg
#define BUTTON1_PIN        0

#define BUTTON2_PORT(reg)  P1##reg
#define BUTTON2_PIN        1

extern const struct sensors_sensor button1_sensor;
extern const struct sensors_sensor button2_sensor;

#endif /* _BUTTON_SENSOR_H */
