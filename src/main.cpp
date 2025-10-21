#include<sMQTTBroker.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ElegantOTA.h>
#include "WiFiRecoveryManager.h"

WiFiRecoveryManager wifiMgr;
WebServer server(80);


const char* MQTT_CLIENT_USER = "user"; // username for mqtt clients. Set your own value here.
const char* MQTT_CLIENT_PASSWORD = "password"; // password for mqtt clients. Set your own value here.

class MyBroker:public sMQTTBroker
{
public:
    bool onEvent(sMQTTEvent *event) override
    {
        switch(event->Type())
        {
        case NewClient_sMQTTEventType:
            {
                sMQTTNewClientEvent *e=(sMQTTNewClientEvent*)event;
		            // Check username and password used for new connection
                if ((e->Login() != MQTT_CLIENT_USER) || (e->Password() != MQTT_CLIENT_PASSWORD)) {
                  Serial.println("Invalid username or password");  
                  return false;
                  }
            };
            break;
        case LostConnect_sMQTTEventType:
            WiFi.reconnect();
            break;
        case UnSubscribe_sMQTTEventType:
        case Subscribe_sMQTTEventType:
            {
                sMQTTSubUnSubClientEvent *e=(sMQTTSubUnSubClientEvent*)event;
            }
            break;
        }
        return true;
    }
};

MyBroker broker;
unsigned long Time;
unsigned long freeRam;

void setup()
{
    Serial.begin(9600);
    Serial.println("Setup started");
  
    wifiMgr.begin("Mqtt Broker Setup", "mqttbroker");
    if (wifiMgr.isConnected()) {
      Serial.println("Verbunden mit WiFi");
      Serial.print("IP-Adresse: ");
      Serial.println(WiFi.localIP());
    }
    
    // Webserver starten
    server.begin();
    Serial.println("Webserver gestartet");
 
    ElegantOTA.begin(&server);  // OTA aktivieren
    ElegantOTA.setAutoReboot(true);

    const unsigned short mqttPort=1883;
    broker.init(mqttPort);
    // all done
    // your magic code
    Time=millis();
    freeRam=ESP.getFreeHeap();
};
void loop()
{
    broker.update();
    // your magic code
    if(millis()-Time>1000)
    {
      Time=millis();
      if(ESP.getFreeHeap()!=freeRam)
      {
        freeRam=ESP.getFreeHeap();
      Serial.print("RAM:");
      Serial.println(freeRam);
      }
    }

    server.handleClient();
    wifiMgr.loop();
    ElegantOTA.loop();
};
