#include <WiFi.h>
#include <ArduinoJson.h>
#include "firebase.h"

const char* SSID     = "Your WiFi SSID";
const char* PASSWORD = "Your WiFi PASS";

//------------------------------------------------------------------------------
void setup()
{
     Serial.begin(115200);
     WiFi.begin(SSID, PASSWORD);

     Serial.print("connecting");
     while( WiFi.status() != WL_CONNECTED )
     {
          Serial.print(".");
          delay(500);
     }
     Serial.println();
     Serial.print("connected: ");
     Serial.println(WiFi.localIP());

     delay(2000);

     Firebase().begin("command");
}

//------------------------------------------------------------------------------
void loop()
{
     String e, d;
     if( Firebase().getStreamResponse(e, d) )
     {
          Serial.print("event : ");
          Serial.println(e);
          Serial.print("data  : ");
          Serial.println(d);
     }

     static uint32_t t = 0;

     if( millis() > t )
     {
          StaticJsonBuffer<256> jsonBuffer;
          JsonObject& root = jsonBuffer.createObject();     
          root["isConnected"] = t;
          Firebase().patch("debug", JsonVariant(root));
          t = millis() + 1000;
     }     

     // put your main code here, to run repeatedly:
}
