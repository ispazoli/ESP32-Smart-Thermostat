#include "globals.h"

// ============================================================================
// V55 — Dynamic TPI (stable, no AutoTune) + minimal HTTP status
// ============================================================================
namespace V55 {
  // ... (the beginning of the namespace remains unchanged) ...
  bool     enabled          = true;
  uint32_t cycleMs          = 10UL * 60UL * 1000UL; 
  float    kp_percentPerC   = 30.0f; 
  float    ki_percent       = 0.0f;  
  float    dynHystMinC      = 0.10f; 
  float    dynHystMaxC      = 0.40f; 
  float    dynHystMaxErrC   = 2.00f; 

  uint32_t cycleStartMs     = 0;
  float    integralPct      = 0.0f;  
  float    dutyPct          = 0.0f;  
  bool     adviceHeat       = false; 
  float    dynHystC         = 0.30f; 

  static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
  }
  static inline float mapRange(float x, float in_min, float in_max, float out_min, float out_max) {
    if (x <= in_min) return out_min;
    if (x >= in_max) return out_max;
    float t = (x - in_min) / (in_max - in_min);
    return out_min + t * (out_max - out_min);
  }

  // ---- compute dyn hysteresis ----
  float computeDynHyst(float target, float current) {
    float err = target - current;
    if (err < 0) err = -err;
    float h = mapRange(err, 0.0f, dynHystMaxErrC, dynHystMinC, dynHystMaxC);
    dynHystC = h;
    return h;
  }

  // ---- core TPI update (no relay action here) ----
  void updateTPI() {
    
    // --- NEW: Get the one true target from the central brain ---
    const float currentTarget = getActiveProgramInfo().target;
    // ---

    if (!enabled) {
      adviceHeat = (currentTemp < currentTarget - hyst);
      return;
    }
    
    // dyn hyst always computed
    float dynH = computeDynHyst(currentTarget, currentTemp);

    // proportional + optional integral
    float errorC = currentTarget - currentTemp;
    float p = kp_percentPerC * errorC;
    integralPct += (errorC > 0) ? (ki_percent * errorC) : (-ki_percent * (-errorC));
    integralPct = clampf(integralPct, 0.0f, 100.0f);
    float duty = p + integralPct;
    duty = clampf(duty, 0.0f, 100.0f);
    dutyPct = duty;

    // cycle timing
    uint32_t now = millis();
    if (now - cycleStartMs >= cycleMs) {
      cycleStartMs = now;
    }
    uint32_t onMs = (uint32_t)((duty / 100.0f) * cycleMs);
    uint32_t inCycle = now - cycleStartMs;
    adviceHeat = (inCycle < onMs);
    (void)dynH; // keep variable used
  }

  // === TPI MODIFICATION STEP 2: This function passes the TPI decision ===
  bool getTPIAdvice() {
    return adviceHeat;
  }
  // ====================================================================

  // ---- HTTP: /pro_status (manual JSON, no ArduinoJson dep) ----
  void handleProStatus() {
    String out;
    out.reserve(256);
    out += '{';
    out += "\"mode\":\""; out += (enabled ? "TPI" : "BASIC"); out += "\",";
    out += "\"tpi_duty\":"; out += String((int)dutyPct); out += ',';
    out += "\"tpi_cycle_min\":"; out += String((int)(cycleMs / 60000UL)); out += ',';
    out += "\"dyn_hyst\":"; out += String(dynHystC, 2); out += ',';
    
    const float currentTarget = getActiveProgramInfo().target;
    out += "\"error\":"; out += String(currentTarget - currentTemp, 2); out += ',';
    out += "\"advice_heat\":"; out += (adviceHeat ? "true" : "false");
    out += ',';
    out += "\"takeover\":false";
    out += '}';
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", out);
  }
} // namespace V55

// ---- exported functions required by v5.4 builds ----
void V55_afterSetupRegisterRoutes() {
  using namespace V55;
  server.on("/pro_status", HTTP_GET, []() {
    V55::handleProStatus();
  });
  server.on("/pro_status", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200);
  });
  V55::cycleStartMs = millis();
  Serial.println("🧠 [V55] routes registered; TPI ready.");
}

void V55_loopTick() {
  V55::updateTPI();
}