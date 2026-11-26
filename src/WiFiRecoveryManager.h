#pragma once
#include <WiFiManager.h>

class WiFiRecoveryManager {
public:
  void begin(const char* apName = "ESP-Setup", const char* hostname = "esp32", const char* apPassword = nullptr);
  void loop();
  bool isConnected();
  bool isInAPMode();

private:
  WiFiManager wm;
  bool apMode = false;
  bool wasConnected = false;
  unsigned long lastAttempt = 0;
  unsigned long disconnectedSince = 0;
  const unsigned long reconnectInterval = 30000;
  const unsigned long apTriggerDelay = 60000;
  const char* _apPassword = nullptr;
  const char* _apName = "ESP-Setup";
  const char* _hostname = "esp32";
  void reconnect();
  bool applyStaticIP();
};
