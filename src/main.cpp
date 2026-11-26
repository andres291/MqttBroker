#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <sMQTTBroker.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include "config.h"
#include "WiFiRecoveryManager.h"

static_assert(CONFIG_LWIP_MAX_SOCKETS >= 40, "Rebuild ESP-IDF: at least 40 sockets required");
static_assert(CONFIG_LWIP_MAX_ACTIVE_TCP >= 40, "Rebuild ESP-IDF: at least 40 TCP connections required");

WiFiRecoveryManager wifiMgr;
WebServer server(80);
bool servicesStarted = false;
bool updating = false;
bool firstStartAttempt = true;
unsigned long lastStartAttempt = 0;
uint32_t authenticatedConnections = 0;
uint32_t authenticationFailures = 0;

class MyBroker : public sMQTTBroker {
public:
    bool onEvent(sMQTTEvent *event) override {
        if (event->Type() == NewClient_sMQTTEventType) {
            auto *e = static_cast<sMQTTNewClientEvent *>(event);
            if (e->Login() != Config::MQTT_USER || e->Password() != Config::MQTT_PASSWORD) {
                ++authenticationFailures;
                Serial.println("MQTT: login rejected");
                return false;
            }
            ++authenticatedConnections;
            Serial.println("MQTT: client authenticated");
        }
        return true;
    }
};
MyBroker broker;

String statusJson() {
    String json;
    json.reserve(400);
    json = "{\"firmware\":\"MqttBroker2-1.0.1\",\"uptime_s\":";
    json += String(static_cast<unsigned long long>(esp_timer_get_time() / 1000000));
    json += ",\"clients\":" + String(broker.getConnectedClientCount());
    json += ",\"tcp_sessions\":" + String(broker.getClientCount());
    json += ",\"max_clients\":" + String(Config::MAX_CLIENTS);
    json += ",\"socket_limit\":" + String(CONFIG_LWIP_MAX_SOCKETS);
    json += ",\"free_heap\":" + String(ESP.getFreeHeap());
    json += ",\"min_free_heap\":" + String(ESP.getMinFreeHeap());
    json += ",\"largest_free_block\":" + String(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    json += ",\"wifi_rssi\":" + String(WiFi.RSSI());
    json += ",\"connections_total\":" + String(authenticatedConnections);
    json += ",\"auth_failures\":" + String(authenticationFailures);
    json += ",\"capacity_rejections\":" + String(broker.getRejectedClientCount());
    json += "}";
    return json;
}

void setup() {
    Serial.begin(115200);
    Serial.printf("\nMqttBroker2: sockets=%d, active TCP=%d, MQTT limit=%u\n",
                  CONFIG_LWIP_MAX_SOCKETS, CONFIG_LWIP_MAX_ACTIVE_TCP, Config::MAX_CLIENTS);
    broker.setMaxClients(Config::MAX_CLIENTS);
    wifiMgr.begin(Config::AP_NAME, Config::HOSTNAME, Config::AP_PASSWORD);

    server.on("/", HTTP_GET, [] {
        server.send(200, "text/html; charset=utf-8",
            "<!doctype html><html lang='de'><meta charset='utf-8'><title>MqttBroker2</title>"
            "<h1>MqttBroker2</h1><p><a href='/status'>Status</a> | "
            "<a href='/update'>Firmware aktualisieren</a></p>"
            "<p>MQTT: Port 1883, bis zu 30 Clients. Status auch unter mqttbroker2/status.</p></html>");
    });
    server.on("/status", HTTP_GET, [] { server.send(200, "application/json", statusJson()); });
    ElegantOTA.begin(&server, Config::UPDATE_USER, Config::UPDATE_PASSWORD);
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.onStart([] { updating = true; Serial.println("OTA: update started"); });
    ElegantOTA.onProgress([](size_t, size_t) {
        // WebServer handles uploads synchronously; continue serving MQTT chunks.
        if (servicesStarted && wifiMgr.isConnected()) broker.update();
        delay(1);
    });
    ElegantOTA.onEnd([](bool success) {
        Serial.println(success ? "OTA: successful, restarting" : "OTA: failed");
        if (!success) updating = false;
    });
}

void loop() {
    if (!wifiMgr.isConnected() && servicesStarted) {
        server.stop();
        broker.shutdown();
        servicesStarted = false;
        Serial.println("WiFi lost: MQTT connections released");
    }
    wifiMgr.loop();
    if (wifiMgr.isConnected() && !servicesStarted &&
        (firstStartAttempt || millis() - lastStartAttempt >= 5000)) {
        firstStartAttempt = false;
        lastStartAttempt = millis();
        if (broker.init(Config::MQTT_PORT)) {
            server.begin();
            servicesStarted = true;
            Serial.print("MQTT ready. Web/update: http://");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("MQTT listener failed; retrying in 5 seconds");
        }
    }
    if (servicesStarted) {
        broker.update();
        server.handleClient();
        ElegantOTA.loop();
        static unsigned long lastStatus = 0;
        if (!updating && millis() - lastStatus >= Config::DIAGNOSTIC_INTERVAL_MS) {
            lastStatus = millis();
            String json = statusJson();
            Serial.println(json);
            broker.publish(Config::DIAGNOSTIC_TOPIC, json.c_str(), 0, true);
        }
    }
    delay(1);
}
