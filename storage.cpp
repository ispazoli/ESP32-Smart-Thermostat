#include "globals.h"

// ----------------- STORAGE (Settings) -----------------
// Saves all configurable settings to NVS (flash memory)
void saveSettings() {
  prefs.putBool("manual", manualMode);
  prefs.putFloat("hyst", hyst);
  prefs.putString("lang", lang);

  prefs.putFloat("wd_tm", wdTargetMorning);
  prefs.putFloat("wd_te", wdTargetEvening);
  prefs.putString("wd_hm", wdMorningTime);
  prefs.putString("wd_he", wdEveningTime);

  prefs.putFloat("we_tm", weTargetMorning);
  prefs.putFloat("we_te", weTargetEvening);
  prefs.putString("we_hm", weMorningTime);
  prefs.putString("we_he", weEveningTime);

  prefs.putBool("learn", learnMode);
  
  // --- MODIFIED for Smart-Learn ---
  // Save the new dual learning variables
  prefs.putFloat("l_gain", learnGainRate); 
  prefs.putFloat("l_loss", learnLossCoeff);
  // ---
  
  prefs.putFloat("g_mj", gasEnergyPerM3);
  prefs.putFloat("g_ft", gasPricePerMJ);

  prefs.putString("ow_city", owCity);
  prefs.putString("ow_units", owUnits);
  prefs.putString("ow_key", owApiKey);

  prefs.putFloat("man_tgt", manualTarget);
  prefs.putBool("eco_en", ecoEnabled);

  // --- Save Advanced Schedule ---
  prefs.putBool("advSchedEn", advancedSchedulingEnabled);
  prefs.putInt("advSchedCnt", advancedScheduleCount);
  // Save the entire array as a binary blob
  prefs.putBytes("advSchedArr", advancedSchedule, sizeof(advancedSchedule)); 
  
  // --- Save Presence Detection ---
  prefs.putString("pres_ips", presenceIPs);
  prefs.putInt("pres_away_m", awayTimeoutMins);
  prefs.putFloat("pres_away_t", awayTargetTemp);
  
  // --- Save Calendar Integration ---
  prefs.putBool("cal_mode", calendarMode);
}

// Loads all settings from NVS on boot
void loadSettings() {
  manualMode = prefs.getBool("manual", false);
  hyst       = prefs.getFloat("hyst", 0.3f);
  lang       = prefs.getString("lang", "hu");

  // --- Load Simple Schedule (fallback) ---
  wdTargetMorning = prefs.getFloat("wd_tm", 21.0f);
  wdTargetEvening = prefs.getFloat("wd_te", 19.0f);
  wdMorningTime   = prefs.getString("wd_hm", "06:00");
  wdEveningTime   = prefs.getString("wd_he", "20:00");
  weTargetMorning = prefs.getFloat("we_tm", 22.0f);
  weTargetEvening = prefs.getFloat("we_te", 20.0f);
  weMorningTime   = prefs.getString("we_hm", "07:00");
  weEveningTime   = prefs.getString("we_he", "22:00");

  learnMode   = prefs.getBool("learn", true);
  learnGainRate = prefs.getFloat("l_gain", NAN);
  learnLossCoeff = prefs.getFloat("l_loss", NAN);

  gasEnergyPerM3 = prefs.getFloat("g_mj", 35.91f);
  gasPricePerMJ  = prefs.getFloat("g_ft", 10904.0f / 3806.0f); // Default Ft/MJ

  owCity   = prefs.getString("ow_city", "Bekescsaba");
  owUnits  = prefs.getString("ow_units", "metric");
  owApiKey = prefs.getString("ow_key", "");

  manualTarget = prefs.getFloat("man_tgt", 21.0f);
  ecoEnabled   = prefs.getBool("eco_en", false);

  // --- Load Advanced Schedule ---
  advancedSchedulingEnabled = prefs.getBool("advSchedEn", false);
  advancedScheduleCount = prefs.getInt("advSchedCnt", 0);
  size_t got = prefs.getBytes("advSchedArr", advancedSchedule, sizeof(advancedSchedule));

  // Sanity check on loaded data
  if (got != sizeof(advancedSchedule) || advancedScheduleCount < 0 || advancedScheduleCount > MAX_SCHEDULE_POINTS) {
    Serial.println("⚠️ Advanced schedule data not found or corrupt. Initializing.");
    advancedScheduleCount = 0;
    memset(advancedSchedule, 0, sizeof(advancedSchedule)); // Zero out the array
  } else {
    Serial.printf("✅ Advanced schedule loaded (%d points).\n", advancedScheduleCount);
  }
  
  // --- Load Presence Detection ---
  presenceIPs = prefs.getString("pres_ips", "");
  awayTimeoutMins = prefs.getInt("pres_away_m", 30);
  awayTargetTemp = prefs.getFloat("pres_away_t", 16.0f);
  // --- END ---
  
  // --- Load Calendar Integration ---
  calendarMode = prefs.getBool("cal_mode", false);
  // --- END ---
}

// ----------------- STORAGE (Log) -----------------

// Saves the entire log buffer (for the web UI) to NVS
void saveLogToNVS() {
  prefs.putBytes("logbuf", logBuf, sizeof(logBuf));
  prefs.putInt("logHead", logHead);
  prefs.putInt("logCount", logCount);
}

// Loads the log buffer from NVS on boot
void loadLogFromNVS() {
  size_t got = prefs.getBytes("logbuf", logBuf, sizeof(logBuf));
  logHead  = prefs.getInt("logHead", 0);
  logCount = prefs.getInt("logCount", 0);
  
  // Basic sanity check to prevent buffer overflow
  if (got != sizeof(logBuf) || logHead < 0 || logHead >= LOG_CAP || logCount < 0 || logCount > LOG_CAP) {
    memset(logBuf, 0, sizeof(logBuf)); // Clear buffer if invalid
    logHead = 0;
    logCount = 0;
  }
}

// Appends a new data point to the circular log buffer (in RAM)
void appendLog(time_t ep, float t) {
  logBuf[logHead] = { (uint32_t)ep, t };
  logHead = (logHead + 1) % LOG_CAP;
  if (logCount < LOG_CAP) logCount++;
  // saveLogToNVS(); // <-- REMOVED to save flash wear.
}

/* ===============================================================
 * GOOGLE SHEET LOGGING (NEW)
 * =============================================================== */

// Certificate (GTS Root R1 - G3)
const char* root_ca_google = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFWjCCA0KgAwIBAgIQCgOc4PoVeKwG+G79NwAEgzANBgkqhkiG9w0BAQsFADBH\n" \
"MQswCQYDVQQGEwJVUzETMBEGA1UEChMKR29vZ2xlIExMQzEYMBYGA1UEAxMPR1RT\n" \
"IFJvb3QgUjEgLSBHMzAeFw0yMTA4MDYxNzQ2NDlaFw0zNjAxMDExNzQ2NDhaMEcx\n" \
"CzAJBgNVBAYTAlVTMRMwEQYDVQQKEwpHb29nbGUgTExDMRgwFgYDVQQDEw9HVFMg\n" \
"Um9vdCBSMSAtIEczMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA7YI3\n" \
"yG3sT8Y2K7YpGJe66DjD6NgglGkvJZQ5Efr2ZWnDAEoEsu3nSRIc3RZ5aQ1bmU4J\n" \
"Avve2Ghc2bVfl8GIBjK4GgfxyERHV6uD7ho2T/FxWjPyDUNEmSW4ocbRiKTU2t9V\n" \
"cVvIpi4UB88aNCPCFcK2IQFz1KH3kk8sL/V1xuUoM+xZGjc77lU5R/+IdUqaD2v5\n" \
"Q/cpxIrsA2N7EDIGnKec28RVoFZh4+y8jYXGmYZui1LB8hHZP7V6oDhnZJv5+EKk\n" \
"RWrRY0N7skpnM/BS0tHruNvqxX15CIGiT3X8rKUI3qTQaF2LAXbYBeycnc9I5FPX\n" \
"vFG1/wEdRL9XnA+FVRCH6DJx6N6AFo0HT/8oVkxieK5Axap3ed/9ThZZH8Lop8E3\n" \
"rFViAwn4llHkJY9A58CA0IVCEKX/A1DYwvdzw6cz5jS2W41oVDM1PU3+7jziNStg\n" \
"i9PBAe7FWm+fGxGTohI7HZxt62obToGpIfiL4hS5T+RZ97GQqX8qTzLw6RQnU81S\n" \
"6oKl64qdlCA1mEiK2c5/4xEbc17zUSkNJnP3gq7cB3kxIITcrQFpbr+7OEDb66zC\n" \lGJ1gypUlZ03WDlN7Ouk9KrWxqL8AeAZ6j+P4m0Jckl8FIPzO64HEljcfO2de41\n" \
"A3KzPByYXqeLg4eTbbYrd0S9EkCAwEAAaNTMFEwHQYDVR0OBBYEFKqx+u6Nvb5Vo\n" \
"ZxBqZk0n5U6GjwOMB8GA1UdIwQYMBaAFKqx+u6Nvb5VoZxBqZk0n5U6GjwOMA8GA\n" \
"1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAHLY+RIbJtPOQKaFpdUK4\n" \
"dw3VyUsGS/lcZ1MuMZKJDJN1zEpqt+ccEPYuKQHwKifCJ4raRimXidPxDof3jG9u\n" \
"7GQ5OYAdVq6sk2fdgGML5ckch7hyUB5vUXfSmGihz6lfbUDCh5BywsBHTN9TXiGY\n" \
"wnYkTTZUl3X5BaBtCMnMFNoAfbzOVdbtwBoW2PtNk4dJ4V1Rz1m4gDQc6UFHMbdf\n" \
"2DXyufk6IUPcAxygDF2QXZCVoMlHkwInDRN+N+pQZsL0DQ98heSAVYueEAK+gt7V\n" \
"N/OXoHkYcnkhZICnkz69nyTFUAsTn+mLeKTVZq4bSBw97mQpO3DdBlpRcP2yx+6E\n" \
"QUY=\n" \
"-----END CERTIFICATE-----\n";

/**
 * @brief Connects to the Google Script URL via HTTPS and sends data.
 * @param params The URL parameters to send (e.g., "?temp=21.5" or "?gas_mj=120")
 */
void logToGoogleSheet(String params) {
  // --- NEW: WiFi Guard ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ [GSheet] No WiFi. Log skipped.");
    return;
  }
  // ---
  
  WiFiClientSecure client;
  HTTPClient http;

  // Set the Google root CA certificate
  //client.setCACert(root_ca_google); // For testing
  client.setInsecure();  
  
  String url = String(G_SCRIPT_URL) + params;
  Serial.println("📤 [GSheet] Sending data to URL: " + url);

  // http.begin() for HTTPS requires the client and the URL
  if (http.begin(client, url)) {
    // Google Scripts redirect, so we must follow
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      Serial.printf("📬 [GSheet] HTTP Code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println("📬 [GSheet] Response: " + payload);
      }
    } else {
      Serial.printf("❌ [GSheet] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("❌ [GSheet] http.begin() failed!");
  }
}

/**
 * @brief Fetches the current calendar target from Google Script.
 */
void fetchCalendarTarget() {
  // --- NEW: WiFi Guard ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ [CAL] No WiFi. Fetch skipped.");
    calendarTarget = NAN; // Clear target on error
    return;
  }
  // ---

  WiFiClientSecure client;
  HTTPClient http;

  client.setCACert(root_ca_google);
  
  String url = String(G_SCRIPT_URL) + "?action=get_calendar";
  Serial.println("🗓️ [CAL] Fetching calendar target from: " + url);

  if (http.begin(client, url)) {
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(4000); // 4 second timeout
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = http.getString();
      Serial.println("🗓️ [CAL] Response: " + payload);
      
      DynamicJsonDocument doc(256);
      DeserializationError err = deserializeJson(doc, payload);
      
      if (err) {
        Serial.printf("❌ [CAL] JSON parse error: %s\n", err.c_str());
        calendarTarget = NAN; // On error, clear the target
      } else {
        if (doc["target"].isNull()) {
          // Google Script reported no active event
          calendarTarget = NAN; 
          Serial.println("🗓️ [CAL] No active calendar event.");
        } else {
          // Success! There is an active event
          calendarTarget = doc["target"].as<float>();
          Serial.printf("🗓️✅ [CAL] Calendar override active. New target: %.1f °C\n", calendarTarget);
        }
      }
    } else {
      Serial.printf("❌ [CAL] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
      calendarTarget = NAN; // On error, clear the target
    }
    http.end();
  } else {
    Serial.println("❌ [CAL] http.begin() failed!");
    calendarTarget = NAN; // On error, clear the target
  }
}