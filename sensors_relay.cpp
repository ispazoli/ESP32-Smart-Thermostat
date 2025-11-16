#include "globals.h"

/**
 * @brief (Internal) Parses Tasmota JSON for relay status.
 * Expects payload from "Status 0" or "Status 11".
 * @return true if ON, false if OFF.
 */
static bool parseTasmotaRelayStatus(const String& payload) {
  DynamicJsonDocument doc(1024); // Smaller doc for Status 0
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("❌ [RELAY] JSON parse error: %s\n", err.c_str());
    return false; // Failsafe
  }

  // 1. Primary search (for your Tasmota v15 device)
  // "StatusSTS":{"POWER":"ON"}
  const char* powerSTS = doc["StatusSTS"]["POWER"];
  if (powerSTS) {
    return (strcmp(powerSTS, "ON") == 0);
  }

  // 2. Reserve search (older Tasmota)
  // "Status":{"POWER":"ON"}
  const char* powerStatus = doc["Status"]["POWER"];
  if (powerStatus) {
    return (strcmp(powerStatus, "ON") == 0);
  }

  // 3. Final reserve (very old Tasmota)
  // "POWER":"ON"
  const char* powerRoot = doc["POWER"];
  if (powerRoot) {
    return (strcmp(powerRoot, "ON") == 0);
  }

  Serial.println("❌ [RELAY] Could not find 'POWER' key in JSON response.");
  return false;
}

/**
 * @brief (Internal) Actively queries the Tasmota device for its relay status.
 * @return true if relay is ON, false if OFF or error.
 */
static bool getRelayStatus() {
  // --- NEW: WiFi Guard ---
  if (WiFi.status() != WL_CONNECTED) {
    return false; // Cannot get status without WiFi
  }
  // ---
  
  HTTPClient http;
  http.setTimeout(2000);
  http.setReuse(false);
  String url = String(remoteHost) + "/cm?cmnd=Status%200";
  
  if (http.begin(url)) {
    int code = http.GET();
    if (code == 200) {
      bool status = parseTasmotaPayloadForTemp(http.getString());
      http.end();
      return status;
    }
    Serial.printf("❌ [RELAY] Status check failed, HTTP %d\n", code);
    http.end();
  }
  return false; // Default to OFF on error
}

/**
 * @brief Sets the relay state with verification (Closed-Loop).
 * Sends a command, then queries the device to ensure the state changed.
 * Only updates the global 'heating' flag on successful verification.
 */
void setRelay(bool on) {
  // --- NEW: WiFi Guard ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ [RELAY] No WiFi. Command skipped.");
    return; // Do not attempt HTTP request
  }
  // ---
  
  HTTPClient http;
  http.setTimeout(2500);
  http.setReuse(false);
  String url = String(remoteHost) + (on ? relayOn : relayOff);
  Serial.println(String("📡 [RELAY] CMD: ") + (on ? "ON" : "OFF") + " → " + url);

  if (http.begin(url)) {
    int code = http.GET();
    Serial.printf("📤 [RELAY] HTTP %d\n", code);
    
    if (code == 200) {
      // --- VERIFICATION STEP ---
      Serial.println("🔍 [RELAY] Command OK. Verifying status...");
      delay(500); // Give Tasmota a moment to process
      
      bool actualState = getRelayStatus();
      Serial.printf("👀 [RELAY] Verified state: %s\n", actualState ? "ON" : "OFF");
      
      if (actualState == on) {
        Serial.println("✅ [RELAY] SUCCESS. State matches command.");
        heating = on; 
      } else {
        Serial.println("❌ [RELAY] ERROR! State MISMATCH. Retrying next cycle.");
        // We do NOT update the 'heating' flag.
      }
      // --- END VERIFICATION ---
      
    } else {
      Serial.println("❌ [RELAY] HTTP error. State unchanged. Retrying next cycle.");
    }
    http.end();
  } else {
    Serial.println("❌ [RELAY] http.begin() failed. State unchanged. Retrying next cycle.");
  }
}

/**
 * @brief (Internal) Parses Tasmota JSON for temperature sensor.
 * @return Temperature as float, or NAN on error.
 */
static float parseTasmotaPayloadForTemp(const String& payload) {
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("❌ [TEMP] JSON parse error: %s\n", err.c_str());
    return NAN;
  }
  // Try various common JSON structures for DS18B20 sensors
  if (doc["StatusSNS"]["DS18B20"]["Temperature"])        return doc["StatusSNS"]["DS18B20"]["Temperature"].as<float>();
  if (doc["StatusSNS"]["DS18B20-1"]["Temperature"])      return doc["StatusSNS"]["DS18B20-1"]["Temperature"].as<float>();
  if (doc["DS18B20"]["Temperature"])                     return doc["DS18B20"]["Temperature"].as<float>();
  if (doc["StatusSNS"].is<JsonObject>()) {
    for (JsonPair kv : doc["StatusSNS"].as<JsonObject>()) {
      if (kv.value().is<JsonObject>() && kv.value()["Temperature"])
        return kv.value()["Temperature"].as<float>();
    }
  }
  return NAN;
}

/**
 * @brief Fetches temperature from the remote Tasmota device.
 * @return Temperature as float, or NAN on error.
 */
float getRemoteTemp() {
  // --- NEW: WiFi Guard ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌡️❌ [TEMP] No WiFi. Read skipped.");
    return NAN; // Do not attempt HTTP request
  }
  // ---
  
  HTTPClient http;
  http.setTimeout(2500);
  http.setReuse(false);
  float temp = NAN;
  
  // Try the "Status 8" command first
  String url1 = String(remoteHost) + "/cm?cmnd=Status%208";
  Serial.println("🌡️ [TEMP] GET " + url1);
  if (http.begin(url1)) {
    int code = http.GET();
    if (code == 200) temp = parseTasmotaPayloadForTemp(http.getString());
    http.end();
  }
  
  // If that fails, try the "Sensor" command
  if (isnan(temp)) {
    String url2 = String(remoteHost) + "/cm?cmnd=Sensor";
    Serial.println("🌡️ [TEMP] GET " + url2);
    if (http.begin(url2)) {
      int code = http.GET();
      if (code == 200) temp = parseTasmotaPayloadForTemp(http.getString());
      http.end();
    }
  }
  
  // Note: We no longer print success here, it's handled by the watchdog
  // if (!isnan(temp)) Serial.printf("🌡️✅ [TEMP] DS18B20 -> %.2f °C\n", temp);
  return temp;
}

/**
 * @brief NEW: Sends a "Restart 1" command to the Tasmota device.
 * This is a "fire and forget" command for emergency watchdog use.
 */
void rebootTasmota() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ [WATCHDOG] No WiFi. Cannot send reboot command.");
    return; 
  }

  HTTPClient http;
  http.setTimeout(1500); // Short timeout, we don't care about the response
  http.setReuse(false);
  String url = String(remoteHost) + "/cm?cmnd=Restart%201";
  Serial.println("📡 [WATCHDOG] CMD: RESTART → " + url);

  if (http.begin(url)) {
    http.GET(); // We send the request but don't wait for/check the code
    http.end();
  } else {
    Serial.println("❌ [WATCHDOG] http.begin() failed for reboot command.");
  }
}


// ----------------- WEATHER (MODIFIED with Fallback) -----------------
/**
 * @brief Fetches weather from OpenWeatherMap API.
 * On failure, sets a safe fallback value for owTemp (10°C)
 * to ensure the learning algorithm continues to function.
 */
bool fetchOpenWeather() {
  if (owApiKey.length() < 5 || owCity.length() < 2) {
    Serial.println("🌦️ [WEATHER] Missing API key or city");
    return false;
  }
  
  // --- NEW: WiFi Guard ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌦️❌ [WEATHER] No WiFi. Fetch skipped.");
    return false;
  }
  // ---
  
  WiFiClientSecure client;
  client.setInsecure(); 
  client.setTimeout(3000);
  HTTPClient http;
  http.setTimeout(2500);
  http.setReuse(false);

  String url = "https://api.openweathermap.org/data/2.5/weather?q=" + owCity + "&units=" + owUnits + "&lang=" + (lang == "hu" ? "hu" : "en") + "&appid=" + owApiKey;
  Serial.println("🌦️ [WEATHER] GET " + url);
  if (!http.begin(client, url)) {
    Serial.println("🌦️❌ [WEATHER] http.begin() failed");
    return false;
  }
  int code = http.GET();
  if (code != 200) {
    Serial.printf("🌦️❌ [WEATHER] HTTP %d\n", code);
    // --- FALLBACK ---
    owTemp = 10.0f; 
    owDesc = "API Error";
    owIcon = "01d";
    // ---
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();
  
  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, payload)) {
    Serial.println("🌦️❌ [WEATHER] JSON error");
    // --- FALLBACK ---
    owTemp = 10.0f; 
    owDesc = "JSON Error";
    owIcon = "01d";
    // ---
    return false;
  }
  
  // Update all global weather variables
  owTemp  = doc["main"]["temp"]       | 10.0f; 
  owFeelsLike = doc["main"]["feels_like"] | owTemp;
  owTempMin = doc["main"]["temp_min"] | owTemp;
  owTempMax = doc["main"]["temp_max"] | owTemp;
  owHum   = doc["main"]["humidity"]   | 50.0f;
  owPress = doc["main"]["pressure"]   | 1013.0f;
  owWind  = doc["wind"]["speed"]      | 0.0f;
  owSunrise = doc["sys"]["sunrise"] | 0;
  owSunset = doc["sys"]["sunset"] | 0;
  
  const char* icon_val = doc["weather"][0]["icon"];
  owIcon = String(icon_val ? icon_val : "01d");
  
  const char* desc_val = doc["weather"][0]["description"];
  owDesc = String(desc_val ? desc_val : "Weather OK");
  
  lastWeatherFetch = millis();
  
  Serial.printf("🌦️✅ %.1f°C (Feels: %.1f°C), Min: %.1f°C, Max: %.1f°C\n",
    owTemp, owFeelsLike, owTempMin, owTempMax);
  Serial.printf("🌦️✅ %s (%s)\n", owDesc.c_str(), owIcon.c_str());
  return true;
}