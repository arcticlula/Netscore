#ifndef POWER_H
#define POWER_H

#include <stdbool.h>

void set_ldo_enable(bool enable);
void set_ldo_ctrl(bool enable);
void set_vcc_ctrl(bool enable);
void set_buzzer_sleep(bool enable);

#endif /* POWER_H */