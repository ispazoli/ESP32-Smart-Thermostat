/*
 * ===============================================================
 * ESP32 SMART THERMOSTAT PRO
 * VERSION: 6.5.3 (Tasmota Watchdog)
 * ===============================================================
 *
 * This firmware turns an ESP32 into an advanced, network-controlled
 * smart thermostat.
 *
 * Key Features:
 * - Advanced Control: V55 TPI algorithm.
 * - Closed-Loop Relay: Controls Tasmota device over HTTP
 * with status verification (not just fire-and-forget).
 * - Remote Logging: Logs all temp/gas data to a Google Sheet
 * via a Google Apps Script Web App.
 * - Web UI: Serves a full, dynamic, multi-theme UI.
 * - Advanced Smart Learning: "Optimal Start" (pre-heating) based
 * on dual-variable learning (Gain Rate & Loss Coeff),
 * correlated with live outdoor weather.
 * - Robust Error Handling: Includes OpenWeather API fallback
 * to prevent learning failures during API outages.
 * - System: Modular code (Refactored), WiFiManager, mDNS, OTA.
 * - Advanced Scheduler: 7-day Advanced Scheduler
 * with full CRUD (Create, Read, Update, Delete) support.
 * - Presence Detection (Away Mode) via local IP Ping
 * - Calendar Integration (via Google Script)
 * - Auto-reconnect WiFi and non-blocking HTTP guards.
 * - NEW: Tasmota Watchdog to reboot frozen remote relay.
 * =============================================================== */

#include "config.h"
#include "globals.h"

// --- Global Variable DEFINITIONS ---
WebServer     server(80);
Preferences   prefs;
// State
float         currentTemp = NAN;
float         filtTemp = NAN;
float         hyst;
bool          heating = false;
bool          manualMode = false;
float         manualTarget;
bool          boostActive = false;
uint32_t      boostUntil = 0;
bool          ecoEnabled = false;
bool          preHeatActive = false;
const float   ecoDelta = -2.0f;
String        lang;
// Simple Schedule (Fallback)
String        wdMorningTime, wdEveningTime, weMorningTime, weEveningTime;
float         wdTargetMorning, wdTargetEvening, weTargetMorning, weTargetEvening;
// Advanced Learning
bool          learnMode;
bool          learnActive = false;
float         learnGainRate = NAN;
float         learnLossCoeff = NAN;
float         learnedRate;
// --- NEW: Advanced Schedule Definitions ---
bool          advancedSchedulingEnabled = false;
int           advancedScheduleCount = 0;
SchedulePoint advancedSchedule[MAX_SCHEDULE_POINTS];
// --- Presence Detection (Away Mode) Definitions ---
bool          awayMode = false;
float         awayTargetTemp = 16.0f;
String        presenceIPs = ""; 
int           awayTimeoutMins = 30; 
unsigned long lastPresenceMs = 0;
unsigned long lastPingCheck = 0;
// --- Calendar integration definitions ---
bool          calendarMode = false;
float         calendarTarget = NAN;
unsigned long lastCalendarFetch = 0;
// Tasmota / Remote Units
String        remoteHost = "http://192.168.1.193";
String        relayOn = "/cm?cmnd=Power%20On";
String        relayOff = "/cm?cmnd=Power%20Off";
int           tasmotaFailureCount = 0; // <-- NEW: Tasmota Watchdog
// Gas
int           lastGasDay = -1;
float         gasMJ_today = 0.0f;
float         gasCost_today = 0.0f;
unsigned long onMillis_today = 0;
float         gasEnergyPerM3, gasPricePerMJ;
// Weather
String        owApiKey, owCity, owUnits;
float         owTemp = NAN, owFeelsLike = NAN, owTempMin = NAN, owTempMax = NAN, owHum = NAN, owPress = NAN, owWind = NAN;
uint32_t      owSunrise = 0, owSunset = 0;
String        owIcon, owDesc;
// Log
LogPoint      logBuf[LOG_CAP];
int           logCount = 0;
int           logHead = 0;
// CPU & Timers
unsigned long cpuBusyMicros = 0;
unsigned long cpuWindowStartMs = 0;
int           cpuLoadPctCached = 0;
unsigned long lastSwitchMs = 0;
// Loop timers
unsigned long lastTempFetch = 0;
unsigned long lastControl = 0;
unsigned long lastGas = 0;
unsigned long lastWeatherFetch = 0;
unsigned long lastLog = 0;
unsigned long lastWifiCheck = 0; 

/* ===============================================================
 * SETUP
 * =============================================================== */
void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("🚀 ==== ISPA Thermostat Pro v6.5.3 (Watchdog) ==== 🚀");
  if(!SPIFFS.begin(true)){
    Serial.println("❌ [!] SPIFFS Mount Failed! Formatting...");
    if(SPIFFS.format()){
       Serial.println("⚠️ [!] SPIFFS Formatted. Please reboot.");
    } else {
       Serial.println("🔥 [!] SPIFFS Format Failed. Halting.");
    }
    while(1);
  }
  Serial.println("✅ SPIFFS Mounted.");

  prefs.begin("thermo", false);

  // --- WiFi, mDNS, Time, OTA ---
  setup_wifi_manager();
  setup_mdns();
  setup_time();
  setup_ota();
  // --- Load settings ---
  loadSettings(); 
  loadLogFromNVS();
  // --- Start presence detection ---
  lastPresenceMs = millis();
  awayMode = false;
  // --- Start calendar integration ---
  lastCalendarFetch = millis() - (5UL * 60UL * 1000UL) + 10000;
  // --- Start web server and modules ---
  setup_web_server();
  V55_afterSetupRegisterRoutes();
  
  Serial.println("🎉 ✅ Setup complete. System running. 🎉");
}


/**
 * @brief NEW: Non-blocking WiFi self-healing function.
 * Checks connection every 5 minutes and reconnects if lost.
 */
void checkWiFiConnection() {
  unsigned long now = millis();
  if (now - lastWifiCheck < (5UL * 60UL * 1000UL)) { // 5 minutes
    return; // Not time to check yet
  }
  lastWifiCheck = now;

  if (WiFi.status() == WL_CONNECTED) {
    // Serial.println("📡 [WIFI] Connection OK.");
    return;
  }

  Serial.println("📡❌ [WIF] WiFi connection lost! Attempting reconnect...");
  
  WiFi.reconnect();
  
  delay(100); 
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("📡✅ [WIF] Reconnected successfully!");
  } else {
    Serial.println("📡⚠️ [WIF] Reconnect attempt failed, will try again later.");
  }
}


/* ===============================================================
 * LOOP
 * =============================================================== */
void loop() {
  // --- Always-on tasks ---
  loop_web_server();
  ArduinoOTA.handle();
  update_cpu_load();
  unsigned long now = millis();

  // --- NEW: Run WiFi Self-Heal Check ---
  checkWiFiConnection();
  
  // Run presence detection (per minute)
  if (now - lastPingCheck >= 60000) { 
    lastPingCheck = now;
    if (presenceIPs.length() > 5) { 
      checkPresence();
    }
    updateAwayStatus();
  }

  // --- Run calendar query (every 5 minutes) ---
  if (now - lastCalendarFetch >= (5UL * 60UL * 1000UL)) {
    lastCalendarFetch = now;
    if (calendarMode) { 
      fetchCalendarTarget();
    }
  }

  // --- 1. Read Temperature (every 10 seconds) ---
  if (now - lastTempFetch >= 10000) {
    lastTempFetch = now;
    unsigned long _t0 = micros();

    float prevTemp = filtTemp; 
    float newTemp = getRemoteTemp(); // This is the critical call
    cpuBusyMicros += (micros() - _t0);
    
    if (!isnan(newTemp)) {
      // --- SUCCESS ---
      currentTemp = newTemp;
      tasmotaFailureCount = 0; // <-- NEW: Reset watchdog counter on success

      if (isnan(filtTemp)) {
        filtTemp = currentTemp;
      } else {
        filtTemp = (EMA_ALPHA * currentTemp) + (1.0f - EMA_ALPHA) * filtTemp;
      }
      
      if (learnMode) {
        updateLearning(prevTemp, filtTemp, owTemp, 10000);
      }
      
    } else {
      // --- FAILURE ---
      Serial.println("🌡️❌ [!] Temperature read failed!");
      tasmotaFailureCount++; // <-- NEW: Increment watchdog counter on failure
      
      // --- NEW: Tasmota Watchdog ---
      if (tasmotaFailureCount >= 3) { // 3 consecutive failures (30 seconds)
        Serial.println("🔥 [WATCHDOG] Tasmota sensor unresponsive for 30s!");
        Serial.println("🔥 [WATCHDOG] Forcing relay OFF as a failsafe...");
        setRelay(false); // Failsafe: Try to turn it off
        
        Serial.println("🔥 [WATCHDOG] Sending RESTART command to Tasmota...");
        rebootTasmota(); // Send the reboot command
        
        tasmotaFailureCount = 0; // Reset counter to prevent boot loop
      }
      // --- END Watchdog ---
    }
  }

  // --- 2. Control Logic (every 10s, after temp read) ---
  if (now - lastControl >= 10000) {
    lastControl = now;
    if (!isnan(currentTemp)) { // This guard now protects against NAN
      V55_loopTick(); 
      controlHeating();
    }
  }

  // --- 3. Logging (every 15 minutes) ---
  if (now - lastLog >= (15UL * 60UL * 1000UL)) {
    lastLog = now;
    if (!isnan(currentTemp)) { // Only log if temp is valid
      Serial.println("📝 [LOG] Saving 15-minute log...");
      appendLog(getEpochNow(), filtTemp);
      if (G_SCRIPT_ENABLED) {
        String params = "?temp=" + String(filtTemp, 2);
        logToGoogleSheet(params);
      }
    }
  }

  // --- 4. Gas/Energy statistics (every 2 seconds) ---
  if (now - lastGas >= 2000) {
    lastGas = now;
    updateGasEstimation();
  }

  // --- 5. Weather (every 10 minutes) ---
  if (now - lastWeatherFetch >= (10UL * 60UL * 1000UL)) {
    lastWeatherFetch = now;
    unsigned long _t0 = micros();
    fetchOpenWeather(); 
    cpuBusyMicros += (micros() - _t0);
  }
}