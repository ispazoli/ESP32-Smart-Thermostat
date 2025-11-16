#include "globals.h"

// ----------------- TIME UTILITY HELPERS -----------------
// This file contains all general-purpose time functions
// formerly in helpers.cpp
// --------------------------------------------------------

/**
 * @brief Gets the current epoch time.
 * @return time_t current epoch timestamp.
 */
time_t getEpochNow() {
  time_t now;
  time(&now);
  return now;
}

/**
 * @brief Gets the current time as a "HH:MM" string.
 * @return String formatted as "HH:MM".
 */
String getTimeString() {
  struct tm t;
  if (!getLocalTime(&t)) return "--:--";
  char b[6];
  snprintf(b, sizeof(b), "%02d:%02d", t.tm_hour, t.tm_min);
  return String(b);
}

/**
 * @brief Checks if the current day is a weekend (Sat/Sun).
 * @return true if it's Saturday or Sunday.
 */
bool isWeekendNow() {
  struct tm t;
  if (!getLocalTime(&t)) return false;
  // 0=Sunday .. 6=Saturday
  return (t.tm_wday == 0 || t.tm_wday == 6);
}

/**
 * @brief Checks if the current time is "Night" (for Eco mode).
 * @return true if time is between 22:00 and 06:00.
 */
bool isNightNow() {
  struct tm t;
  if (!getLocalTime(&t)) return false;
  int h = t.tm_hour;
  return (h >= 22 || h < 6); // 22:00—06:00
}

/**
 * @brief Converts a "HH:MM" string to total minutes from midnight.
 * @param hhmm String formatted as "HH:MM".
 * @return int total minutes.
 */
int toMinutes(const String& hhmm) {
  if (hhmm.length() < 5) return 0;
  int H = hhmm.substring(0, 2).toInt(), M = hhmm.substring(3, 5).toInt();
  return H * 60 + M;
}