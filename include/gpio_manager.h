#ifndef GPIO_MANAGER_H
#define GPIO_MANAGER_H

#include "globals.h"

const PinConfig* findPin(uint8_t gpio);
void writePin(uint8_t gpio, int value);
void restorePinStates();
void setupGPIO();

#endif // GPIO_MANAGER_H
