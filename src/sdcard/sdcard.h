#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <Arduino.h>

/***
 * @brief Init the SD Card
*/
void initsdcard(void);

/**
 * @brief Logger, the logs will be written in SD card
 * @param log log messages
*/
void writeLog(const char *log);
#endif // __SDCARD_H__