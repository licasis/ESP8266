/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/


/*
 * 
 * PMS5003 PACKET from Serial 9600bps Check bit：None Stop bit：1 bit 
42 4D - Frame Start
00 1C - frame length
00 08 - DATA1 PM1.0
00 0C - DATA2 PM2.5
00 0C - DATA3 PM10
00 08 - DATA4 PM1.0
00 0C - DATA5 PM2.5
00 0C - DATA6 concentration unit
07 62 - DATA7 indicates the number of particles with diameter beyond 0.3 um in 0.1 L of air.
02 05 - DATA8 indicates the number of particles with diameter beyond 0.5 um in 0.1 L of air.
00 39 - DATA9 indicates the number of particles with diameter beyond 1.0 um in 0.1 L of air.
00 01 - Data 10 indicates the number of particles with diameter beyond 2.5 um in 0.1 L of air.
00 01 - Data 11 indicates the number of particles with diameter beyond 5.0 um in 0.1 L of air.
00 00 - Data 12 indicates the number of particles with diameter beyond 10 um in 0.1 L of air.
98 00 - RESERVED
02 2E - CHECKSUM
*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#define PROTOCOL "http://"
#define SERVER "192.168.0.40"
#define PORT 9000
#define APP_PATH "homeAuto/report"

#undef USE_ESP_01
#define  USE_NODE_MCU

#ifdef USE_ESP_01
  #ifdef  USE_NODE_MCU
    #error YOU CAN NOT DEFINE BOTH USE_ESP_01 AND USE_NODE_MCU
  #endif
#endif

#ifdef USE_NODE_MCU
  #ifdef USE_ESP_01 
    #error YOU CAN NOT DEFINE BOTH USE_ESP_01 AND USE_NODE_MCU
  #endif
#endif

#ifdef USE_ESP_01
      #define GPIO0 0 //ESP01 GPIO0 PIN
      #define GPIO1 1 //ESP01 TX PIN
      #define GPIO2 2 //ESP01 GPIO2 PIN
      #define GPIO3 3 //ESP01 RX PIN
    
      #define TX_PIN GPIO1
      #define RX_PIN GPIO3
      
      #define ESP01_2_LEDs_TYPE
      // #define ESP01_1_LEDs_TYPE
      
      #ifdef ESP01_2_LEDs_TYPE
      #define LED_PIN_NO GPIO1 // ESP01 2 LED type은 LED가 GPIO 1(TX)과 연결 되어있다
      #endif
      
      #ifdef ESP01_1_LEDs_TYPE
      #define LED_PIN_NO GPIO2 // ESP01 1 LED type은 LED가 GPIO 2과 연결 되어있다
      #endif
      #define LED_ON 0
      #define LED_OFF 1
#endif

#ifdef USE_NODE_MCU
      #define GPIO0 0 //NODE MCU GPIO0 PIN
      #define GPIO1 1 //NODE MCU TX0 PIN
      #define GPIO2 2 //NODE MCU BUILT_IN_LED
      #define GPIO3 3 //NODE MCU RX0 PIN
      
      #define TX_PIN GPIO1
      #define RX_PIN GPIO3
      
      #define LED_PIN_NO GPIO2 // NODEMCU 1 LED type은 LED가 GPIO 2과 연결 되어있다
      
      #define LED_ON 0
      #define LED_OFF 1
      #define USE_SERIAL_SWAP // feature 초기화
#endif

#define DUST_SENSOR_MODULE "PMS5003"
#define PACKET_FRAME_LENGTH 32

#define BUILT_IN_LED_PIN LED_PIN_NO 

//==================================================
//Features control
//==================================================
#define REPORT_VIA_LED
#define  REPORT_VIA_SERIAL

#ifdef USE_NODE_MCU
 #ifdef REPORT_VIA_SERIAL
  #define USE_SERIAL_SWAP // << NODE MCU의 경우 UART2를 Serial.swap()으로 사용할수 있다. MH-Z19B는 UART Interface를 사용하므로, 
                          // REPORT_VIA_SERIAL 로 UART0을 사용해야한다면 , Serial.swap()을 통해서 UART2를 이용해서 MH-Z19B와 통신한다.
 #endif
#endif

#define SERIAL_BAUD_RATE 9600

//////////////////////////
//Declare variables
#define PM_10 0// PM1.0
#define PM_25  1// PM2.5
int gPMValue[2]={0,0};
char response[64]={0,};
char reportMessage[1024]; 
int repeat_period = 5000; // 10 sec

ESP8266WiFiMulti WiFiMulti;

//////////////////////////
//Declare functions
int  blinkLED(int period);
void buildReport(int level);
int  getPMLevel();

#define PROTOCOL "http://"
#define SERVER "192.168.0.40"
#define PORT 9000
#define APP_PATH "/homeAuto/report"

void setup() {
  sprintf(reportMessage,"http://192.168.0.40:9000/homeAuto/report/CO2/122345678/?level=-1&maximum=5000");
#ifdef REPORT_VIA_SERIA
  Serial.begin(SERIAL_BAUD_RATE);
  // Serial.setDebugOutput(true);
  Serial. setTimeout(300);
  Serial.println();
#else
  #ifdef SERIAL_BAUD_RATE 
    Serial.begin(SERIAL_BAUD_RATE);
    Serial. setTimeout(300);
    delay(1000); // UART에 연결된 디바이스의 초기화시간동안 대기
    Serial.readBytes(reportMessage,300); // 이것은 timeout동안 input buffer에서 뭐든지 읽어온다.(Timeout 1초)
                                          // UART에 디바이스가 초기화 되면서 보낸 노이즈성 UART 신호를 제거하는 효과.
  #endif
#endif

#ifdef REPORT_VIA_LED
  pinMode(LED_PIN_NO, OUTPUT);
  for(int i=0;i<2;i++)
  {
    blinkLED(2000);
    delay(500);
  }
#endif

  for (uint8_t t = 4; t > 0; t--) {
#ifdef REPORT_VIA_SERIAL
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
#endif
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("WirelessWorld_11n", "kimhaksoo");
}
int selector = -1;
void loop() {

  

    
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    
  // wait for WiFi connection
  if(selector == -1)
  {
#ifdef REPORT_VIA_SERIAL
    Serial.printf("Reading...\n");
    Serial.flush();
#endif
    getPMLevel();
    selector++;
#ifdef REPORT_VIA_SERIAL
    Serial.printf("PMValue[%d,%d]...\n",gPMValue[0],gPMValue[1]);
    Serial.flush();
#endif
  }
  if((gPMValue[PM_10]< 0) || (gPMValue[PM_25] < 0))
  {
#ifdef REPORT_VIA_SERIAL
    Serial.printf("Invalid PM Value, reset selector \n");
    Serial.flush();
#endif
    selector = -1;
    return;
  }
  else
  {
    buildReport(selector);    
  }
  

    WiFiClient client;

    HTTPClient http;
#ifdef REPORT_VIA_SERIAL
    Serial.print("[HTTP] begin...\n");
#endif

    

#ifdef REPORT_VIA_SERIAL
    Serial.println(reportMessage);
#endif
    if (http.begin(client, reportMessage)) {  // HTTP
#ifdef REPORT_VIA_SERIAL
      Serial.print("[HTTP] GET...\n");
#endif

      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
#ifdef REPORT_VIA_SERIAL
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
#endif

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
#ifdef REPORT_VIA_SERIAL
          Serial.println(payload);
#endif

        }
      } else {
#ifdef REPORT_VIA_SERIAL
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
#endif
      }

      http.end();
      for(int i=0;i<2;i++)
      {
        blinkLED(100);
        delay(50);
      }  
    } 
    else 
    {
#ifdef REPORT_VIA_SERIAL
      Serial.printf("[HTTP} Unable to connect\n");
#endif
#ifdef REPORT_VIA_LED
      for(int i=0;i<4;i++)
      {
        blinkLED(100);
        delay(50);
      }
#endif
    }
  }
  else
  {
#ifdef REPORT_VIA_SERIAL
    Serial.println("Waiting.....Wifi Connection");
#endif
#ifdef REPORT_VIA_LED
    for(int i=0;i<5;i++)
    {
      blinkLED(100);
      delay(50);
    }
#endif 
  }
  if(selector==PM_25)
  {
    delay(repeat_period);
    selector = -1;
  }
  else
  {
    delay(2000);
    selector++;
  }
}


// /homeAuto/report/bcddc263f2a5/PMS5003/PM1.0/845.00/
// /homeAuto/report/bcddc263f2a5/PMS5003/PM2.5/845.00/
void buildReport(int type)
{
  reportMessage;
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(reportMessage,"%s%s:%d%s/%02x%02x%02x%02x%02x%02x/%s/%s/%.2f/",
                PROTOCOL,
                SERVER,
                PORT,
                APP_PATH,
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],DUST_SENSOR_MODULE,
                (type==PM_10)?"PM1.0":"PM2.5",
                (float)gPMValue[type]);
}
int getPMLevel()
{
  static int reset_counter = 5;
  int level = 0;
  int readLength;
  #ifdef USE_SERIAL_SWAP
  Serial.flush();
  delay(10);
  Serial.swap(); // UART0 ->UART 2
  delay(10);
  #endif
  int count = 10;
  do
  {
    readLength = Serial.readBytes(response,PACKET_FRAME_LENGTH); // read data frame from PMS5003
    if(readLength==PACKET_FRAME_LENGTH) // Response length must be PACKET_FRAME_LENGTH (9 BYTE)
    {
        if(response[0]==0x42 && response[1]==0x4D)
        {
          gPMValue[PM_10] = response[4]*256 + response[5];//PM1.0
          gPMValue[PM_25] = response[6]*256 + response[7];//PM2.5
          break;
        }
    }
    delay(50);
    gPMValue[PM_10] = -1;
    gPMValue[PM_25] = -1;
  }while(count--);

  #ifdef USE_SERIAL_SWAP
  Serial.swap(); // UART2 ->UART 0
  delay(100);
  #endif

  {
    //Serial.printf("[0x%02x 0x%02x 0x%02x 0x%02x]\n",response[0],response[1],response[2],response[3]);
    //Serial.flush();
  }

}

 
int blinkLED(int period)
{
#ifdef REPORT_VIA_LED
  digitalWrite(LED_PIN_NO, LED_OFF);
  delay(10);
  digitalWrite(LED_PIN_NO, LED_ON);
  delay(period);
  digitalWrite(LED_PIN_NO, LED_OFF);
#else
  delay(period);
#endif
 
}
