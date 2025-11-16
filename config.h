#pragma once

// ----------------- CONFIG -----------------
#define HOSTNAME "esp32thermostat"
static const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;      // CET
const int   daylightOffset_sec = 3600; // CEST

// ===== Safety & Gas =====
#ifndef MIN_ON_MS
  #define MIN_ON_MS 300000UL   // 5 minutes
#endif
#ifndef MIN_OFF_MS
  #define MIN_OFF_MS 300000UL  // 5 minutes
#endif

#ifndef BOILER_POWER_KW
  #define BOILER_POWER_KW 20.0f
#endif
#ifndef BOILER_EFFICIENCY
  #define BOILER_EFFICIENCY 0.88f
#endif

// Filtering
static const float EMA_ALPHA = 0.10f;       // For main temperature sensor
static const float EMA_LEARN_ALPHA = 0.10f; // For learning filters

// Log
static const int LOG_CAP = 192; // 48h * 4/h
struct LogPoint {
  uint32_t epoch;
  float temp;
};

/* ===============================================================
 * GOOGLE SHEET LOGGING (Set this to your Web App URL)
 * =============================================================== */
#define G_SCRIPT_ENABLED true
// Paste your SECRET URL
#define G_SCRIPT_URL "https://script.google.com/macros/s/exec" // <-- Your Sheet Link
