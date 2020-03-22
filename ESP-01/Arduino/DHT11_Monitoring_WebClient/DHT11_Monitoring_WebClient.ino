/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include "DHTesp.h"

#define SERVER_URL "http://192.168.0.40:9000"
#define APP_PATH "/homeAuto/report/TempAndHumi/"


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

#define ESP01_DHT11_IO_PIN 2 //GPIO 2  , LED 1개 짜리 ESP-01은 LED, DTH11 모두 GPIO 2번을 사용한다. 

#define ESP01_BUILTIN_LED_IO_PIN LED_PIN_NO  // in case of TX-LED , Pinnumber is 1 otherwise pinnumber is 2

//==================================================
//Features control
//==================================================
#define REPORT_VIA_LED
#undef  REPORT_VIA_SERIAL

char bufferPayload[8916];
int blinkLED(int period);
ESP8266WiFiMulti WiFiMulti;
DHTesp dht;

float g_humi=0.0;
float g_temp=0.0;


int getInfo(float * temp ,float *humi);
void buildPayload(char * );;


void setup() {
#ifdef REPORT_VIA_SERIAL
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  Serial.println();
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
  dht.setup(ESP01_DHT11_IO_PIN, DHTesp::DHT11);
}

void loop() {


    
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;
#ifdef REPORT_VIA_SERIAL
    Serial.print("[HTTP] begin...\n");
#endif

    getInfo(&g_temp,&g_humi);
    buildPayload(bufferPayload);

#ifdef REPORT_VIA_SERIAL
    Serial.println(bufferPayload);
#endif
    if (http.begin(client, bufferPayload)) {  // HTTP
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
      for(int i=0;i<3;i++)
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
    Serial.printf("Waiting.....Wifi Connection");
#endif
#ifdef REPORT_VIA_LED
    for(int i=0;i<3;i++)
    {
      blinkLED(100);
      delay(50);
    }
#endif 
  }

  delay(10000);
}
int getInfo(float * temp ,float *humi)
{
    int count = 5;
    char *pStatus=NULL;

    do
    {
      delay(dht.getMinimumSamplingPeriod());
      *humi = dht.getHumidity();
      *temp = dht.getTemperature();
      pStatus = ( char *)dht.getStatusString();
      if(pStatus!=NULL)
        if(pStatus[0]=='O' && pStatus[1]=='K')// Data is taken correctly
            return 0;
      count--;
    }while(count!=0);
    if(count==0)
    {
      *temp = -1; 
      *humi = -1;
       return -1;
    }
    return -1;
}

void buildPayload(char * buffer)
{

  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(buffer,"%s%s%x%x%x%x%x%x/DHT11/?temperature=%5.2f&humidity=%5.2f",
                SERVER_URL,APP_PATH,
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
                g_temp,g_humi
                );
}
int blinkLED(int period)
{
#ifdef REPORT_VIA_LED
  digitalWrite(LED_PIN_NO, LED_ON);
  delay(period);
  digitalWrite(LED_PIN_NO, LED_OFF);
#else
  delay(period);
#endif
 
}