/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

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
      #undef USE_SERIAL_SWAP // feature 초기화
#endif

#define CO2_SENSOR_MODULE "MH-Z19B"
#define MH_Z19B_CMD_PACKET_LENGTH 9

#define BUILT_IN_LED_PIN LED_PIN_NO 

//==================================================
//Features control
//==================================================
#define REPORT_VIA_LED
#undef  REPORT_VIA_SERIAL

#ifdef USE_NODE_MCU
 #ifdef REPORT_VIA_SERIAL
  #define USE_SERIAL_SWAP // << NODE MCU의 경우 UART2를 Serial.swap()으로 사용할수 있다. MH-Z19B는 UART Interface를 사용하므로, 
                          // REPORT_VIA_SERIAL 로 UART0을 사용해야한다면 , Serial.swap()을 통해서 UART2를 이용해서 MH-Z19B와 통신한다.
 #endif
#endif

#define SERIAL_BAUD_RATE 9600

#define PACKET_LENGTH 9

//////////////////////////
//Declare variables
char request[]={0xff,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
char response[20]={0,};
char reportMessage[1024]; 
int repeat_period = 5000; // 10 sec

ESP8266WiFiMulti WiFiMulti;

//////////////////////////
//Declare functions
int  blinkLED(int period);
void buildReport(int level);
int  getCO2Level();

#define PROTOCOL "http://"
#define SERVER "192.168.0.40"
#define PORT 9000
#define APP_PATH "/homeAuto/report"

void setup() {
  sprintf(reportMessage,"http://192.168.0.40:9000/homeAuto/report/CO2/122345678/?level=-1&maximum=5000");
#ifdef REPORT_VIA_SERIA
  Serial.begin(SERIAL_BAUD_RATE);
  // Serial.setDebugOutput(true);
  Serial.println();
#else
  #ifdef SERIAL_BAUD_RATE 
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000); // UART에 연결된 디바이스의 초기화시간동안 대기
    Serial.readBytes(reportMessage,1000); // 이것은 timeout동안 input buffer에서 뭐든지 읽어온다.(Timeout 1초)
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

int gCO2Level = 0;
void loop() {


    
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    gCO2Level = getCO2Level();
    if(gCO2Level<0)
    {
      for(int i=0;i<5;i++)
      {
        blinkLED(200);
        delay(50);
      }
      delay(repeat_period);
      return;
    }
    buildReport(gCO2Level);
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
  delay(repeat_period);
}


// /homeAuto/report/bcddc263f2a5/MH-Z19B/CO2/845.00/
void buildReport(int level)
{
  reportMessage;
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(reportMessage,"%s%s:%d%s/%02x%02x%02x%02x%02x%02x/%s/CO2/%.2f/",
                PROTOCOL,
                SERVER,
                PORT,
                APP_PATH,
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
                CO2_SENSOR_MODULE,
                (float)level);
}

int getCO2Level()
{
  static int reset_counter = 5;
  int level = 0;
  int readLength;
  #ifdef USE_SERIAL_SWAP
  Serial.flush();
  delay(100);
  Serial.swap(); // UART0 ->UART 2
  delay(100);
  #endif
  memset(response,0x00,PACKET_LENGTH); //cleanup.
 
  Serial.write(request,PACKET_LENGTH); //send command to MH-Z19B to measure CO2 level.
  delay(100);
  readLength = Serial.readBytes(response,PACKET_LENGTH); // read CO2 level from MH-Z19B.
  if(readLength==PACKET_LENGTH) // Response length must be PACKET_LENGTH (9 BYTE)
  {
    if(response[0]==0xff && response[1]==0x86)
    {
      level = response[2]*256+response[3];
      reset_counter =0; //리셋 카운터 초기화
    }
    else
    {
      level = -2;
      reset_counter--;  // 전원 입력에 의한 노이즈 때문인지, MH-Z18B에서 처음에는 잘못된 패킷이 온다. 
                        // 2번 이상 잘못된 packet이 오면 리셋하자 
      if(reset_counter ==0)
             ESP.restart();
    }
  }
  else
  {
    level = -1;
    
  }


  #ifdef USE_SERIAL_SWAP
  delay(100);
  Serial.swap(); // UART2 ->UART 0
  delay(100);
  #endif
  
  return level;
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
