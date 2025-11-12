#ifndef LEDS_H
#define LEDS_H

#define CONNECTION_STATUS_LED_GPIO 12

extern void connectionLED_TOGGLE();
extern void connectionLED_ON();
extern void connectionLED_OFF();
extern void loopLEDs();

extern void setupLEDs();

#endif