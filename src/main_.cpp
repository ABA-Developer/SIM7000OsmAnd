//#include <Arduino.h>
#include <HardwareSerial.h>
#include <UnixTime.h>
#include <iostream>
#include <stdlib.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>

using namespace std;

#define SEND_INTERVAL_SLEEP 300

int YEAR, MONTH, DAY, HOUR, MINUTE, SECONDS;

UnixTime stamp(0);
HardwareSerial SerialPort(2); // use UART2

String response = "0";
int send = 0;
uint32_t unix;

File myFile;

// change this to match your SD shield or module;
const int chipSelect = 5;

int counter = 10;
int countgps = 5;
int counterror = 10;
long di1 = 0;
long batt = 0;
long accu = 0;
bool gpsin = 0;
bool bearercon = 0;
bool sendok = 0;
bool chargestate = 0;
int milliscount = 0;

enum statenow
{
  initfirst,
  initgprs,
  initbearer,
  initgpss,
  getgps,
  sendgps,
  sendsleep
};

statenow state;

bool checkSendCmd(const char *cmd, const char *resp)
{
  char SIMbuffer[100];
  String response;
  SerialPort.write(cmd);
  Serial.println(cmd);
  delay(100);
  if (SerialPort.available() > 0)
  {
    response = SerialPort.readString();
    Serial.println(response);

    SerialPort.flush();
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

void firstinit()
{
  digitalWrite(27, LOW);
  delay(500);
  digitalWrite(27, HIGH);
  Serial.println("Begin");

  // Test AT+CPIN: READY
  while (1)
  {
    char SIMbuffer[100];
    SerialPort.write("AT\r\n");
    delay(500);
    String response = SerialPort.readString();
    Serial.println(response);
    response.toCharArray(SIMbuffer, response.length() + 1);
    if (NULL != strstr(SIMbuffer, "+CPIN: READY"))
    {
      Serial.println("OK");
      delay(10);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      // Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }
  delay(200);

  // Set Functionality
  checkSendCmd("AT+CFUN=1\r\n", "0K");

  // Get IMEI
  while (1)
  {
    String data[2];
    String IMEI = "";
    SerialPort.write("AT+GSN\r\n");
    if (SerialPort.available() > 0)
    {
      IMEI = SerialPort.readString();
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
      else if (counter < 0)
      {
        counter = 10;
        state = initfirst;
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

  // Init GNSS
  while (1)
  {

    if (checkSendCmd("AT+CGNSPWR=1\r\n", "OK"))
    {
      Serial.println("GNSS OK");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  checkSendCmd("AT+CGNSPWR?\r\n", "1");

  // Init GNSS
  SerialPort.write("AT+CGNSINF\r\n");

  // Start Single IP Mode
  while (1)
  {
    if (checkSendCmd("AT+CIPMUX=0\r\n", "OK"))
    {
      Serial.println("CIPMUX OK");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  // Check SIM Card Status
  while (1)
  {
    if (checkSendCmd("AT+CPIN?\r\n", "READY"))
    {
      Serial.println("SIM OK");
      state = initgprs;
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }
}

void gprsinit()
{
  // Check Signal Quality
  checkSendCmd("AT+CSQ\r\n", "OK");

  // Set Mode
  // AT+CNMP=<mode>
  // 2 = Auto
  //   13 = GSM Only
  //   38 = LTE Only
  //   51 = GSM and LTE Only
  while (1)
  {
    if (checkSendCmd("AT+CNMP=13\r\n", "OK"))
    {
      Serial.println("Mode=GPRS");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  // Preferred Selection between CAT-M and NB-IoT

  while (1)
  {
    if (checkSendCmd("AT+CMNB=1\r\n", "OK"))
    {
      Serial.println("CAT-M");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  checkSendCmd("AT+CGATT=0\r\n", "OK");
  delay(200);
  // Attach or Detach from GPRS Service
  while (1)
  {
    if (checkSendCmd("AT+CGATT=1\r\n", "OK"))
    {
      Serial.println("Attached");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  // Start Task and Set APN, USER NAME, PASS
  while (1)
  {
    char SIMbuffer[100];
    SerialPort.write("AT+CSTT=\"M2MAUTOTRONIC\"\r\n");
    delay(500);
    if (SerialPort.available() > 0)
    {
      String response = SerialPort.readString();
      Serial.println(response);
      response.toCharArray(SIMbuffer, response.length() + 1);
      if (NULL != strstr(SIMbuffer, "OK"))
      {
        Serial.println("APN OK");
        delay(100);
        break;
      }
      else if (counter < 0)
      {
        counter = 10;
        state = initfirst;
        break;
      }
      else
      {
        Serial.println("ERROR");
        counter--;
        delay(100);
      }
    }
  }

  // Bring Up Wireless Connection with GPRS
  while (1)
  {
    if (checkSendCmd("AT+CIICR\r\n", "OK"))
    {
      Serial.println("GPRS OK");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  while (1)
  {
    char SIMbuffer[100];
    SerialPort.write("AT+CIFSR\r\n");
    delay(100);
    response = SerialPort.readString();
    Serial.println(response);
    response.toCharArray(SIMbuffer, response.length() + 1);
    if (NULL != strstr(SIMbuffer, "0.0.0.0"))
    {

      Serial.println("ERROR");
      counter--;
      delay(100);
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("IP Connected");
      state = initbearer;
      delay(100);
      break;
    }
  }
}
void bearerinit()
{
  //  Bearer Settings for Applications Based on IP

  if (bearercon == 1)
  {
    while (1)
    {
      if (checkSendCmd("AT+SAPBR=0,1\r\n", "OK"))
      {
        Serial.println("GPRS Conf Closed");
        bearercon = 0;
        delay(100);
        break;
      }
      else if (counter < 0)
      {
        counter = 10;
        state = initfirst;
        break;
      }
      else
      {
        Serial.println("ERROR");
        counter--;
        delay(100);
      }
    }
  }

  while (1)
  {
    if (checkSendCmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n", "OK"))
    {
      Serial.println("GPRS Conf Ok");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  while (1)
  {
    if (checkSendCmd("AT+SAPBR=3,1,\"APN\",\"M2MAUTOTRONIC\"\r\n", "OK"))
    {
      Serial.println("APN Conf Ok");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  while (1)
  {
    if (checkSendCmd("AT+SAPBR=1,1\r\n", "OK"))
    {
      Serial.println("Bearer Connected");
      delay(100);
      break;
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("ERROR");
      counter--;
      delay(100);
    }
  }

  while (1)
  {
    char SIMbuffer[100];
    SerialPort.write("AT+SAPBR=2,1\r\n");
    delay(100);
    response = SerialPort.readString();
    Serial.println(response);
    response.toCharArray(SIMbuffer, response.length() + 1);
    if (NULL != strstr(SIMbuffer, "0.0.0.0"))
    {

      Serial.println("ERROR");
      counter--;
      delay(100);
    }
    else if (counter < 0)
    {
      counter = 10;
      state = initfirst;
      break;
    }
    else
    {
      Serial.println("Bearer Connected2");
      bearercon = 1;
      state = sendgps;
      delay(100);
      break;
    }
  }
  delay(100);
}

// void gpsinit()
// {
//   checkSendCmd("AT+CGNSPWR=0\r\n", "OK");
//   delay(100);

//   // Init GNSS
//   while (1)
//   {

//     if (checkSendCmd("AT+CGNSPWR=1\r\n", "OK"))
//     {
//       Serial.println("GNSS OK");
//       delay(100);
//       break;
//     }
//     else if (counter < 0)
//     {
//       counter = 10;
//       state = initfirst;
//       break;
//     }
//     else
//     {
//       Serial.println("ERROR");
//       counter--;
//       delay(100);
//     }
//   }

//   checkSendCmd("AT+CGNSPWR?\r\n", "1");

//   // Init GNSS
//   SerialPort.write("AT+CGNSINF\r\n");
//   // while (1)
//   // {
//   //   if (gpsin == 1)
//   //   {
//   //     if (checkSendCmd("AT+CGNSMOD=1,1,1,1\r\n", "OK"))
//   //     {
//   //       Serial.println("MOD OK");
//   //       gpsin = 0;
//   //       state = sendgps;
//   //       delay(100);
//   //       break;
//   //     }
//   //     else if (counter < 0)
//   //     {
//   //       counter = 10;
//   //       gpsin = 0;
//   //       state = initfirst;
//   //       break;
//   //     }
//   //     else
//   //     {
//   //       Serial.println("ERROR");
//   //       counter--;
//   //       delay(100);
//   //     }
//   //   }
//   //   else
//   //   {
//   //     if (checkSendCmd("AT+CGNSMOD=1,1,1,1\r\n", "OK"))
//   //     {
//   //       Serial.println("MOD OK");
//   //       state = initgprs;
//   //       delay(100);
//   //       break;
//   //     }
//   //     else if (counter < 0)
//   //     {
//   //       counter = 10;
//   //       state = initfirst;
//   //       break;
//   //     }
//   //     else
//   //     {
//   //       Serial.println("ERROR");
//   //       counter--;
//   //       delay(100);
//   //     }
//   //   }
//   // }
// }
void gpssend()
{

  String data[22];
  String latitude = "";
  String longitude = "";
  bool para = 0;
  String gnss = "";
  String times = "";
  String buff = "";
  String cno = "";
  String chars= "";
  if (send == 0)
  {
    checkSendCmd("AT+HTTPINIT\r\n", "OK");
  }
  // Init HTTP
  int datalenght;
  char bufferunix[11];
  // 1689420424
  char buffer[33];
  char SIMbuffer[100];
  di1 = analogRead(34);
  di1 = map(di1, 485, 2975, 5000, 25000);
  accu = analogRead(35);
  accu = map(accu, 485, 2975, 5000, 25000);
  batt = analogRead(25);
  batt = map(batt, 2215, 2525, 3700, 4200);
  chars = String(chargestate);
  Serial.print("di1: ");
  Serial.println(di1);
  Serial.print("batt: ");
  Serial.println(batt);
  Serial.print("accu: ");
  Serial.println(accu);
  SerialPort.write("AT+CGNSINF\r\n");
  send = 1;
  delay(500);
  if (SerialPort.available() > 0)
  {
    gnss = SerialPort.readString();
  }
  delay(1500);
  state = sendgps;
  gnss.toCharArray(SIMbuffer, gnss.length() + 1);
  if (NULL != strstr(SIMbuffer, "OK"))
  {
    Serial.println(gnss);
    data[0] = getValue(gnss, ',', 0);
    data[1] = getValue(gnss, ',', 1);
    data[2] = getValue(gnss, ',', 2);
    data[3] = getValue(gnss, ',', 3);
    data[4] = getValue(gnss, ',', 4);
    data[5] = getValue(gnss, ',', 5);
    data[6] = getValue(gnss, ',', 6);
    data[7] = getValue(gnss, ',', 7);
    data[8] = getValue(gnss, ',', 8);
    data[9] = getValue(gnss, ',', 9);
    data[10] = getValue(gnss, ',', 10);
    data[11] = getValue(gnss, ',', 11);
    data[12] = getValue(gnss, ',', 12);
    data[13] = getValue(gnss, ',', 13);
    data[14] = getValue(gnss, ',', 14);
    data[15] = getValue(gnss, ',', 15);
    data[16] = getValue(gnss, ',', 16);
    data[17] = getValue(gnss, ',', 17);
    data[18] = getValue(gnss, ',', 18);
    data[19] = getValue(gnss, ',', 19);
    data[20] = getValue(gnss, ',', 20);
    // Serial.print("Init: ");
    // Serial.println(data[0]);
    // Serial.print("Fix: ");
    // Serial.println(data[1]);
    // Serial.print("Time: ");
    // Serial.println(data[2]);
    // Serial.print("Latitude: ");
    // Serial.println(data[3]);
    // Serial.print("Longitude: ");
    // Serial.println(data[4]);
    // Serial.print("Altitude: ");
    // Serial.println(data[5]);
    // Serial.print("Speed: ");
    // Serial.println(data[6]);
    // Serial.print("Angle: ");
    // Serial.println(data[7]);
    // Serial.print("Fix Mode: ");
    // Serial.println(data[8]);
    // Serial.print("Reserved1: ");
    // Serial.println(data[9]);
    // Serial.print("HDOP: ");
    // Serial.println(data[10]);
    // Serial.print("PDOP: ");
    // Serial.println(data[11]);
    // Serial.print("VDOP: ");
    // Serial.println(data[12]);
    // Serial.print("Reserved2: ");
    // Serial.println(data[13]);
    // Serial.print("GNSS: ");
    // Serial.println(data[14]);
    // Serial.print("GPS: ");
    // Serial.println(data[15]);
    // Serial.print("GLONASS: ");
    // Serial.println(data[16]);
    // Serial.print("Reserved3: ");
    // Serial.println(data[17]);
    // Serial.print("CNO max: ");
    // Serial.println(data[18]);
    // Serial.print("HPA: ");
    // Serial.println(data[19]);
    // Serial.print("VPA: ");
    // Serial.println(data[20]);
    // Serial.print("countgps: ");
    // Serial.println(countgps);
  }

  buff = String(data[2].charAt(0)) + String(data[2].charAt(1)) + String(data[2].charAt(2)) + String(data[2].charAt(3));
  int year;
  buff.toCharArray(buffer, buff.length() + 1);
  year = atoi(buffer);
  // Serial.println(buff);
  buff = String(data[2].charAt(4)) + String(data[2].charAt(5));
  int month;
  buff.toCharArray(buffer, buff.length() + 1);
  month = atoi(buffer);
  // Serial.println(month);
  buff = String(data[2].charAt(6)) + String(data[2].charAt(7));
  int day;
  buff.toCharArray(buffer, buff.length() + 1);
  day = atoi(buffer);
  // Serial.println(day);
  buff = String(data[2].charAt(8)) + String(data[2].charAt(9));
  int hour;
  buff.toCharArray(buffer, buff.length() + 1);
  hour = atoi(buffer);
  // Serial.println(hour);
  buff = String(data[2].charAt(10)) + String(data[2].charAt(11));
  int minute;
  buff.toCharArray(buffer, buff.length() + 1);
  minute = atoi(buffer);
  // Serial.println(minute);
  buff = String(data[2].charAt(12)) + String(data[2].charAt(13));
  int sec;
  buff.toCharArray(buffer, buff.length() + 1);
  sec = atoi(buffer);
  // Serial.println(sec);
  stamp.setDateTime(year, month, day, hour, minute, sec);
  unix = stamp.getUnix();
  Serial.print("Unix: ");
  Serial.println(unix);
  sprintf(bufferunix, "%d", unix);

  if (data[1] == "0")
  {
    send = 2;
  }
  delay(500);
  if (data[1] == "1")
  {
    digitalWrite(2, HIGH);
    // String sending = "AT+HTTPPARA=\"URL\",\"http://194.233.80.108:6055/?id=865234033676973&lat=-7.060677&lon=110.447651&timestamp=1689403987\""
    String sending = "AT+HTTPPARA=\"URL\",\"http://194.233.80.108:6055/?id=865234033676973&lat=" + String(data[3]) + "&lon=" + String(data[4]) + "&timestamp=" + String(bufferunix) + "&Altitude=" + String(data[5]) + "&Speed=" + String(data[6]) + "&Angle=" + String(data[7]) + "&GNSS=" + String(data[14]) + "&HDOP=" + String(data[10]) + "&di1=" + String(di1) + "&batt=" + String(batt) + "&accu=" + String(accu) + "&charge=" + chars + '"' + "\r\n";
    char sendit[300];
    sending.toCharArray(sendit, sending.length() + 1);
    // Serial.println(sendit);
    String logsd = "";
    String sendingsd = "865234033676973&" + String(data[3]) + "&" + String(data[4]) + "&" + String(bufferunix) + "&" + String(data[5]) + "&" + String(data[6]) + "&" + String(data[7]) + "&" + String(data[14]) + "&" + String(data[10]);
    String httppara = "";
    String httpact = "0";
    SerialPort.write(sendit);
    delay(400);

    while (1)
    {
      if (SerialPort.available())
      {
        response = SerialPort.readString();
        response.toCharArray(SIMbuffer, response.length() + 1);
        Serial.println(response);
        if (NULL != strstr(SIMbuffer, "ERROR"))
        {
          httppara = "ERROR";
          para = 0;
          checkSendCmd("AT+CIPCLOSE\r\n", "OK");
          state = initgprs;
          break;
        }
        else
        {
          para = 1;
          httppara = "OK";
          break;
        }
      }
    }
    int count = 20;
    if (para == 1)
    {
      String statusdata[2];
      SerialPort.flush();
      SerialPort.write("AT+HTTPACTION=0\r\n");
      while (1)
      {
        delay(800);
        if (SerialPort.available() > 0)
        {
          response = SerialPort.readString();
          statusdata[0] = getValue(response, ',', 0);
          Serial.print("Data1: ");
          Serial.println(statusdata[0]);
          statusdata[1] = getValue(response, ',', 1);
          Serial.print("status: ");
          Serial.println(statusdata[1]);
          httpact = statusdata[1];
          Serial.println(httpact);
        }
        Serial.println(response);
        response.toCharArray(SIMbuffer, response.length() + 1);
        if (NULL != strstr(SIMbuffer, "200"))
        {
          httpact = "200";
          Serial.println("SEND");
          SerialPort.write("AT+HTTPREAD\r\n");
          delay(200);
          digitalWrite(2, LOW);
          // Stop TCP or UDP Connection
          checkSendCmd("AT+HTTPTERM\r\n", "OK");
          send = 0;
          sendok = 1;
          state = sendgps;
          di1 = analogRead(34);
          if (di1 <= 300)
          {
          //   delay(6000);
          //   unix = unix + 5;
          //   batt--;
          //   sprintf(bufferunix, "%d", unix);
          //   delay(1000);
          //   checkSendCmd("AT+HTTPINIT\r\n", "OK");
          //   delay(1000);
          //   sending = "AT+HTTPPARA=\"URL\",\"http://194.233.80.108:6055/?id=865234033676973&lat=" + String(data[3]) + "&lon=" + String(data[4]) + "&timestamp=" + String(bufferunix) + "&Altitude=" + String(data[5]) + "&Speed=0&Angle=" + String(data[7]) + "&GNSS=" + String(data[14]) + "&HDOP=" + String(data[10]) + "&di1=" + String(di1) + "&batt=" + String(batt) + "&accu=" + String(accu) + '"' + "\r\n";
          //   sending.toCharArray(sendit, sending.length() + 1);
          //   SerialPort.write(sendit);
          //   Serial.println(sendit);
          //   int counting = 10;
          //   while (1)
          //   {
          //     delay(1000);
          //     response = SerialPort.readString();
          //     response.toCharArray(SIMbuffer, response.length() + 1);
          //     Serial.println(response);
          //     if (NULL != strstr(SIMbuffer, "ERROR"))
          //     {
          //       counting--;
          //     }
          //     else if (counting <= 0)
          //     {
          //       break;
          //     }
          //     else
          //     {
          //       counting = 10;
          //       break;
          //     }
          //   }

          //   Serial.println(response);
          //   SerialPort.write("AT+HTTPACTION=0\r\n");
          //   while (1)
          //   {
          //     delay(1000);
          //     response = SerialPort.readString();
          //     response.toCharArray(SIMbuffer, response.length() + 1);
          //     Serial.println(response);
          //     if (NULL != strstr(SIMbuffer, "200"))
          //     {
          //       counting = 10;
          //       break;
          //     }
          //     else if (counting <= 0)
          //     {
          //       break;
          //     }
          //     else
          //     {
          //       counting--;
          //     }
          //   }
            // SerialPort.write("AT+HTTPREAD\r\n");
            // delay(1000);
            // response = SerialPort.readString();
            // Serial.println(response);
            // SerialPort.write("AT+HTTPTERM\r\n");
            // delay(1000);
            // response = SerialPort.readString();
            // bearercon = 0;
            state = sendsleep;
            Serial.println("sleep");
          }
          break;
        }
        else if (NULL != strstr(SIMbuffer, "601"))
        {
          httpact = "601";
          Serial.println("601");
          delay(200);
          send = 2;
          para = 0;
          checkSendCmd("AT+CIPCLOSE\r\n", "OK");
          state = initgprs;
          break;
        }
        else if (NULL != strstr(SIMbuffer, "604"))
        {
          httpact = "604";
          Serial.println("604");
          delay(200);
          break;
        }
        else if (count <= 0)
        {
          Serial.println("ERROR");
          SerialPort.write("AT+HTTPACTION=0\r\n");
          delay(200);
          send = 2;
          break;
        }
        // else if (NULL != strstr(SIMbuffer, "604"))
        // {
        //   Serial.println("604");
        //   delay(200);
        //   send = 2;
        //   state = sendgps;
        //   break;
        // }
        else
        {
          count--;
          delay(100);
        }
      }
    }

    Serial.println("Next");

    myFile = SD.open("/directory/datalog.csv", FILE_APPEND);
    if (myFile)
    {
      logsd = sendingsd + "&" + httppara + "&" + httpact;
      myFile.println(logsd);
      Serial.print("Writing to test.txt...");
      myFile.close();
      Serial.println("done log");
    }
    else
    {
      // if the file didn't open, print an error:
      Serial.println("error opening datalog.txt");
    };
    // }
    // else
    // {
    //   delay(2000);
    // }
  }
}

void initsdcard()
{
  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect))
  {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}

void sleepmode()
{
  digitalWrite(27, LOW);
  di1 = analogRead(34);
  if (di1 > 300)
  {
    state = initfirst;
  }
  if (milliscount >= 300)
  {
    state = initfirst;
    Serial.println("exit sleep");
    // save the last time you blinked the LED
    milliscount = 0;
  }
  milliscount++;
  delay(1000);
}

void setup()
{
  pinMode(27, OUTPUT); // restart sim
  pinMode(22, OUTPUT); //charging
  pinMode(2, OUTPUT);  //  
  pinMode(34, INPUT); // di1
  pinMode(35, INPUT); // accu
  pinMode(25, INPUT); // batt

  Serial.begin(115200);
  initsdcard();
  SerialPort.begin(9600, SERIAL_8N1, 16, 17);
  SerialPort.setTimeout(1000);
  state = initfirst;
}
void loop()
{
  batt = analogRead(25);
  batt = map(batt, 2215, 2525, 3700, 4200);
  if(batt <= 3700 && chargestate == 0)
  {
    Serial.println("charging");
    chargestate = 1;
    digitalWrite(22, HIGH);
  }
  else if(batt >= 4100 && chargestate == 1)
  {
    Serial.println("discharging");
    chargestate = 0;
    digitalWrite(22, LOW);
  }
  
  switch (state)
  {
  case sendsleep:
    sleepmode();
    break;
  case initfirst:
    Serial.println("initfirst");
    firstinit();
    break;
  // case initgpss:
  //   Serial.println("initgps");
  //   gpsinit();
  //   break;
  case initgprs:
    Serial.println("initgprs");
    gprsinit();
    break;
  case initbearer:
    Serial.println("initbearer");
    bearerinit();
    break;
  case sendgps:
    Serial.println("sendgps");
    gpssend();
    break;
  default:

    break;
  }
}
