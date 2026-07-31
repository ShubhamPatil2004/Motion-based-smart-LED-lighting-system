#include <Arduino.h>
#include "BluetoothSerial.h"
#include <Preferences.h>

BluetoothSerial SerialBT;
Preferences prefs;

// ===== PINS =====
#define PIR_PIN     27
#define LED_PIN     23
#define LDR_PIN     32

#define POT_T1      34
#define POT_T2      35
#define POT_LDR     33   // NEW: pot for LDR threshold

// ===== PWM =====
#define PWM_CH      0
#define PWM_FREQ    5000
#define PWM_RES     8

// ===== VARIABLES =====
// BT-set base values (in ms)
long bt_t1_base = 10000;
long bt_t2_base = 10000;
unsigned long t3_time  = 10000;

// Final computed values (base + pot offset)
unsigned long t1_time;
unsigned long t2_time;

int  lux_threshold;
bool useLux;

unsigned long startTime;
bool lightActive = false;

// smoothing filters
float t1_f = 0, t2_f = 0, ldr_f = 0;

// track last pot positions to detect change
int last_t1_raw = -1, last_t2_raw = -1;

// ===== MODE =====
enum Mode { AUTO, MANUAL_ON, MANUAL_OFF };
Mode currentMode;

// ===== HELPERS =====
// Map a 0–4095 pot reading to a ±5000 ms offset
long potOffset(float filtered) {
  // 0 → -5000 ms, 2048 → 0 ms, 4095 → +5000 ms
  return map((long)filtered, 0, 4095, -5000, 5000);
}

// Clamp value to [minV, maxV]
long clamp(long v, long minV, long maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

//-------------------------------------
void loadSettings() {
  prefs.begin("settings", true);
  bt_t1_base     = (long)prefs.getULong("t1_base", 10000);
  bt_t2_base     = (long)prefs.getULong("t2_base", 10000);
  t3_time        = prefs.getULong("t3", 10000);
  lux_threshold  = prefs.getInt("lux", 2000);
  useLux         = prefs.getBool("luxMode", true);
  currentMode    = (Mode)prefs.getInt("mode", 0);
  prefs.end();
}

//-------------------------------------
void saveSettings() {
  prefs.begin("settings", false);
  prefs.putULong("t1_base", (unsigned long)bt_t1_base);
  prefs.putULong("t2_base", (unsigned long)bt_t2_base);
  prefs.putULong("t3", t3_time);
  prefs.putInt("lux", lux_threshold);
  prefs.putBool("luxMode", useLux);
  prefs.putInt("mode", currentMode);
  prefs.end();
}

//-------------------------------------
void sendStatus() {
  SerialBT.println("=== STATUS ===");
  SerialBT.print("Mode: ");
  SerialBT.println(currentMode == AUTO ? "AUTO" : currentMode == MANUAL_ON ? "ON" : "OFF");
  SerialBT.print("LuxMode: ");
  SerialBT.println(useLux ? "ON" : "OFF");
  SerialBT.print("LDR_raw: ");
  SerialBT.print((int)ldr_f);
  SerialBT.print("  Threshold: ");
  SerialBT.println(lux_threshold);
  SerialBT.print("T1_base: ");
  SerialBT.print(bt_t1_base / 1000);
  SerialBT.print("s  T1_final: ");
  SerialBT.print(t1_time / 1000);
  SerialBT.println("s");
  SerialBT.print("T2_base: ");
  SerialBT.print(bt_t2_base / 1000);
  SerialBT.print("s  T2_final: ");
  SerialBT.print(t2_time / 1000);
  SerialBT.println("s");
  SerialBT.print("T3: ");
  SerialBT.print(t3_time / 1000);
  SerialBT.println("s");
  SerialBT.println("==============");
}

//-------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);

  ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, PWM_CH);

  loadSettings();

  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  Serial.println(reason == ESP_SLEEP_WAKEUP_EXT1 ? "Wakeup: PIR" : "Normal Start");

  startTime = millis();
  SerialBT.begin("SmartLight");
  Serial.println("BT Ready. Commands: ON, OFF, AUTO, LUX, NOLUX, T1=<sec>, T2=<sec>, T3=<sec>, STATUS");
}

//-------------------------------------
void loop() {
  unsigned long now = millis();

  // ===== READ SENSORS =====
  bool motion  = digitalRead(PIR_PIN);
  int  ldr_val = analogRead(LDR_PIN);

  // ===== SMOOTH POTS =====
  int t1_raw  = analogRead(POT_T1);
  int t2_raw  = analogRead(POT_T2);
  int ldr_raw = analogRead(POT_LDR);

  t1_f  = 0.9f * t1_f  + 0.1f * t1_raw;
  t2_f  = 0.9f * t2_f  + 0.1f * t2_raw;
  ldr_f = 0.9f * ldr_f + 0.1f * ldr_raw;

  // ===== LDR THRESHOLD from POT (GPIO33) =====
  // Maps 0–4095 pot → 0–4095 threshold (direct / full range)
  lux_threshold = (int)ldr_f;

  // ===== T1 / T2: base (from BT) + pot offset ±5s =====
  long t1_offset = potOffset(t1_f);
  long t2_offset = potOffset(t2_f);

  t1_time = (unsigned long)clamp(bt_t1_base + t1_offset, 1000, 60000);
  t2_time = (unsigned long)clamp(bt_t2_base + t2_offset, 1000, 60000);

  bool isDark = (ldr_val < lux_threshold);

  // ===== BLUETOOTH COMMANDS =====
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    Serial.print("BT CMD: "); Serial.println(cmd);

    if      (cmd == "ON")     { currentMode = MANUAL_ON;  SerialBT.println("Mode: MANUAL ON"); }
    else if (cmd == "OFF")    { currentMode = MANUAL_OFF; SerialBT.println("Mode: MANUAL OFF"); }
    else if (cmd == "AUTO")   { currentMode = AUTO;       SerialBT.println("Mode: AUTO"); }
    else if (cmd == "LUX")    { useLux = true;            SerialBT.println("Lux mode: ON"); }
    else if (cmd == "NOLUX")  { useLux = false;           SerialBT.println("Lux mode: OFF"); }
    else if (cmd == "STATUS") { sendStatus(); }

    else if (cmd.startsWith("T1=")) {
      long v = cmd.substring(3).toInt();
      if (v >= 1 && v <= 60) {
        bt_t1_base = v * 1000;
        SerialBT.print("T1 base set to "); SerialBT.print(v); SerialBT.println("s (pot adjusts ±5s)");
      } else { SerialBT.println("ERR: T1 must be 1–60 sec"); }
    }
    else if (cmd.startsWith("T2=")) {
      long v = cmd.substring(3).toInt();
      if (v >= 1 && v <= 60) {
        bt_t2_base = v * 1000;
        SerialBT.print("T2 base set to "); SerialBT.print(v); SerialBT.println("s (pot adjusts ±5s)");
      } else { SerialBT.println("ERR: T2 must be 1–60 sec"); }
    }
    else if (cmd.startsWith("T3=")) {
      long v = cmd.substring(3).toInt();
      if (v >= 1 && v <= 60) {
        t3_time = v * 1000;
        SerialBT.print("T3 set to "); SerialBT.print(v); SerialBT.println("s");
      } else { SerialBT.println("ERR: T3 must be 1–60 sec"); }
    }
    else {
      SerialBT.println("Unknown cmd. Try: ON/OFF/AUTO/LUX/NOLUX/T1=<s>/T2=<s>/T3=<s>/STATUS");
    }

    saveSettings();
  }

  // ===== MANUAL MODES =====
  if (currentMode == MANUAL_ON)  { ledcWrite(PWM_CH, 255); return; }
  if (currentMode == MANUAL_OFF) { ledcWrite(PWM_CH, 0);   return; }

  // ===== AUTO MODE =====
  if (useLux && !isDark) {
    ledcWrite(PWM_CH, 0);
    lightActive = false;
  } else {
    if (motion) {
      startTime   = now;
      lightActive = true;
    }

    if (lightActive) {
      unsigned long elapsed = now - startTime;

      if      (elapsed < t1_time)                          ledcWrite(PWM_CH, 255);
      else if (elapsed < t1_time + t2_time)                ledcWrite(PWM_CH, 128);
      else if (elapsed < t1_time + t2_time + t3_time)      ledcWrite(PWM_CH, 50);
      else {
        ledcWrite(PWM_CH, 0);
        lightActive = false;
      }
    }
  }

  // ===== DEEP SLEEP =====
  static unsigned long idleStart = 0;
  if (!lightActive && currentMode == AUTO) {
    if (idleStart == 0) idleStart = millis();
    if (millis() - idleStart > 10000) {
      Serial.println("Sleeping...");
      esp_sleep_enable_ext1_wakeup((1ULL << GPIO_NUM_27), ESP_EXT1_WAKEUP_ANY_HIGH);
      delay(50);
      esp_deep_sleep_start();
    }
  } else {
    idleStart = 0;
  }

  // ===== DEBUG (every 1s) =====
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    Serial.printf("Mode:%d Lux:%d LDR:%d Thr:%d Motion:%d T1:%lus(base:%lds) T2:%lus(base:%lds) T3:%lus\n",
      currentMode, useLux, ldr_val, lux_threshold, motion,
      t1_time/1000, bt_t1_base/1000,
      t2_time/1000, bt_t2_base/1000,
      t3_time/1000);
  }
}