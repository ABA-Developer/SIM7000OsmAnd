#include "battery.h"

long readBattery(void)
{
  return uint16_t( map(analogRead(BATTERY_PIN), 2215, 2525, 3700, 4200) );
}