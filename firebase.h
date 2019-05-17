#ifndef FIREBASE_IO_H
#define FIREBASE_IO_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>


//------------------------------------------------------------------------------
class FirebaseIO
{
     friend FirebaseIO& Firebase();
     private:
          enum
          {
               STREAM_EVENT_SIZE = 32,
               STREAM_DATA_SIZE  = 300
          };
          enum
          {
               PATCH_PATH_SIZE = 32,
               PATCH_DATA_SIZE = 300
          };
          static const char *HOST;
          static const char *SECRETS;
          static const int   PORT;

          WiFiClientSecure m_patchClient;
          xQueueHandle     m_patchQueue;

          WiFiClientSecure m_streamClient;
          String           m_streamPath;
          char             m_streamBuffer[STREAM_EVENT_SIZE+STREAM_DATA_SIZE];
          xQueueHandle     m_streamQueue;

          FirebaseIO(){}
          void checkServerResponse();
          void processStreaming();
          static void streamTask(void *param);

          void processPatch();
          static void patchTask(void *param);

     public:
          bool patch(String path, JsonVariant value);
          void begin(String path);
          bool getStreamResponse(String& event, String& data);
};

FirebaseIO& Firebase();

#endif

