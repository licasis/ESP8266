
#include <Adafruit_NeoPixel.h>

#define LED_PIN    0
#define LED_COUNT 4
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t color;
uint32_t colors[4]={0,0,0,0};
int g_index = 1;
void setup() {


  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  
  colors[0]= strip.Color(100,0, 0);
  colors[1]= strip.Color(0, 100, 0);
  colors[2]= strip.Color(0, 0, 100);
  colors[3]= strip.Color(200, 150, 30);
}

void loop() {

    strip.clear();         //   Set all pixels in RAM to 0 (off)

    strip.setPixelColor(0, colors[g_index%4]); 
    strip.setPixelColor(1, colors[(g_index+1)%4]); 
    strip.setPixelColor(2, colors[(g_index+2)%4]); 
    strip.setPixelColor(3, colors[(g_index+3)%4]); 
    strip.show(); 
    g_index++;
    delay(500);
}
