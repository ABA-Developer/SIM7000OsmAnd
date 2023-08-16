#include <Arduino.h>
#include "sim7000.h"

/**
 * SIM7000 use UART 2.
 * see this for multiple serial com in ESP32 https://gist.github.com/ahmaddidiks/c7ee039eb8abbdda5a3c226fe12662c1
*/

bool checkSendCmd(const char *cmd, const char *resp)
{
  char SIMbuffer[100];
  String response;
  Serial2.write(cmd);
  Serial.println(cmd);
  delay(100);
  if (Serial2.available() > 0)
  {
    response = Serial2.readString();
    Serial.println(response);

    Serial2.flush();
  }
  response.toCharArray(SIMbuffer, response.length() + 1);
  if (NULL != strstr(SIMbuffer, resp))
  {
    return true;
  }
  else
  {
    return false;
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void sim7000Init(void)
{
  Serial2.begin (9600, SERIAL_8N1, 16, 17);
  Serial2.setTimeout(1000);

  digitalWrite(SIM_7000_ENABLE_PIN, LOW);
  delay(500);
  digitalWrite(SIM_7000_ENABLE_PIN, HIGH);
  Serial.println("SIM700 Begin");
}

bool checkATCommand(void)
{
  // Test AT+CPIN: READY
  while (1)
  {
    char SIMbuffer[100];
    uint8_t counter;

    Serial2.write("AT\r\n");
    delay(1);
    String response = Serial2.readString();
    Serial.println(response);
    response.toCharArray(SIMbuffer, response.length() + 1);
    if (NULL != strstr(SIMbuffer, "+CPIN: READY"))
    {
      Serial.println("OK");
      return true;
    }
    else if(counter > 10 )
    {
      return false;
    }
    else
    {
      // Serial.println("ERROR");
      counter++;
      delay(100);
    }
  }
}

bool setFunctionality(void)
{
  return checkSendCmd("AT+CFUN=1\r\n", "0K");
}

String getIMEI(void)
{
  // Get IMEI
  while (1)
  {
    uint8_t counter =0;
    String data[2];
    String IMEI = "";
    Serial2.write("AT+GSN\r\n");
    if (Serial2.available() > 0)
    {
      IMEI = Serial2.readString();
      data[0] = getValue(IMEI, '\n', 0);
      Serial.print("Data1: ");
      Serial.println(data[0]);
      data[1] = getValue(IMEI, '\n', 1);
      Serial.print("IMEI: ");
      Serial.println(data[1]);
      IMEI = data[1];
      Serial.println(IMEI);
      if (IMEI != "ERROR")
      {
        Serial.println("IMEI OK");
        delay(100);
        break;
      }
      else if (counter > 10)
      {
        counter = 10;
        // state = initfirst;
        break;
      }
      else
      {
        Serial.println("ERROR");
        counter--;
        delay(50);
      }
    }
  }
  checkSendCmd("AT+CGNSPWR=0\r\n", "OK");
  delay(100);
}



