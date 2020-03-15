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
#define ESP01_DHT11_IO_PIN 2 //GPIO 2  , LED 1개 짜리 ESP-01은 LED, DTH11 모두 GPIO 2번을 사용한다. 


char bufferPayload[8916];

ESP8266WiFiMulti WiFiMulti;
DHTesp dht;

float g_humi=0.0;
float g_temp=0.0;


int getInfo(float * temp ,float *humi);
void buildPayload(char * );;


void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
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

    Serial.print("[HTTP] begin...\n");
    getInfo(&g_temp,&g_humi);
    buildPayload(bufferPayload);
    Serial.println(bufferPayload);

    if (http.begin(client, bufferPayload)) {  // HTTP
      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
  else
  {
    Serial.printf("Waiting.....Wifi Connection");
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
  sprintf(buffer,"%s%s%x%x%x%x%x%x/?temperature=%5.2f&humidity=%5.2f",
                SERVER_URL,APP_PATH,
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
                g_temp,g_humi
                );
}
