#ifndef POWER_H
#define POWER_H

#include <stdbool.h>

void init_power();
void init_usb_interrupt();
bool is_usb_connected();
void set_ldo_enable(bool enable);
void set_ldo_ctrl(bool enable);
void set_vcc_ctrl(bool enable);
void set_buzzer_sleep(bool enable);

void go_to_sleep();
void wake_up();
bool is_system_sleeping();

#endif /* POWER_H */