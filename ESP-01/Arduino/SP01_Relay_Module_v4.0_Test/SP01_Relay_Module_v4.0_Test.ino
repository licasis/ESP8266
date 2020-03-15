/*
  ESP8266 Relay Test PGM v 1.0
  Product infomation
    ESP-01S Relay Module
    Load: 10A/250VAC 10A/30VDC, the relay pulls 100,000 times
    Control pin: GPIO0 of ESP-01/01S (Low level active) 
    Product size: 37 * 25mm
*/

#define GPIO0 0 //ESP01 GPIO0 PIN
#define GPIO1 1 //ESP01 TX PIN
#define GPIO2 2 //ESP01 GPIO2 PIN
#define GPIO3 3 //ESP01 RX PIN

#define TX_PIN GPIO1
#define RX_PIN GPIO3

#define RELEAY_PIN GPIO0

// ESP01의 LED 갯수가 다르므로 , 경우에 따라 선택한다.
#define ESP01_2_LEDs_TYPE
// #define ESP01_1_LEDs_TYPE

#ifdef ESP01_2_LEDs_TYPE
#define LED_PIN GPIO1 // ESP01 2 LED type은 LED가 GPIO 1(TX PIN)과 연결 되어있다
#endif

#ifdef ESP01_1_LEDs_TYPE
#define LED_PIN GPIO2 // ESP01 1 LED type은 LED가 GPIO 2과 연결 되어있다
#endif

#define RELAY_ON LOW
#define RELAY_OFF HIGH

#define LED_ON LOW
#define LED_OFF HIGH

// function declare
void RelayCon(int state);

void setup() {
  // Init state.
  delay(2000);
  pinMode(LED_PIN, OUTPUT);  
  pinMode(RELEAY_PIN, OUTPUT);  
  digitalWrite(RELEAY_PIN, RELAY_OFF);
  digitalWrite(LED_PIN, LED_OFF);
}

// the loop function runs over and over again forever
void loop() {

  RelayCon(RELAY_ON);
  delay(2000);     
  RelayCon(RELAY_OFF);
  delay(2000);  
}
void RelayCon(int state)
{
  digitalWrite(LED_PIN, (state==0)?LED_OFF:LED_ON);
  digitalWrite(RELEAY_PIN, (state==0)?RELAY_OFF:RELAY_ON);
}
