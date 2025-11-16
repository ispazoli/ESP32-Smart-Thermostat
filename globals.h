#pragma once

/* ===============================================================
 * GLOBALS.H (v6.5.2 - Stability Patch)
 * =============================================================== */

// --- Libraries ---
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <math.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include "config.h"
#include <ESPping.h> // <-- FIXED: Correct library name (no underscore)

// --- Program Info Structs ---
struct TargetInfo {
  float target;
  String programName;
};
struct ScheduleEvent {
  time_t epochTime;
  float targetTemp;
  String programName;
};

// --- NEW: Advanced Schedule Struct ---
#define MAX_SCHEDULE_POINTS 21 // Max 21 schedule points (e.g., 3 per day)
struct SchedulePoint {
  uint8_t day;     // 0=Sunday, 1=Monday, ... 6=Saturday
  uint16_t time;   // Minutes since midnight (0-1439)
  float target;    // Target temp
  bool enabled;    // Is this point active?
};

// --- Global Variable (Extern Declaration) ---
extern WebServer server;
extern Preferences prefs;
extern float currentTemp;
extern float filtTemp;
extern float hyst;
extern bool heating;
extern bool manualMode;
extern float manualTarget;
extern bool boostActive;
extern uint32_t boostUntil;
extern bool ecoEnabled;
extern bool preHeatActive; // Flag for Pre-Heat UI
extern const float ecoDelta;
extern String lang;

// Simple Schedule (Fallback)
extern String wdMorningTime, wdEveningTime, weMorningTime, weEveningTime;
extern float wdTargetMorning, wdTargetEvening, weTargetMorning, weTargetEvening;

// Advanced Learning
extern bool learnMode;
extern bool learnActive;
extern float learnGainRate;
extern float learnLossCoeff;

// Advanced Schedule Globals
extern bool advancedSchedulingEnabled;
extern int advancedScheduleCount;
extern SchedulePoint advancedSchedule[MAX_SCHEDULE_POINTS];

// --- Presence Detection (Away Mode) ---
extern bool awayMode;
extern float awayTargetTemp;
extern String presenceIPs; // Comma-separated IP addresses
extern int awayTimeoutMins;
extern unsigned long lastPresenceMs; // Timestamp of last successful ping
extern unsigned long lastPingCheck;  // Timer for ping check

// --- NEW: Calendar Integration ---
extern bool calendarMode;     // Is the feature enabled
extern float calendarTarget;  // Target temp from calendar (or NAN)
extern unsigned long lastCalendarFetch; // Timer
// --- END: Calendar Integration ---

// Timers
extern unsigned long lastTempFetch, lastControl, lastLog, lastGas, lastWeatherFetch, lastSwitchMs;
extern unsigned long lastWifiCheck; // <-- WiFi Check Timer

// CPU Load
extern unsigned long cpuBusyMicros;
extern unsigned long cpuWindowStartMs;
extern int cpuLoadPctCached;

// Remote Relay
extern String remoteHost, relayOn, relayOff;
extern int tasmotaFailureCount; // <-- Tasmota Watchdog

// Gas
extern int lastGasDay;
extern float gasMJ_today, gasCost_today;
extern unsigned long onMillis_today;
extern float gasEnergyPerM3, gasPricePerMJ;

// Weather
extern String owApiKey, owCity, owUnits;
extern float owTemp, owFeelsLike, owTempMin, owTempMax, owHum, owPress, owWind;
extern uint32_t owSunrise, owSunset;
extern String owIcon, owDesc;

// Log
extern LogPoint logBuf[LOG_CAP];
extern int logCount;
extern int logHead;


// --- Function Prototypes ---

// main .ino
void setup_wifi_manager();
void setup_mdns();
void setup_ota();
void setup_time();
void checkWiFiConnection(); 

// time_utils.cpp
time_t getEpochNow();
String getTimeString();
bool isWeekendNow();
bool isNightNow();
int toMinutes(const String& hhmm);

// schedule.cpp
ScheduleEvent getNextScheduleEvent();
TargetInfo getActiveProgramInfo();

// control.cpp
void controlHeating();
void updateLearning(float prevT, float nowT, float outdoorT, unsigned long dtMs);
void updateGasEstimation();
void update_cpu_load();
void checkPresence(); 
void updateAwayStatus(); 

// sensors_relay.cpp
void setRelay(bool on);
float getRemoteTemp();
bool fetchOpenWeather();
void rebootTasmota(); // <-- NEW

// storage.cpp
void saveSettings();
void loadSettings();
void saveLogToNVS();
void loadLogFromNVS();
void appendLog(time_t ep, float t);
void logToGoogleSheet(String params); 
void fetchCalendarTarget(); // <-- NEW FUNCTION

// web_server.cpp
void setup_web_server();
void loop_web_server();
void handleStatus();
void handleToggle();
void handleManualHeat();
void handleSave();
void handleSaveLang();
void handleSaveSched();
void handleProSet();
void handleBoost();
void handleEco();
void handleSchedJson();
void handleSettingsJson();
void handleDevInfo();
void handleGasSave();
void handleWeatherSave();
void handleLogJson();
void handleDownloadCSV();
void handleClearLog();
void handleWifiReset();
void handleEnergyJson();
void handleDiagJson();
void handleReboot();
void handleWeatherJson();

// Advanced Schedule Handlers
void handleAdvSchedJson();
void handleAdvSchedSave();
void handleAdvSchedEnable();
void handleAdvSchedDelete(); 

// Presence API
void handlePresenceSave();

// --- Calendar API ---
void handleCalendarSave();

// V55 Module
void V55_afterSetupRegisterRoutes();
void V55_loopTick();
namespace V55 { bool getTPIAdvice(); }