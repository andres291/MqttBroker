#pragma once
#include <cstdint>
#include "credentials.h"

namespace Config {
// Home LAN: fixed broker endpoint, retained across WiFi reconnects.
constexpr uint8_t STATIC_IP[4] = {192, 168, 178, 120};
constexpr uint8_t GATEWAY[4] = {192, 168, 178, 1};
constexpr uint8_t SUBNET[4] = {255, 255, 255, 0};
constexpr uint8_t DNS[4] = {192, 168, 178, 1};
constexpr char HOSTNAME[] = "mqttbroker";
constexpr char AP_NAME[] = "MqttBroker Setup";
constexpr char AP_PASSWORD[] = "mqttbroker";
constexpr uint16_t MQTT_PORT = 1883;
constexpr uint8_t MAX_CLIENTS = 30;
constexpr unsigned long DIAGNOSTIC_INTERVAL_MS = 60000;
constexpr char DIAGNOSTIC_TOPIC[] = "mqttbroker/status";
}
