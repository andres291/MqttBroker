#include "WiFiRecoveryManager.h"
#include <WiFi.h>
#include "config.h"

void WiFiRecoveryManager::begin(const char* apName, const char* hostname, const char* apPassword) {
  _apName = apName;
  _hostname = hostname;
  _apPassword = apPassword;
  WiFi.mode(WIFI_STA);
  // WiFiManager must use the same address when connecting from its portal.
  wm.setSTAStaticIPConfig(IPAddress(Config::STATIC_IP), IPAddress(Config::GATEWAY),
                          IPAddress(Config::SUBNET), IPAddress(Config::DNS));
  applyStaticIP();
  WiFi.setAutoReconnect(true);
  wm.setHostname(_hostname);
  wm.setConnectTimeout(20);
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(0);
  wm.autoConnect(_apName, apPassword);

  apMode = wm.getConfigPortalActive();
  wasConnected = isConnected();
  disconnectedSince = lastAttempt = millis();
  if (!wasConnected) {
    reconnect();
  }
}

bool WiFiRecoveryManager::applyStaticIP() {
  const bool success = WiFi.config(IPAddress(Config::STATIC_IP), IPAddress(Config::GATEWAY),
                                   IPAddress(Config::SUBNET), IPAddress(Config::DNS));
  if (!success) Serial.println("WiFi: static IP configuration failed");
  return success;
}

void WiFiRecoveryManager::reconnect() {
  lastAttempt = millis();
  if (!wm.getWiFiIsSaved()) {
    return;
  }

  // Enabling STA preserves the running setup AP (AP + STA mode).
  WiFi.enableSTA(true);
  WiFi.setHostname(_hostname);
  WiFi.disconnect(false, false);
  // Reapply before every explicit reconnect; do not fall back to DHCP.
  if (!applyStaticIP()) return;
  WiFi.begin();
  Serial.println("WiFi: retrying saved network");
}

void WiFiRecoveryManager::loop() {
  wm.process();
  apMode = wm.getConfigPortalActive();

  if (isConnected()) {
    if (apMode) {
      wm.stopConfigPortal();
      apMode = wm.getConfigPortalActive();
      Serial.println("WiFi: connected, setup AP closed");
    }
    wasConnected = true;
    return;
  }

  const unsigned long now = millis();
  if (wasConnected) {
    wasConnected = false;
    disconnectedSince = now;
    reconnect();
    return;
  }

  if (!apMode && now - disconnectedSince >= apTriggerDelay) {
    wm.startConfigPortal(_apName, _apPassword);
    apMode = wm.getConfigPortalActive();
    reconnect();
  } else if (now - lastAttempt >= reconnectInterval) {
    reconnect();
  }
}

bool WiFiRecoveryManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool WiFiRecoveryManager::isInAPMode() {
  return apMode;
}
