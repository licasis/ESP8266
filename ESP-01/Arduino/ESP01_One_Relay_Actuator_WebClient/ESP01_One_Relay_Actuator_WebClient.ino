/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#define GPIO0 0 //ESP01 GPIO0 PIN
#define GPIO1 1 //ESP01 TX PINLED_PIN_NO 
#define GPIO2 2 //ESP01 GPIO2 PIN
#define GPIO3 3 //ESP01 RX PIN

#define TX_PIN GPIO1
#define RX_PIN GPIO3

#define ESP01_2_LEDs_TYPE
// #define ESP01_1_LEDs_TYPE

#ifdef ESP01_2_LEDs_TYPE
#define LED_PIN_NO TX_PIN // ESP01 2 LED type은 LED가 GPIO 1(TX)과 연결 되어있다
#endif

#ifdef ESP01_1_LEDs_TYPE
#define LED_PIN_NO GPIO2 // ESP01 1 LED type은 LED가 GPIO 2과 연결 되어있다
#endif

#define LED_ON 0
#define LED_OFF 1

#define ESP01_DHT11_IO_PIN GPIO2 //GPIO 2  , LED 1개 짜리 ESP-01은 LED, DTH11 모두 GPIO 2번을 사용한다. 

#define ESP01_BUILTIN_LED_IO_PIN LED_PIN_NO  // in case of TX-LED , Pinnumber is 1 otherwise pinnumber is 2

#define RELAY_ON LOW
#define RELAY_OFF HIGH
#define RELAY_IO_PIN GPIO0

#define DEFAULT_PERIOD 10

//==================================================
//Features control
//==================================================
#define REPORT_VIA_LED
#undef  REPORT_VIA_SERIAL

#define MODULE_NAME "ESP-01-RELEAY"
#define SERVER_URL "http://192.168.0.40:9000"
#define APP_PATH "/homeAuto/hello/"

#define ERROR_SENT_OK 2 // wifi 연결이 안되는 경우 3번 주기적으로 점멸한다. 
#define ERROR_WIFI 3 // wifi 연결이 안되는 경우 3번 주기적으로 점멸한다.  
#define ERROR_CONNCET 4 // Server 찾기를 실패하면 LED를 4번 주기적으로 점멸한다. 
#define ERROR_HELLO 6 // Hello 에서 실패하면 5번씩 LED를 주기적으로 점멸한다.
#define ERROR_PARAM_PERIOD 7 // Hello 에서 실패하면 5번씩 LED를 주기적으로 점멸한다.
#define ERROR_PARAM_ACTION 8 // Hello 에서 실패하면 5번씩 LED를 주기적으로 점멸한다.

char bufferPayload[8916];
int request_period = DEFAULT_PERIOD ; // 이 주기로 Server에 물어본다. 
int current_relay_state=RELAY_OFF;
bool isHelloDone = false; // Hello 를 해서 Pass 했는가??

int blinkLED(int period);
ESP8266WiFiMulti WiFiMulti;
void buildPayload(char * );;
int showStateLED(int count);
int doReply(String * response);
int doActionParam(String * toDo);
int doPeriodChange(String * toDo);

int doRelayDrive(int state);


void setup() {
  
#ifdef REPORT_VIA_SERIAL
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  Serial.println();
#endif

  pinMode(RELAY_IO_PIN, OUTPUT);
  delay(100);
  digitalWrite(RELAY_IO_PIN, RELAY_OFF);
  
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

void loop() {

  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;
#ifdef REPORT_VIA_SERIAL
    Serial.print("[HTTP] begin...\n");
#endif
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
          doReply(&payload);


        }
      } else {
#ifdef REPORT_VIA_SERIAL
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
#endif

      }

      http.end();
      showStateLED(ERROR_SENT_OK);  
    } 
    else 
    {
#ifdef REPORT_VIA_SERIAL
      Serial.printf("[HTTP} Unable to connect\n");
#endif
#ifdef REPORT_VIA_LED
      showStateLED(ERROR_CONNCET);
#endif
    }
  }
  else
  {
#ifdef REPORT_VIA_SERIAL
    Serial.printf("Waiting.....Wifi Connection");
#endif
#ifdef REPORT_VIA_LED
    showStateLED(ERROR_WIFI);
#endif 
  }
  for(int i=0;i<request_period;i++)
  {
#ifdef REPORT_VIA_SERIAL
    Serial.printf("Waiting %d/%d sec before next request\n\n",i,request_period);
#endif  
    delay(1000);
  }

}

void buildPayload(char * buffer)
{
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(buffer,"%s%s%x%x%x%x%x%x/%s",
                SERVER_URL,APP_PATH,
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
                MODULE_NAME
                );
}
/*
hello Response ACK Device=<bcddc2e54f73>, Module=<ESP-01-RELEAY>, Action=<ON>, Period=<35>
*/
int doReply(String * response)
{
  if(response->indexOf("Action")>=0)
    doActionParam(response);
  if(response->indexOf("Period")>=0)
    doPeriodChange(response);
  return 0;
}
int doActionParam(String * toDo)
{
  if(toDo->indexOf("Action=<ON>")>0)
  {
#ifdef REPORT_VIA_SERIAL
    Serial.println("doActionParam: Action On");
#endif
    doRelayDrive(RELAY_ON);
  }
  else if(toDo->indexOf("Action=<OFF>")>0)
  {
#ifdef REPORT_VIA_SERIAL
    Serial.println("doActionParam: Action Off");
#endif    
    doRelayDrive(RELAY_OFF);
  }
  
  return 0; 
}
int doRelayDrive(int state)
{
  current_relay_state = state;
  digitalWrite(RELAY_IO_PIN, current_relay_state);
  return current_relay_state;
}
int doPeriodChange(String * toDo)
{

  int index_value_start=-1;
  int index_value_end=-1;
  char *pParamName="Period=<";
  int   paramLength = strlen(pParamName);
  index_value_start = toDo->indexOf(pParamName);
#ifdef REPORT_VIA_SERIAL

#endif
  Serial.printf("doPeriodChange:1. index_value_start %d [%c]\n",index_value_start,toDo->charAt(index_value_start));
  if(index_value_start < 0)
    return -1;
  index_value_start += paramLength;
#ifdef REPORT_VIA_SERIAL
  Serial.printf("doPeriodChange:1. index_value_start %d [%c]\n",index_value_start,toDo->charAt(index_value_start));
#endif
  index_value_end = toDo->indexOf(">",index_value_start)-1;
#ifdef REPORT_VIA_SERIAL
  Serial.printf("doPeriodChange:2. index_value_end %d\n",index_value_end);
  Serial.printf("doPeriodChange:1. index_value_end %d [%c]\n",index_value_end,toDo->charAt(index_value_end));
  Serial.printf("doPeriodChange: start = %d , end= %d\n",index_value_start,index_value_end);
#endif


  if(index_value_start < index_value_end)
  {
    
    String value = toDo->substring(index_value_start,index_value_end+1);
#ifdef REPORT_VIA_SERIAL
    Serial.printf("doPeriodChange, period = %s\n",value.c_str());
#endif
    value.trim();

    request_period = value.toInt();
    
#ifdef REPORT_VIA_SERIAL
    Serial.printf("doPeriodChange(%d)\n",request_period);
#endif
    
    if(request_period<5)
      request_period = DEFAULT_PERIOD;
  }
  else
  showStateLED(ERROR_PARAM_PERIOD);
  return 0; 
}

int showStateLED(int count)
{
    for(int i=0;i<count;i++)
    {
      blinkLED(100);
      delay(50);
    }
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
