#include "globals.h"

// --- File Request Handler ---
// This feature serves all static files (index.html, style.css, app.js)
// Also supports Gzip compression if you upload files with .gz extension.
bool handleFileRequest(String path) {
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz)) {
    path = pathWithGz;
    Serial.println("🌐 [HTTP] Serving GZIPPED file: " + path);
  } else if (SPIFFS.exists(path)) {
    Serial.println("🌐 [HTTP] Serving file: " + path);
  } else {
    Serial.println("❌ [HTTP] File NOT FOUND: " + path);
    return false;
  }

  String contentType;
  if (path.endsWith(".html.gz") || path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css.gz") || path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js.gz") || path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".json")) contentType = "application/json";
  else contentType = "text/plain";

  File file = SPIFFS.open(path, "r");
  if (!file) return false;

  if (path.endsWith(".gz")) {
    server.sendHeader("Content-Encoding", "gzip");
  }
  
  // Enable caching in the browser for 1 hour (reduces load)
  server.sendHeader("Cache-Control", "public, max-age=3600");
  server.streamFile(file, contentType);
  file.close();
  return true;
}

// --- /status (MODIFIED for Pre-Heat UI and Away Mode) ---
void handleStatus() {
  long left = (boostActive) ? (long)((boostUntil - millis()) / 1000) : 0;
  if (left <= 0) { left = 0; boostActive = false; }
  TargetInfo info = getActiveProgramInfo(); // This now sets preHeatActive flag
  String j = "{";
  
  j += "\"temp\":" + (isnan(currentTemp) ? String("null") : String(currentTemp, 1)) + ",";
  j += "\"heat\":"; j += (heating ? "true" : "false"); j += ",";
  j += "\"manual\":"; j += (manualMode ? "true" : "false"); j += ",";
  j += "\"mt\":" + String(manualTarget, 1) + ",";
  j += "\"boost\":"; j += (boostActive ? "true" : "false"); j += ",";
  j += "\"boostLeft\":" + String(left) + ",";
  j += "\"eco\":"; j += (ecoEnabled ? "true" : "false"); j += ",";
  j += "\"preheat\":"; j += (preHeatActive ? "true" : "false"); j += ","; 
  j += "\"isNight\":"; j += (isNightNow() ? "true" : "false"); j += ",";
  j += "\"away\":"; j += (awayMode ? "true" : "false"); j += ","; // <-- NEW: Away Mode status
  j += "\"tgt\":" + String(info.target, 1) + ",";
  j += "\"prog\":\"" + info.programName + "\",";
  j += "\"time\":\"" + getTimeString() + "\"";
  j += "}";
  server.send(200, "application/json", j);
}

// --- All other handle... functions ---
void handleToggle() {
  manualMode = !manualMode;
  saveSettings();
  server.send(200, "text/plain", manualMode ? "manual" : "auto");
}
void handleManualHeat() {
  if (!manualMode) { server.send(400, "text/plain", "not-manual"); return; }
  bool wantOn = server.hasArg("on") ? (server.arg("on") == "1") : !heating;
  setRelay(wantOn);
  server.send(200, "text/plain", heating ? "on" : "off");
}
void handleSave() {
  if (server.hasArg("hyst")) {
    float v = server.arg("hyst").toFloat();
    if (v < 0.1f) v = 0.1f;
    if (v > 1.5f) v = 1.5f;
    hyst = v;
  }
  if (server.hasArg("learn")) learnMode = (server.arg("learn") == "1");
  saveSettings();
  server.send(200, "text/plain", "ok");
}
void handleSaveLang() {
  if (server.hasArg("lang")) {
    String L = server.arg("lang");
    if (L == "hu" || L == "en") { lang = L; saveSettings(); }
  }
  server.send(200, "text/plain", "ok");
}
void handleSaveSched() {
  if (server.hasArg("wd_m")) wdMorningTime = server.arg("wd_m");
  if (server.hasArg("wd_e")) wdEveningTime = server.arg("wd_e");
  if (server.hasArg("we_m")) weMorningTime = server.arg("we_m");
  if (server.hasArg("we_e")) weEveningTime = server.arg("we_e");
  if (server.hasArg("wd_tm")) wdTargetMorning = server.arg("wd_tm").toFloat();
  if (server.hasArg("wd_te")) wdTargetEvening = server.arg("wd_te").toFloat();
  if (server.hasArg("we_tm")) weTargetMorning = server.arg("we_tm").toFloat();
  if (server.hasArg("we_te")) weTargetEvening = server.arg("we_te").toFloat();
  saveSettings();
  server.send(200, "text/plain", "ok");
}
void handleProSet() {
  if (server.hasArg("mt")) { manualTarget = server.arg("mt").toFloat(); saveSettings(); }
  server.send(200, "text/plain", "ok");
}
void handleBoost() {
  bool on = server.hasArg("on") ? (server.arg("on") == "1") : !boostActive;
  if (on) {
    boostActive = true;
    boostUntil = millis() + 30UL * 60UL * 1000UL;
    manualMode = true; 
    setRelay(true);    
  } else {
    boostActive = false;
    boostUntil = 0;
    setRelay(false);   
  }
  saveSettings();
  server.send(200, "text/plain", boostActive ? "boost-on" : "boost-off");
}
void handleEco() {
  bool on = server.hasArg("on") ? (server.arg("on") == "1") : !ecoEnabled;
  ecoEnabled = on;
  saveSettings();
  server.send(200, "text/plain", ecoEnabled ? "eco-on" : "eco-off");
}
void handleSchedJson() {
  String j = "{";
  j += "\"wd_m\":\"" + wdMorningTime + "\",";
  j += "\"wd_e\":\"" + wdEveningTime + "\",";
  j += "\"we_m\":\"" + weMorningTime + "\",";
  j += "\"we_e\":\"" + weEveningTime + "\",";
  j += "\"wd_tm\":" + String(wdTargetMorning, 1) + ",";
  j += "\"wd_te\":" + String(wdTargetEvening, 1) + ",";
  j += "\"we_tm\":" + String(weTargetMorning, 1) + ",";
  j += "\"we_te\":" + String(weTargetEvening, 1);
  j += "}";
  server.send(200, "application/json", j);
}

// --- /settings (MODIFIED for Adv. Sched, Presence, Calendar) ---
void handleSettingsJson() {
  String j = "{";
  j += "\"hyst\":" + String(hyst, 1) + ",";
  j += "\"learnMode\":"; j += (learnMode ? "true" : "false"); j += ",";
  j += "\"learnActive\":"; j += (learnActive ? "true" : "false"); j += ",";
  j += "\"lang\":\"" + lang + "\",";
  j += "\"gmj\":" + String(gasEnergyPerM3, 2) + ",";
  j += "\"gft\":" + String(gasPricePerMJ, 3) + ",";
  j += "\"advSchedEn\":"; j += (advancedSchedulingEnabled ? "true" : "false"); j += ","; 
  // --- Presence Detection Data ---
  j += "\"pres_ips\":\"" + presenceIPs + "\",";
  j += "\"pres_mins\":" + String(awayTimeoutMins) + ",";
  j += "\"pres_temp\":" + String(awayTargetTemp, 1) + ",";
  // --- NEW: Calendar Data ---
  j += "\"cal_mode\":"; j += (calendarMode ? "true" : "false");
  // --- END ---
  j += "}";
  server.send(200, "application/json", j);
}
void handleDevInfo() {
  uint32_t s = millis() / 1000;
  char b[32];
  snprintf(b, sizeof(b), "%u:%02u:%02u", (unsigned)(s / 3600), (unsigned)((s / 60) % 60), (unsigned)(s % 60));
  String h;
  h.reserve(350);
  h += F("<div style='display:grid; grid-template-columns:120px auto; row-gap:4px; column-gap:12px; font-size:13px; padding:8px 4px; background: rgba(255,255,255,0.30); border-radius:12px; box-shadow: inset 0 1px 3px rgba(0,0,0,0.08)'>");
  h += F("<div style='font-weight:600; opacity:.85'>IP</div><div>"); h += WiFi.localIP().toString(); h += F("</div>");
  h += F("<div style='font-weight:600; opacity:.85'>Host</div><div>"); h += String(HOSTNAME); h += F(".local</div>");
  h += F("<div style='font-weight:600; opacity:.85'>SSID</div><div>"); h += WiFi.SSID(); h += F("</div>");
  h += F("<div style='font-weight:600; opacity:.85'>RSSI</div><div>"); h += String(WiFi.RSSI()); h += F(" dBm</div>");
  h += F("<div style='font-weight:600; opacity:.85'>Uptime</div><div>"); h += b; h += F("</div>");
  h += F("</div>");
  server.send(200, F("text/html"), h);
}
void handleGasSave() {
  if (server.hasArg("mj")) gasEnergyPerM3 = server.arg("mj").toFloat();
  if (server.hasArg("ft")) gasPricePerMJ  = server.arg("ft").toFloat();
  saveSettings();
  server.send(200, "text/plain", "ok");
}
void handleWeatherSave() {
  if (server.hasArg("city")) owCity = server.arg("city");
  if (server.hasArg("key"))  owApiKey = server.arg("key");
  saveSettings();
  unsigned long _tw0 = micros();
  bool _fw = fetchOpenWeather();
  cpuBusyMicros += (micros() - _tw0);
  (void)_fw;
  server.send(200, "text/plain", "ok");
}
void handleLogJson() {
  String out = "[";
  for (int i = 0; i < logCount; i++) {
    int idx = (logHead - logCount + i + LOG_CAP) % LOG_CAP;
    out += "{\"x\":" + String((uint32_t)logBuf[idx].epoch) + ",\"t\":" + String(logBuf[idx].temp, 1) + "}";
    if (i != logCount - 1) out += ",";
  }
  out += "]";
  server.send(200, "application/json", out);
}
void handleDownloadCSV() {
  String csv = "timestamp,iso8601,temp_c\n";
  for (int i = 0; i < logCount; i++) {
    int idx = (logHead - logCount + i + LOG_CAP) % LOG_CAP;
    time_t ep = (time_t)logBuf[idx].epoch;
    struct tm t;
    gmtime_r(&ep, &t);
    char b[24];
    snprintf(b, sizeof(b), "%04d-%02d-%02dT%02d:%02d:%02dZ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    csv += String((uint32_t)ep) + "," + String(b) + "," + String(logBuf[idx].temp, 1) + "\n";
  }
  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=\"esp_thermostat_log.csv\"");
  server.send(200, "text/csv", csv);
}
void handleClearLog() {
  memset(logBuf, 0, sizeof(logBuf));
  logHead = 0;
  logCount = 0;
  saveLogToNVS();
  server.send(200, "text/plain", "ok");
}
void handleWifiReset() {
  WiFiManager wm;
  wm.resetSettings();
  server.send(200, "text/plain", "rebooting");
  delay(300);
  ESP.restart();
}
void handleEnergyJson() {
  updateGasEstimation();
  float gasM3_today = (gasEnergyPerM3 > 0.1f) ? (gasMJ_today / gasEnergyPerM3) : 0.0f;
  String j = "{";
  j += "\"mj\":" + String(gasMJ_today, 1) + ",";
  j += "\"m3\":" + String(gasM3_today, 2) + ",";
  j += "\"cost\":" + String(gasCost_today, 0);
  j += "}";
  server.send(200, "application/json", j);
}

// --- /diag.json (MODIFIED for Smart-Learn AND Presence Ping) ---
void handleDiagJson() {
  uint32_t up_s = millis() / 1000;
  long rssi = WiFi.isConnected() ? WiFi.RSSI() : -200;
  uint32_t heapFree = ESP.getFreeHeap();
  uint32_t heapSize = ESP.getHeapSize();
  int heapPct = heapSize ? (int)((heapSize - heapFree) * 100 / heapSize) : 0;
  int cpuPct = cpuLoadPctCached;
  if (cpuPct > 95) cpuPct = 95;
  bool ntp = (getEpochNow() > 1577836800);
  String j = "{";
  j += "\"device\":\"" + String(HOSTNAME) + "\",";
  j += "\"uptime_s\":" + String(up_s) + ",";
  j += "\"rssi\":" + String(rssi) + ",";
  j += "\"heap_used_pct\":" + String(heapPct) + ",";
  j += "\"heap_free\":" + String(heapFree) + ",";
  j += "\"heap_size\":" + String(heapSize) + ",";
  j += "\"cpu_pct\":" + String(cpuPct) + ",";
  j += "\"learnGainRate\":" + String(learnGainRate, 2) + ",";
  j += "\"learnLossCoeff\":" + String(learnLossCoeff, 4) + ",";
  j += "\"ntp_sync\":"; j += (ntp ? "true" : "false"); j += ",";
  
  // --- Add live presence data ---
  // We use lastPresenceMs which is updated by checkPresence() in control.cpp
  unsigned long presence_last_s = (millis() - lastPresenceMs) / 1000UL;
  j += "\"presence_last_s\":"; j += String(presence_last_s); j += ",";
  // --- END ---

  j += "\"wifi_ssid\":\"" + WiFi.SSID() + "\",";
  j += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  j += "}";
  server.send(200, "application/json", j);
}

void handleWeatherJson() {
  String j = "{";
  j += "\"ok\":"; j += (!isnan(owTemp) ? "true" : "false"); j += ",";
  j += "\"city\":\"" + owCity + "\",";
  j += "\"temp\":" + String(owTemp, 1) + ",";
  j += "\"feels\":" + String(owFeelsLike, 1) + ",";
  j += "\"temp_min\":" + String(owTempMin, 1) + ",";
  j += "\"temp_max\":" + String(owTempMax, 1) + ",";
  j += "\"hum\":" + String(owHum, 0) + ",";
  j += "\"press\":" + String(owPress, 0) + ",";
  j += "\"wind\":" + String(owWind, 1) + ",";
  j += "\"sunrise\":" + String(owSunrise) + ",";
  j += "\"sunset\":" + String(owSunset) + ",";
  j += "\"icon\":\"" + owIcon + "\",";
  j += "\"desc\":\"" + owDesc + "\",";
  j += "\"keyset\":"; j += (owApiKey.length() > 5 ? "true" : "false");
  j += "}";
  server.send(200, "application/json", j);
}

void handleReboot() {
  String guard = server.hasHeader("X-Confirm") ? server.header("X-Confirm") : "";
  if (guard == "ctrl") {
    server.send(200, "text/plain", "rebooting");
    delay(200);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "need-ctrl");
  }
}

// --- Advanced Schedule Handlers ---
void handleAdvSchedJson() {
  String j = "[";
  for (int i = 0; i < advancedScheduleCount; i++) {
    j += "{";
    j += "\"idx\":" + String(i) + ","; // Send index for easier editing
    j += "\"day\":" + String(advancedSchedule[i].day) + ",";
    j += "\"time\":" + String(advancedSchedule[i].time) + ",";
    j += "\"temp\":" + String(advancedSchedule[i].target, 1) + ",";
    j += "\"enabled\":"; j += (advancedSchedule[i].enabled ? "true" : "false");
    j += "}";
    if (i < advancedScheduleCount - 1) j += ",";
  }
  j += "]";
  server.send(200, "application/json", j);
}

void handleAdvSchedSave() {
  if (!server.hasArg("day") || !server.hasArg("time") || !server.hasArg("temp") || !server.hasArg("enabled")) {
    server.send(400, "text/plain", "Missing args");
    return;
  }
  
  int day = server.arg("day").toInt();
  int time = server.arg("time").toInt();
  float temp = server.arg("temp").toFloat();
  bool enabled = (server.arg("enabled") == "true");

  if (day < 0 || day > 6 || time < 0 || time > 1439 || temp < 5 || temp > 30) {
    server.send(400, "text/plain", "Invalid data");
    return;
  }

  // Find a free spot or an existing spot to update
  int idx = -1;
  if (server.hasArg("idx")) {
    idx = server.arg("idx").toInt();
    if (idx < 0 || idx >= advancedScheduleCount) idx = -1; // Invalid index
  }

  if (idx == -1) { 
    if (advancedScheduleCount >= MAX_SCHEDULE_POINTS) {
      server.send(400, "text/plain", "Schedule full");
      return;
    }
    idx = advancedScheduleCount;
    advancedScheduleCount++;
  }
  
  // Save the point
  advancedSchedule[idx] = { (uint8_t)day, (uint16_t)time, temp, enabled };
  
  saveSettings(); // Save all settings (including the new array)
  Serial.printf("✅ [SCHED] Saved advanced schedule point %d\n", idx);
  server.send(200, "text/plain", "ok");
}

void handleAdvSchedDelete() {
  if (!server.hasArg("idx")) {
    server.send(400, "text/plain", "Missing index");
    return;
  }
  
  int idx = server.arg("idx").toInt();
  
  if (idx < 0 || idx >= advancedScheduleCount) {
    server.send(400, "text/plain", "Invalid index");
    return;
  }

  Serial.printf("🗑️ [SCHED] Deleting advanced schedule point %d\n", idx);

  // Shift all subsequent elements one position to the left
  for (int i = idx; i < advancedScheduleCount - 1; i++) {
    advancedSchedule[i] = advancedSchedule[i + 1];
  }
  
  // Decrement the count
  advancedScheduleCount--;
  
  saveSettings(); // Save the modified array and new count
  server.send(200, "text/plain", "deleted");
}


void handleAdvSchedEnable() {
  if (server.hasArg("enable")) {
    advancedSchedulingEnabled = (server.arg("enable") == "1");
    saveSettings();
    server.send(200, "text/plain", "ok");
  } else {
    server.send(400, "text/plain", "Missing 'enable' arg");
  }
}

// --- Presence Detection API Save ---
void handlePresenceSave() {
  if (server.hasArg("ips")) {
    presenceIPs = server.arg("ips");
  }
  if (server.hasArg("mins")) {
    awayTimeoutMins = server.arg("mins").toInt();
    if (awayTimeoutMins < 5) awayTimeoutMins = 5; // Minimum 5 mins
  }
  if (server.hasArg("temp")) {
    awayTargetTemp = server.arg("temp").toFloat();
    if (awayTargetTemp < 5) awayTargetTemp = 5;
    if (awayTargetTemp > 18) awayTargetTemp = 18; // Max 18 C
  }
  saveSettings();
  server.send(200, "text/plain", "ok");
}

// --- Calendar Integration API Save ---
void handleCalendarSave() {
  if (server.hasArg("enable")) {
    calendarMode = (server.arg("enable") == "1");
    saveSettings();
    // Immediately triggers a fetch to update data
    if (calendarMode) {
      fetchCalendarTarget();
    } else {
      calendarTarget = NAN; // If we turn it off, clear the target
    }
    server.send(200, "text/plain", "ok");
  } else {
    server.send(400, "text/plain", "Missing 'enable' arg");
  }
}


// --- Web Server Setup ---
void setup_web_server() {
  // CORS (for development)
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type,X-Confirm");
      server.send(204);
    } else {
      // If not an API call, try to serve the file from SPIFFS
      if (!handleFileRequest(server.uri())) {
        server.send(404, "text/plain", "Not Found (API or File)");
      }
    }
  });

  // --- STATIC FILES ---
  server.on("/", HTTP_GET, []() {
    if (!handleFileRequest("/index.html")) {
      server.send(404, "text/plain", "index.html not found");
    }
  });
  server.on("/style.css", HTTP_GET, []() {
    if (!handleFileRequest("/style.css")) {
      server.send(404, "text/plain", "style.css not found");
    }
  });
  server.on("/app.js", HTTP_GET, []() {
    if (!handleFileRequest("/app.js")) {
      server.send(404, "text/plain", "app.js not found"); // FIXED from 44
    }
  });

  // --- API ENDPOINTS ---
  server.on("/status", handleStatus);
  server.on("/toggle", handleToggle);
  server.on("/manual_heat", handleManualHeat);
  server.on("/save", handleSave);
  server.on("/save_lang", handleSaveLang);
  server.on("/save_sched", handleSaveSched);
  server.on("/sched", handleSchedJson); 
  server.on("/settings", handleSettingsJson); 
  server.on("/devinfo", handleDevInfo);
  server.on("/energy", handleEnergyJson);
  server.on("/gassave", handleGasSave);
  server.on("/weather", handleWeatherJson);
  server.on("/weathersave", handleWeatherSave);
  server.on("/logdata", handleLogJson);
  server.on("/download", handleDownloadCSV);
  server.on("/clearlog", handleClearLog);
  server.on("/wifi_reset", handleWifiReset);
  server.on("/pro_set", handleProSet);
  server.on("/boost", handleBoost);
  server.on("/eco", handleEco);
  server.on("/diag.json", handleDiagJson);
  server.on("/reboot", handleReboot);
  
  // --- Advanced Schedule API ---
  server.on("/adv_sched_get", handleAdvSchedJson);
  server.on("/adv_sched_save", handleAdvSchedSave);
  server.on("/adv_sched_enable", handleAdvSchedEnable);
  server.on("/adv_sched_delete", handleAdvSchedDelete);
  
  // --- Presence API ---
  server.on("/save_presence", handlePresenceSave);
  
  // --- Calendar API ---
  server.on("/save_calendar", handleCalendarSave);
  
  server.begin();
}

void loop_web_server() {
  server.handleClient();
}