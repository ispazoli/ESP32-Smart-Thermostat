#include "globals.h"

// ----------------- SCHEDULE & PRE-HEAT LOGIC -----------------
// This file contains the "brain" of the thermostat,
// formerly in helpers.cpp
// -------------------------------------------------------------

/**
 * @brief Finds the next upcoming schedule change.
 */
ScheduleEvent getNextScheduleEvent() {
  struct tm now_tm;
  if (!getLocalTime(&now_tm)) {
    return {getEpochNow() + 86400, 20.0f, "Error"};
  }
  time_t nowEpoch = mktime(&now_tm);
  int nowMins = now_tm.tm_hour * 60 + now_tm.tm_min;
  bool todayIsWeekend = (now_tm.tm_wday == 0 || now_tm.tm_wday == 6);
  
  // 1. Get all 4 schedule points
  int wdM_m = toMinutes(wdMorningTime);
  int wdE_m = toMinutes(wdEveningTime);
  int weM_m = toMinutes(weMorningTime);
  int weE_m = toMinutes(weEveningTime);

  // 2. Create event lists for today and tomorrow
  struct { int mins; float temp; String name; } todayEvents[2], tomorrowEvents[2];
  
  bool tomorrowIsWeekend = (now_tm.tm_wday == 5 || now_tm.tm_wday == 6); // Fri or Sat -> tomorrow is weekend

  if (todayIsWeekend) {
    todayEvents[0] = {weM_m, weTargetMorning, "Weekend morning"};
    todayEvents[1] = {weE_m, weTargetEvening, "Weekend evening"};
  } else {
    todayEvents[0] = {wdM_m, wdTargetMorning, "Weekday morning"};
    todayEvents[1] = {wdE_m, wdTargetEvening, "Weekday evening"};
  }
  
  if (tomorrowIsWeekend) {
    tomorrowEvents[0] = {weM_m, weTargetMorning, "Weekend morning"};
    tomorrowEvents[1] = {weE_m, weTargetEvening, "Weekend evening"};
  } else {
    tomorrowEvents[0] = {wdM_m, wdTargetMorning, "Weekday morning"};
    tomorrowEvents[1] = {wdE_m, wdTargetEvening, "Weekday evening"};
  }

  // 3. Find the next event today
  ScheduleEvent nextEvent = {0, 0, ""};
  for (int i = 0; i < 2; i++) {
    if (todayEvents[i].mins > nowMins) {
      nextEvent = {
        (nowEpoch - (nowMins * 60) + (todayEvents[i].mins * 60)),
        todayEvents[i].temp,
        todayEvents[i].name
      };
      return nextEvent; // Found it
    }
  }

  // 4. No event found today, so the next event is tomorrow's *first* event
  int firstEventMins = tomorrowEvents[0].mins;
  int minsUntilEvent = (1440 - nowMins) + firstEventMins;
  
  nextEvent = {
    nowEpoch + (minsUntilEvent * 60),
    tomorrowEvents[0].temp,
    tomorrowEvents[0].name
  };
  return nextEvent;
}

/**
 * @brief This is the new "brain". (MODIFIED for Smart-Learn + UI)
 * It returns the REAL target temp and program name,
 * including logic for Manual, Boost, Eco, and Optimal Start.
 */
TargetInfo getActiveProgramInfo() {
  
  // --- NEW: Priority 0 - AWAY MODE ---
  // If presence detection is active (nobody home),
  // override everything and return the frost-protection temp.
  if (awayMode) {
    preHeatActive = false; // No pre-heat during Away Mode
    return {awayTargetTemp, "Away Mode"};
  }
  // --- END: AWAY MODE ---
  
  // --- NEW: Priority 1 - CALENDAR OVERRIDE ---
  // If calendar mode is on AND we have a valid temp (not NAN),
  // return it, overriding manual and auto modes.
  if (calendarMode && !isnan(calendarTarget)) {
    preHeatActive = false; // No pre-heat for calendar events
    return {calendarTarget, "Calendar"};
  }
  // --- END: CALENDAR ---


  // --- Priority 2: Manual, Boost, and Eco Overrides ---
  if (manualMode) {
    preHeatActive = false; // Cannot pre-heat in manual
    float target = manualTarget;
    String name = "Manual";
    if (boostActive && (long)(boostUntil - millis()) > 0) {
      target += 1.0f; // Assumes +1.0C boost
      name = "Boost";
    } else {
      boostActive = false;
    }
    if (ecoEnabled && isNightNow()) {
      target += ecoDelta;
      name += " (Eco)";
    }
    return {target, name};
  }

  // --- Priority 3: Auto Mode: Get current scheduled program ---
  bool we = isWeekendNow();
  struct tm t;
  float currentTarget;
  String currentProgram;

  if (!getLocalTime(&t)) {
    currentTarget = we ? weTargetEvening : wdTargetEvening;
    currentProgram = we ? "Weekend evening" : "Weekday evening";
  } else {
    int nowM = t.tm_hour * 60 + t.tm_min;
    int mM = toMinutes(we ? weMorningTime : wdMorningTime);
    int eM = toMinutes(we ? weEveningTime : wdEveningTime);
    
    if (nowM >= mM && nowM < eM) {
      currentTarget = we ? weTargetMorning : wdTargetMorning;
      // --- FIXED (v6.4.1) ---
      currentProgram = we ? "Weekend morning" : "Weekday morning";
      // --- END FIX ---
    } else {
      currentTarget = we ? weTargetEvening : wdTargetEvening;
      currentProgram = we ? "Weekend evening" : "Weekday evening";
    }
  }

  // --- Priority 4: Auto Mode: Check for Optimal Start (NOW WITH OUTDOOR TEMP) ---
  // Check if:
  // 1. Learning feature is enabled
  // 2. We have a valid indoor temp, outdoor temp, gain rate, and loss coeff
  if (learnMode && !isnan(currentTemp) && !isnan(owTemp) && 
      !isnan(learnGainRate) && !isnan(learnLossCoeff) && learnGainRate > 0.1f) 
  {
    ScheduleEvent nextEvent = getNextScheduleEvent();
    float deltaT = nextEvent.targetTemp - currentTemp;
    
    // Only pre-heat if temp needs to RISE and the next target is HIGHER
    if (deltaT > 0.1f && nextEvent.targetTemp > currentTarget) {
      
      // Calculate expected heat loss at the *current* outdoor temp
      // This is the heat we are constantly fighting against
      float deltaAmbient = currentTemp - owTemp;
      float expectedHeatLoss = 0.0f;
      if (deltaAmbient > 0) {
        expectedHeatLoss = learnLossCoeff * deltaAmbient;
      }

      // The *net* heating power of the boiler is its gain MINUS the loss
      float netHeatGain = learnGainRate - expectedHeatLoss;

      // Safety check: if it's so cold that our loss > gain,
      // just assume a minimum crawl rate (0.1 C/h)
      if (netHeatGain < 0.1f) netHeatGain = 0.1f; 

      // Calculate how long it will take to heat up
      float hoursToHeat = deltaT / netHeatGain;
      unsigned long preHeatSeconds = (unsigned long)(hoursToHeat * 3600.0f);
      
      // Safety cap: don't pre-heat more than 3 hours
      if (preHeatSeconds > 3 * 3600) preHeatSeconds = 3 * 3600; 

      time_t optimalStartTime = nextEvent.epochTime - preHeatSeconds;
      
      // If we are *in* the pre-heat window...
      if (getEpochNow() >= optimalStartTime) {
        // --- THIS IS THE UI FIX ---
        // 1. Set the global flag for the UI
        preHeatActive = true;
        // 2. Return the *target* program, not the "Pre-Heating" text
        return {nextEvent.targetTemp, nextEvent.programName};
      }
    }
  }
  // --- END OPTIMAL START ---

  // --- Priority 5: Auto Mode: Default (no pre-heat) + Eco ---
  preHeatActive = false; // We are not in a pre-heat window
  
  float target = currentTarget;
  String name = currentProgram;
  if (ecoEnabled && isNightNow()) {
    target += ecoDelta; // ecoDelta is negative (e.g., -2.0f)
    name += " (Eco)";
  }
  
  return {target, name};
}