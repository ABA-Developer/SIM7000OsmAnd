#ifndef __SIM7000_H__
#define __SIM7000_H__

#include <Arduino.h>

/**
 * @brief Communication module with GPS.
 * @param cmd command will be sent into GPS module.
 * @param resp response from GPS module
 * @return bool - True if success or False if fail
*/
bool checkSendCmd(const char *cmd, const char *resp);

/**
 * @brief SIM 7000 initiation
*/
void sim7000Init(void);

/**
 * Get value from a string
 * @param data String data to be shrink
 * @param separator separator
 * @param index maximum index of the data looked for
 * @return data looked for substring
*/
String getValue(String data, char separator, int index);

/**
 * @brief check AT commad
 * @param void
 * @return bool True if success, false if fail
*/
bool checkATCommand(void);

/**
 * @brief Set functionality of SIM7000 module
 * @param void
 * @return bool True if success, false if fail
*/
bool setFunctionality(void);

/**
 * @brief Get SIM7000 IMEI
 * @param void
 * @return String IMEI of the module
*/
String getIMEI(void);

/**
 * @brief Init SIM7000 GGNS Module
 * @param void
 * @return void
*/
void initGGNS(void);

/**
 * @brief Start single IP Mode
 * @param void
 * @return void
*/
void singeIPMode(void);

/**
 * @brief Check SIM Card Status
 * @param void
 * @return void
*/
void checkSIMCard(void);

/**
 * @brief Init SIM7000 GPRS module
 * 
*/
void initGPRS(void);

/**
 * @brief Init SIM7000 Bearer Settings for Applications Based on IP
 * @param void
 * @return void
*/
void initBearer(void);




#endif // __SIM7000_H__