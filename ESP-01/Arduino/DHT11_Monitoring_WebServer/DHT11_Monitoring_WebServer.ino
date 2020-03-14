#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "DHTesp.h"

#ifndef STASSID
#define STASSID "WirelessWorld_11n"
#define STAPSK  "kimhaksoo"
#endif

#define MAX_RESPONSE_LENGTH 4096


enum renderType{ERROR,HTML,XML,JSON};
const char* ssid = STASSID;
const char* password = STAPSK;
char responseBuffer[MAX_RESPONSE_LENGTH];
renderType g_responseType=HTML;

ESP8266WebServer server(80);
#define DTH11_ESP01_IO_PIN 2
DHTesp dht;


int buildResponse(renderType,float,float);
int responseTemp(renderType);
//const int led = 13; // Do not USE LED

void handleRoot() {
  //digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  //digitalWrite(led, 0);
}

void handleNotFound() {
  //digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  //digitalWrite(led, 0);
}

void setup(void) {
  //pinMode(led, OUTPUT);
  //digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  dht.setup(DTH11_ESP01_IO_PIN, DHTesp::DHT11);
  Serial.println("\n\n\n\n");
  Serial.print("Wifi Connecting .");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.on("/Temp/xml", []() {
    responseTemp(XML);
  });

  server.on("/Temp/html", []() {
    responseTemp(HTML);
  });  
  server.on("/gif", []() {
    static const uint8_t gif[] PROGMEM = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x10, 0x00, 0x10, 0x00, 0x80, 0x01,
      0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x10, 0x00, 0x00, 0x02, 0x19, 0x8c, 0x8f, 0xa9, 0xcb, 0x9d,
      0x00, 0x5f, 0x74, 0xb4, 0x56, 0xb0, 0xb0, 0xd2, 0xf2, 0x35, 0x1e, 0x4c,
      0x0c, 0x24, 0x5a, 0xe6, 0x89, 0xa6, 0x4d, 0x01, 0x00, 0x3b
    };
    char gif_colored[sizeof(gif)];
    memcpy_P(gif_colored, gif, sizeof(gif));
    // Set the background to a random set of colors
    gif_colored[16] = millis() % 256;
    gif_colored[17] = millis() % 256;
    gif_colored[18] = millis() % 256;
    server.send(200, "image/gif", gif_colored, sizeof(gif_colored));
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}
int responseTemp(renderType type)
{
  
    int count = 5;
    float humidity;
    float temperature;
    char *pStatus=NULL;

    do
    {
    delay(dht.getMinimumSamplingPeriod());
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();
    pStatus = ( char *)dht.getStatusString();
    if(pStatus!=NULL)
    {
      if(pStatus[0]=='O' && pStatus[1]=='K')// Data is taken correctly
      { 
        g_responseType = type;
        if(buildResponse(temperature,humidity))
          break;
      }
    }
    count--;
    }while(count!=0);
    if(count==0)
    {
      g_responseType = ERROR;
      buildResponse(-1.0,-1.0);
    }
    Serial.println("response");
    
    if(g_responseType == XML)
      server.send(200, "text/xml", responseBuffer);
    else if(g_responseType == HTML || g_responseType==ERROR)
      server.send(200, "text/html", responseBuffer);

}
int buildResponse(float temp,float  humidity)
{
  if(g_responseType == HTML)
  {
    sprintf(responseBuffer,
      "<html>\n" \
          "\t<head>\n"\
          "\t\t<meta name=\"MCU\" content=\"ESP8266\">\n"\
          "\t\t<meta name=\"BOARD\" content=\"ESP-01\">\n"\
          "\t\t<meta name=\"DEVICE\" content=\"DHT11v1.0\">\n"\
          "\t\t<meta http-equiv=\"refresh\" content=\"5\">\n"
          "\t</head>\n"\
          "\t<body>\n"\
              "\t\t<p>Temperature=%5.2f</p>\n"\
              "\t\t<p>Humidity=%5.2f</p>\n"\ 
          "\t</body>\n"\
      "</html>"\
      ,temp,humidity);
  }
  else if(g_responseType == XML)
  {
    sprintf(responseBuffer,
      "<ROOT>\n" \
          "\t<meta name=\"MCU\" content=\"ESP8266\" />\n"\
          "\t<meta name=\"BOARD\"  comment=\"Build-In LED (TX pin)\" content=\"ESP-01\" />\n"\
          "\t<meta name=\"MODULE\" comment=\"Temperature and Humidity sensor\" content=\"DHT11 v1.0\" />\n"\
          "\t<meta name=\"refresh\" comment=\"Refrash data every n secconds\" content=\"5\" />\n"\
          "\t<MESUREMENT>\n"\
              "\t\t<Item type=\"Temperature\" value=\"%5.2f\" />\n"\
              "\t\t<Item type=\"Humidity\" value=\"%5.2f\" />\n"\
          "\t</MESUREMENT>\n"\
      "</ROOT>"\
      ,temp,humidity);
  }
  else if(g_responseType == ERROR)
  {
    sprintf(responseBuffer,
      "<html>\n" \
          "\t<head>\n"\
          "\t\t<meta name=\"MCU\" content=\"ESP8266\">\n"\
          "\t\t<meta name=\"BOARD\" content=\"ESP-01\">\n"\
          "\t\t<meta name=\"DEVICE\" content=\"DHT11v1.0\">\n"\
          "\t\t<meta http-equiv=\"refresh\" content=\"5\">\n"
          "\t</head>\n"\
          "\t<body>\n"\
              "\t\t<p>Temperature=%5.2f</p>\n"\
              "\t\t<p>Humidity=%5.2f</p>\n"\ 
          "\t</body>\n"\
      "</html>"\
      ,temp,humidity);
  }
  Serial.println(responseBuffer);
  return 1;
}





void loop(void) {
  server.handleClient();
  MDNS.update();
}
