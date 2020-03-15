/*
  ESP8266 Blink by Simon Peter
  Blink the blue LED on the ESP-01 module
  This example code is in the public domain

  The blue LED on the ESP-01 module is connected to GPIO1
  (which is also the TXD pin; so we cannot use Serial.print() at the same time)

  Note that this sketch uses LED_BUILTIN to find the pin with the internal LED
*/
#define GPIO0 0 //ESP01 GPIO0 PIN
#define GPIO1 1 //ESP01 TX PIN
#define GPIO2 2 //ESP01 GPIO2 PIN
#define GPIO3 3 //ESP01 RX PIN

#define TX_PIN GPIO1
#define RX_PIN GPIO3

// ESP01의 LED 갯수가 다르므로 , 경우에 따라 선택한다.
#define ESP01_2_LEDs_TYPE
// #define ESP01_1_LEDs_TYPE

#ifdef ESP01_2_LEDs_TYPE
#define LED_PIN_NO GPIO1 // ESP01 2 LED type은 LED가 GPIO 1과 연결 되어있다
#endif

#ifdef ESP01_1_LEDs_TYPE
#define LED_PIN_NO GPIO2 // ESP01 1 LED type은 LED가 GPIO 2과 연결 되어있다
#endif


void setup() {
  pinMode(LED_PIN_NO, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_PIN_NO, LOW);   // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is active low on the ESP-01)
  delay(1000);                      // Wait for a second
  digitalWrite(LED_PIN_NO, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(2000);                      // Wait for two seconds (to demonstrate the active low LED)
}
