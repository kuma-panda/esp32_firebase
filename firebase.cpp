#include "firebase.h"

//------------------------------------------------------------------------------
FirebaseIO& Firebase()
{
     static FirebaseIO _firebase;
     return _firebase;
}

//------------------------------------------------------------------------------
const char *FirebaseIO::HOST    = "your firebase project URL";
const int   FirebaseIO::PORT    = 443;
const char *FirebaseIO::SECRETS = "your firebase project SECRETS";

//------------------------------------------------------------------------------
void FirebaseIO::begin(String path)
{
     m_streamPath = path;
     m_streamQueue = xQueueCreate(5, STREAM_EVENT_SIZE+STREAM_DATA_SIZE);
     xTaskCreatePinnedToCore(streamTask, "streamTask", 8192, this, 3, NULL, 1);

     m_patchQueue = xQueueCreate(5, PATCH_PATH_SIZE+PATCH_DATA_SIZE);
     xTaskCreatePinnedToCore(patchTask, "patchTask", 8192, this, 2, NULL, 1);
}

//------------------------------------------------------------------------------
void FirebaseIO::streamTask(void *param)
{
     FirebaseIO *self = (FirebaseIO *)param;
     while( true )
     {
          self->processStreaming();
          vTaskDelay(1);          
     }
}

//------------------------------------------------------------------------------
void FirebaseIO::processStreaming()
{
     if( !m_streamClient.connected() )
     {
          if( !m_streamClient.connect(HOST, PORT) )
          {
               vTaskDelay(500);
               return;
          }

          String request;
          request = "GET /";
          request += m_streamPath + ".json?auth=";
          request += String(SECRETS) + " HTTP/1.1\r\n";

          String header;
          header = "Host: ";
          header += String(HOST) + "\r\n";
          header += "Accept: text/event-stream\r\n";
          header += "Connection: close\r\n";
          header += "\r\n"; //空行

          // Serial.println( "Send Server-Sent Events GET request." );
          m_streamClient.print(request);
          m_streamClient.print(header);

          // Serial.print(request);
          // Serial.print(header);
     }

     if( m_streamClient.available() )
     {
          String line = m_streamClient.readStringUntil(0x0A);
          if( line.startsWith("event:") )
          {
               String s = line.substring(7, line.length());
               s.trim();
               strcpy(m_streamBuffer, s.c_str());
		}
          else if( line.startsWith("data:") )
          {
               String s = line.substring(6, line.length());
               s.trim();
               strcpy(m_streamBuffer+STREAM_EVENT_SIZE, s.c_str());
          } 
          else if( line.length() == 0 )
          {
               xQueueSend(m_streamQueue, m_streamBuffer, portMAX_DELAY);
          }
     }     
}

//------------------------------------------------------------------------------
bool FirebaseIO::getStreamResponse(String& event, String& data)
{
     if( uxQueueMessagesWaiting(m_streamQueue) )
     {
          char buffer[STREAM_EVENT_SIZE+STREAM_DATA_SIZE];
          xQueueReceive(m_streamQueue, buffer, portMAX_DELAY);
          event = String(buffer);
          data  = String(buffer+STREAM_EVENT_SIZE);
          return true;         
     }
     return false;
}

//------------------------------------------------------------------------------
bool FirebaseIO::patch(String path, JsonVariant value)
{
     if( !uxQueueSpacesAvailable(m_patchQueue) )
     {
          return false;
     }
     char buffer[PATCH_PATH_SIZE+PATCH_DATA_SIZE];
     strcpy(buffer, path.c_str());
     JsonObject& data = value;
     data.printTo(buffer+PATCH_PATH_SIZE, PATCH_DATA_SIZE);
     xQueueSend(m_patchQueue, buffer, portMAX_DELAY);
     return true;
}

//------------------------------------------------------------------------------
void FirebaseIO::patchTask(void *param)
{
     FirebaseIO *self = (FirebaseIO *)param;
     while( true )
     {
          self->processPatch();
          vTaskDelay(1);
     }
}

//------------------------------------------------------------------------------
void FirebaseIO::processPatch()
{
     if( !uxQueueMessagesWaiting(m_patchQueue) )
     {
          return;
     }

     if( !m_patchClient.connect(HOST, PORT) )
     {
          Serial.print("Unable to connect to ");
          Serial.println(HOST);
          return;
     }

     char buffer[PATCH_PATH_SIZE+PATCH_DATA_SIZE];
     xQueueReceive(m_patchQueue, buffer, portMAX_DELAY);
     String path = String(buffer);
     String body = String(buffer+PATCH_PATH_SIZE);
     
     String request;
     request = "PATCH /";
     request += path;
     request += ".json?auth=";
     request += String(SECRETS);
     request += " HTTP/1.1\r\n";

     String head;
     head = "Host: ";
     head += String(HOST) + "\r\n";
     head += "Connection: keep-alive\r\n";
     head += "Content-Length: ";
     head += String(body.length()) + "\r\n";
     head += "\r\n";

     m_patchClient.print(request);
     m_patchClient.print(head);
     m_patchClient.print(body);

     // Serial.println( "####### Send HTTP Patch Request #######" );
     // Serial.print(request);
     // Serial.print(head);
     // Serial.println(body);

     checkServerResponse();

     delay(2);
     m_patchClient.stop(); //これ絶対必要
     delay(2);
}

//------------------------------------------------------------------------------
void FirebaseIO::checkServerResponse()
{
     // Firebaseサーバーからのレスポンスを全て受信し切ることが重要
     // Serial.println("####### Firebase server HTTP Response #######");
     while( m_patchClient.connected() )
     {
          String response = m_patchClient.readStringUntil('\n');
          // Serial.println(response);
          if( response == "\r" )
          {
               //空行検知
               // Serial.println("------------- Response Headers Received");
               break;
          }
          vTaskDelay(1);
     }

     //レスポンスヘッダが返ってきた後、body部分が返って来る
     while( m_patchClient.available() )
     {
          char c = m_patchClient.read();
          // Serial.print( c );
          vTaskDelay(1);
     }
     // Serial.println("\r\n----------- Body Received");
}


