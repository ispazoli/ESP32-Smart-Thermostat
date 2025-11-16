#include "globals.h"

// ----------------- WIFI / MDNS / TIME / OTA SETUP -----------------
// This file now ONLY contains setup helper functions.
// Time and Schedule logic has been moved to time_utils.cpp and schedule.cpp
// ------------------------------------------------------------------

/**
 * @brief Sets up WiFi connection using WiFiManager.
 * Falls back to AP mode if connection fails.
 */
void setup_wifi_manager() {
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConnectTimeout(20);
  wm.setConfigPortalTimeout(240);
  bool ok = wm.autoConnect("ESP_Thermostat_Setup");
  if (!ok) {
    Serial.println("⚠️ Wi-Fi failed → AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP_Thermostat_Setup");
    Serial.print("📡 AP IP: "); Serial.println(WiFi.softAPIP());
  } else {
    Serial.print("✅ Wi-Fi: "); Serial.println(WiFi.localIP());
  }
}

/**
 * @brief Sets up MDNS (Bonjour) service.
 */
void setup_mdns() {
  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("🌐 mDNS: http://" + String(HOSTNAME) + ".local");
  } else {
    Serial.println("⚠️ mDNS start failed");
  }
}

/**
 * @brief Sets up and synchronizes NTP time.
 */
void setup_time() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  time_t now = time(nullptr);
  unsigned long _ntpStart = millis();

  // Wait max 20 sec for NTP
  while (now < 100000 && millis() - _ntpStart < 20000) {
    delay(500);
    now = time(nullptr);
    Serial.print(".");
  }

  if (now < 100000) {
    Serial.println("\n⏰ [!] NTP timeout, continuing without full sync.");
  } else {
    Serial.println("\n✅ NTP synchronized successfully!");
  }
  Serial.println();
}

/**
 * @brief Sets up Over-The-Air (OTA) update functionality.
 */
void setup_ota() {
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    Serial.println("🔄 [OTA] Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\n✨ [OTA] End");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("⏳ [OTA] Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("❌ [OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}