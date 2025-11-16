#include "globals.h"

// --- Helper function for Exponential Moving Average ---
// Provides a NaN-safe way to apply an EMA filter.
static float ema(float oldVal, float newVal, float alpha) {
  if (isnan(oldVal)) {
    return newVal;
  }
  return (alpha * newVal) + ((1.0f - alpha) * oldVal);
}

// ----------------- LEARNING & CONTROL (TPI-based) -----------------
void controlHeating() {
  unsigned long _t0micros = micros();
  // Current temperature (filtered or raw)
  float T = !isnan(filtTemp) ? filtTemp : currentTemp;
  if (isnan(T)) return;

  // Get TPI module's advice
  // This is calculated by V55_loopTick(), which runs before this
  bool wantHeat = V55::getTPIAdvice();

  // Get target temperature from the new brain
  // *** THIS FUNCTION NOW ALSO CARES ABOUT "awayMode" ***
  const TargetInfo targetInfo = getActiveProgramInfo();
  const float target = targetInfo.target;
  const unsigned long now = millis();

  // TPI-based control with boiler safety timers
  if (!heating) {
    // State: OFF -> ON
    if (wantHeat && (now - lastSwitchMs) >= MIN_OFF_MS) {
      Serial.printf("🔥 [CTRL-TPI] ON: TPI advice ON (T=%.2f, Target=%.2f, P=%s)\n", T, target, targetInfo.programName.c_str());
      setRelay(true);
      lastSwitchMs = now;
    }
  } else {
    // State: ON -> OFF
    if (!wantHeat && (now - lastSwitchMs) >= MIN_ON_MS) {
      Serial.printf("❄️ [CTRL-TPI] OFF: TPI advice OFF (T=%.2f, Target=%.2f, P=%s)\n", T, target, targetInfo.programName.c_str());
      setRelay(false);
      lastSwitchMs = now;
    }
  }

  cpuBusyMicros += (micros() - _t0micros);
}
// =======================================================

// ===== Smart Learning (Dual-Variable) =====
// This function is now much smarter. It learns two separate values:
// 1. learnGainRate: The boiler's heating power (°C/h)
// 2. learnLossCoeff: The building's heat loss coefficient (unitless)
// It uses the outdoor temperature to separate these two factors.
void updateLearning(float prevT, float nowT, float outdoorT, unsigned long dtMs) {
  if (isnan(prevT) || isnan(nowT) || isnan(outdoorT) || dtMs == 0) {
    return; // Need all data points to learn
  }
  
  // --- Don't study if you are in awayMode ---
  if (awayMode) {
    return;
  }

  float dT = nowT - prevT;
  float rateCph = dT * (3600000.0f / (float)dtMs); // Measured °C/hour
  
  // Delta T (Ambient) is the driver for heat loss
  float deltaAmbient = nowT - outdoorT;

  if (heating && dT > 0) {
    // --- STATE 1: HEATING (We are warming up) ---
    // We are measuring Net Gain.
    // NetGain = GrossGain (Boiler) - HeatLoss
    // So, GrossGain = NetGain + HeatLoss
    
    // Estimate heat loss during this period using our *current* coefficient
    float estimatedLossCph = 0.0f;
    if (!isnan(learnLossCoeff) && deltaAmbient > 0) {
      estimatedLossCph = learnLossCoeff * deltaAmbient;
    }

    // Calculate the boiler's actual (gross) heating power
    float grossGainRate = rateCph + estimatedLossCph;

    // Apply EMA to our learned boiler gain rate
    learnGainRate = ema(learnGainRate, grossGainRate, EMA_LEARN_ALPHA);
    
    Serial.printf("🧠📈 [LEARN] HEATING: Net=%.2f°C/h, Est.Loss=%.2f°C/h, GrossGain=%.2f°C/h -> AvgGain=%.2f\n", 
                  rateCph, estimatedLossCph, grossGainRate, learnGainRate);

  } else if (!heating && dT < 0) {
    // --- STATE 2: COOLING (We are losing heat) ---
    // We are measuring Net Loss.
    // NetLoss = HeatLoss
    
    // We can only learn if it's colder outside (deltaAmbient > 0)
    if (deltaAmbient <= 0.1f) {
      return; // Can't learn loss if it's warm outside
    }

    // Calculate the coefficient
    // Loss (°C/h) = Coeff * (Temp_In - Temp_Out)
    // Coeff = Loss / (Temp_In - Temp_Out)
    // 'rateCph' is negative, so we use -rateCph to get a positive Loss
    float newLossCoeff = -rateCph / deltaAmbient;

    // Apply EMA to our learned loss coefficient
    learnLossCoeff = ema(learnLossCoeff, newLossCoeff, EMA_LEARN_ALPHA);

    Serial.printf("🧠📉 [LEARN] COOLING: Net=%.2f°C/h, T_Delta=%.2f°C -> NewCoeff=%.4f, AvgCoeff=%.4f\n", 
                  rateCph, deltaAmbient, newLossCoeff, learnLossCoeff);
  }
}

// --- PRESENCE DETECTION (PING) ---

/**
 * @brief Checks the state of 'awayMode' based on the timeout.
 */
void updateAwayStatus() {
  if (presenceIPs.length() < 5) {
    awayMode = false; // If no IP is set, the function is disabled
    return;
  }
  
  unsigned long now = millis();
  unsigned long timeSinceLastSeen = now - lastPresenceMs;
  unsigned long awayTimeoutMs = (unsigned long)awayTimeoutMins * 60 * 1000UL;

  if (timeSinceLastSeen > awayTimeoutMs) {
    if (!awayMode) {
      Serial.println("🏠 [PRESENCE] Nobody is home. Away Mode ACTIVATED.");
      awayMode = true;
    }
  } else {
    if (awayMode) {
      Serial.println("🏠 [PRESENCE] Someone is home! Away Mode DEACTIVATED.");
      awayMode = false;
    }
  }
}

/**
 * @brief Pings the configured IP addresses.
 * This is called by the main loop every minute.
 */
void checkPresence() {
  // A String (for example "192.168.1.10,192.168.1.11") dismemberment
  int start = 0;
  int end = presenceIPs.indexOf(',');
  String ipStr;

  while (end != -1) {
    ipStr = presenceIPs.substring(start, end);
    start = end + 1;
    end = presenceIPs.indexOf(',', start);
    
    IPAddress ip;
    if (ip.fromString(ipStr)) {
      Serial.printf("Pinging... %s\n", ipStr.c_str());
      
      if (Ping.ping(ip, 1)) { // 1 attempt
        Serial.printf("🏠 [PRESENCE] Success! %s is reachable.\n", ipStr.c_str());
        lastPresenceMs = millis(); // SUCCESS! Updating timestamp
        return; // Someone has returned home, there is no need to check the other IPs
      }
    }
  }
  
  // Checking the last (or only) IP address
  ipStr = presenceIPs.substring(start);
  IPAddress ip;
  if (ip.fromString(ipStr)) {
    Serial.printf("Pinging... %s\n", ipStr.c_str());
    
    if (Ping.ping(ip, 1)) {
      Serial.printf("🏠 [PRESENCE] Success! %s is reachable.\n", ipStr.c_str());
      lastPresenceMs = millis(); // Success!
      return;
    }
  }

  // If you've reached this point, we didn't find anyone on this round.
  // We're NOT doing anything, 'updateAwayStatus' will handle the timeout.
  Serial.println("Ping... Nobody replied in this round.");
}
// --- END: PRESENCE SENSATION ---


// ----------------- GAS (simple daily) -----------------
void updateGasEstimation() {
  unsigned long _t0micros = micros();
  // day rollover by local time (epoch day)
  time_t ep = getEpochNow();
  struct tm *lt = localtime(&ep);
  int day = lt ? lt->tm_yday : -1;
  if (lastGasDay == -1) lastGasDay = day;

  if (day != -1 && day != lastGasDay) {
    // --- NEW DAY ---
    
    // --- Google Sheet ---
    if (G_SCRIPT_ENABLED) {
      Serial.printf("⛽️ [LOG] Saving previous day's gas data (%.2f MJ) to GSheet...\n", gasMJ_today);
      String params = "?gas_mj=" + String(gasMJ_today, 2) + "&gas_cost=" + String(gasCost_today, 2);
      logToGoogleSheet(params);
    }
    
    // Reset counters
    gasMJ_today = 0.0f;
    gasCost_today = 0.0f;
    onMillis_today = 0;
    lastGasDay = day;
  }

  static unsigned long lastUpdate = 0;
  unsigned long now = millis();
  if (lastUpdate == 0) lastUpdate = now;

  // Accumulate ON runtime
  if (heating) {
    onMillis_today += (now - lastUpdate);
  }
  lastUpdate = now;

  // Convert runtime → MJ using boiler power and efficiency
  float hours = onMillis_today / 3600000.0f;
  float thermalMJ = (BOILER_POWER_KW * hours * 3.6f) / (BOILER_EFFICIENCY <= 0.01f ? 1.0f : BOILER_EFFICIENCY);
  gasMJ_today = thermalMJ;
  // Cost from configured Ft/MJ
  gasCost_today = gasMJ_today * gasPricePerMJ;

  cpuBusyMicros += (micros() - _t0micros);
}

// ----------------- CPU LOAD -----------------
void update_cpu_load() {
  if (cpuWindowStartMs == 0) cpuWindowStartMs = millis();
  if (millis() - cpuWindowStartMs >= 1000) {
    unsigned long windowMs = millis() - cpuWindowStartMs;
    float busyMs = cpuBusyMicros / 1000.0f;
    float load = (busyMs / (float)windowMs) * 100.0f;
    if (load < 0) load = 0;
    if (load > 100) load = 100;
    cpuLoadPctCached = (int)(load + 0.5f);
    cpuBusyMicros = 0;
    cpuWindowStartMs = millis();
  }
}