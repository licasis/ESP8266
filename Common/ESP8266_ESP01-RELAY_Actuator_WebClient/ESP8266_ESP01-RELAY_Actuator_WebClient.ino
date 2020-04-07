
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#define USE_ESP_01
#undef  USE_NODE_MCU

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


//////////////////////////
//define 
#define PROTOCOL "http://"
#define SERVER "192.168.0.40"
#define PORT 9000
#define APP_PATH "homeAuto/request"


#define SERIAL_BAUD_RATE 115200
#define MODULE_COMPONENT_COUNT 1
#define MODULE_NAME "ESP01-RELAY"

#define GPIO_PIN_COMPONENT0 GPIO0
#define RELAY_OFF 1
#define RELAY_ON 0
#define RELAY_INIT RELAY_OFF



//////////////////////////
//structure & enums
typedef enum 
{
  IDLE,
  KEEP,
  MEASURE,
  OFF,
  ON,
  STOP,
  START,
  FORWARD,
  BACKWARD,
  ALARM_ON,
  ALARM_OFF,
  COLOR,
  END_OF_ENUM
}Order;


typedef struct _tComponent
{
  int number;
  Order order;
  String value;
}Component;



//////////////////////////
//Declare variables
byte MACaddress[6];
int isMACTaken=0; 
int isFirstRequest=1;
int repeat_period = 10000; // 60 sec
char serialBuff[256];
char requestURL[1024]; 

char * strOrder[]{"IDLE","KEEP","MEASURE","OFF","ON","STOP","START","FORWARD","BACKWARD","ALARM_ON","ALARM_OFF","COLOR"};

Component components[10];
int nComponent=0;

int mapGPIO[MODULE_COMPONENT_COUNT]={GPIO_PIN_COMPONENT0};
ESP8266WiFiMulti WiFiMulti;

//////////////////////////
//Declare functions
int  blinkLED(int period);
int handleResponse(String *response);
int fillComponentInfo(String *raw,Component * pComponent);
int doHandleComponent(Component * pComponent,int nCount);


void setup() {


 
#ifdef REPORT_VIA_SERIA
  Serial.begin(SERIAL_BAUD_RATE);
  //Serial. setTimeout(300);
  Serial.println();
#else
  #ifdef SERIAL_BAUD_RATE 
    Serial.begin(SERIAL_BAUD_RATE);
    //Serial. setTimeout(300);
    delay(1000); // UART에 연결된 디바이스의 초기화시간동안 대기
    Serial.readBytes(serialBuff,512); // 이것은 timeout동안 input buffer에서 뭐든지 읽어온다.(Timeout 1초)
                            // UART에 디바이스가 초기화 되면서 보낸 노이즈성 UART 신호를 제거하는 효과.
  #endif
#endif

  for(int i=0;i<MODULE_COMPONENT_COUNT;i++)
  {
    pinMode(mapGPIO[i], OUTPUT); 
    digitalWrite(mapGPIO[i], RELAY_INIT);
  }

#ifdef REPORT_VIA_LED
  pinMode(LED_PIN_NO, OUTPUT);
  for(int i=0;i<2;i++)
  {
    blinkLED(2000);
    delay(500);
  }
#endif
  Serial.println("\n\n\n");
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

  if ((WiFiMulti.run() != WL_CONNECTED))
  {

#ifdef REPORT_VIA_SERIAL
    Serial.println("[SETUP] Waiting.....Wifi Connection");
#endif
#ifdef REPORT_VIA_LED
    for(int i=0;i<5;i++)
    {
      blinkLED(100);
      delay(50);
    }
#endif 

     delay(1000);

     return;
  }

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Now Using wifi is available from here.
//
////////////////////////////////////////////////////////////////////////////////////////////////////


// take MAC if it is not taken yet

  if(isMACTaken==0)
  {
    WiFi.macAddress(MACaddress);
    isMACTaken = 1;
  }
  
// send request URL
// build URL 
// ex) "http://192.168.0.40:9000/homeAuto/request/WS2812/"
  sprintf(requestURL,"%s%s:%d/%s/%02x%02x%02x%02x%02x%02x/%s/%s/",
                PROTOCOL,
                SERVER,
                PORT,
                APP_PATH,
                MACaddress[0],MACaddress[1],MACaddress[2],MACaddress[3],MACaddress[4],MACaddress[5],MODULE_NAME,
                (isFirstRequest==1)?"Init":"Run" // 최초 request일때는 Init을 보내서 초기상태받아야하고, , 그 이후 request에는 Run을 보내서 동작중임을 알린다.  
        );
                
    WiFiClient client;

    HTTPClient http;
#ifdef REPORT_VIA_SERIAL
    Serial.print("[ HTTP] begin...\n");
#endif


    if (http.begin(client, requestURL)) 
    {  // HTTP
#ifdef REPORT_VIA_SERIAL
      Serial.printf("[ HTTP] GET... request %s \n",requestURL);
#endif


      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) 
      {
        // HTTP header has been send and Server response header has been handled
#ifdef REPORT_VIA_SERIAL
        Serial.printf("[ HTTP] GET... code: %d \n", httpCode);
#endif
        
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
        {
          String response = http.getString();
          Serial.printf("[ HTTP] GET... response: %s \n", response.c_str());
          if(handleResponse(&response)==0)
          {
#ifdef REPORT_VIA_SERIAL
            Serial.println("[ HTTP] Handling response OK");
#endif
            isFirstRequest = 0;
          }
        }
      } 
      else {
#ifdef REPORT_VIA_SERIAL
        Serial.printf("[ HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
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
#ifdef REPORT_VIA_SERIAL
      Serial.printf("[ HTTP] Wating %d sec\n", repeat_period/1000);
#endif
    delay(repeat_period);
}

//
//Component components[10];
//int nComponent=0;
/*
typedef struct _tComponent
{
  int number;
  Order order;
  String value;
}Component;

 */
int handleResponse(String *response)
{
  //component#0:$ORDER->$VALUE;component#1:$ORDER->$VALUE;
  nComponent = 0;
  int start_index = 0;
  int find_index = 0;
  String temp;
  while(1)
  {
    find_index = response->indexOf(';',start_index);
    if(find_index<0)
      break;
    temp = response->substring(start_index,find_index+1);
    if(fillComponentInfo(&temp,&components[nComponent])!=0)
      return -1;
    start_index = find_index +1;
    nComponent++;
  }
  doHandleComponent(components,nComponent);
  return 0; // means OK.
}

int doHandleComponent(Component * pComponent,int nCount)
{
  int max_led_count = (MODULE_COMPONENT_COUNT<nCount)?MODULE_COMPONENT_COUNT:nCount;
  for(int i=0;i<max_led_count;i++)
  {
    if((pComponent+i)->order == ON)
    {
      digitalWrite(mapGPIO[i], RELAY_ON);
    }
    else if ((pComponent+i)->order == OFF)
    {
      digitalWrite(mapGPIO[i], RELAY_OFF);
    }
  }
  return 0;
}

int fillComponentInfo(String *raw,Component * pComponent)
{
  //component#1:ORDER->VALUE;
  int start_index=0;
  int end_index=0;
  int order=-1;
  String temp;

#ifdef REPORT_VIA_SERIAL
      Serial.printf("[HANDLE] raw text >>%s<< \n", raw->c_str());
#endif

  // get component number
  start_index = raw->indexOf('#');
  end_index = raw->indexOf(":");

  if(start_index >= end_index)
    return -1;
  temp = raw->substring(start_index+1,end_index);
#ifdef REPORT_VIA_SERIAL
      Serial.printf("[HANDLE] Number text >>%s<<\n", temp.c_str());
#endif
  pComponent->number = temp.toInt() ;

  // get component number
  pComponent->order = END_OF_ENUM;
  start_index = end_index+1;
  end_index = raw->indexOf("->");
  
  if(start_index >= end_index)
    return -1;
    
  temp = raw->substring(start_index,end_index); 
  #ifdef REPORT_VIA_SERIAL
      Serial.printf("[HANDLE] Order text >>%s<< \n", temp.c_str());
  #endif
  for(int i=0;i<END_OF_ENUM;i++)
  {

    #ifdef REPORT_VIA_SERIAL
      //Serial.printf(">>%s<<:>>%s<< \n", temp.c_str(),strOrder[i]);
    #endif
  
    if(temp.equalsIgnoreCase(String(strOrder[i]))==true)
    {
#ifdef REPORT_VIA_SERIAL
      Serial.printf("MATCH!!! >>%s<<:>>%s<<@%d \n", temp.c_str(),strOrder[i],i);
#endif
      pComponent->order=(Order)i;
      break;
    }
  }
  if(pComponent->order == END_OF_ENUM) //만약 Order에 일치하는것을 못찾으면 error
    return -1;


  // get value
  start_index = end_index+2; //"->" 이므로 2byte 앞으로 간다
  end_index = raw->indexOf(";");
  if(start_index >= end_index)
      return -1;
  pComponent->value = raw->substring(start_index,end_index);
#ifdef REPORT_VIA_SERIAL
      Serial.printf("[HANDLE] Value text >>%s<< \n", pComponent->value.c_str());
#endif
#ifdef REPORT_VIA_SERIAL
      Serial.printf("[HANDLE] Component Number :%d\n", pComponent->number);
      Serial.printf("[HANDLE] Component Order  :%d\n", pComponent->order);
      Serial.printf("[HANDLE] Component Value  :%s\n", pComponent->value.c_str());
#endif
  
  return 0;
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
