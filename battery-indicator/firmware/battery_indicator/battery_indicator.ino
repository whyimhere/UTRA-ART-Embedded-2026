/*
  UTRA rover - Battery Amount Indicator
  ------------------------------------
  Board:   Arduino Nano (5V). Same code runs on an Uno.
  Sensor:  Adafruit INA228 breakout, I2C address 0x40 (no jumpers)
  Display: 1.3" SH1106 OLED 128x64, I2C address 0x3C

  Wiring (see ../final_diagram_2026-11-15.png):
    battery +            -> INA228 VIN+
    INA228 VIN-          -> rover load (motor drivers + buck converter IN)
    INA228 VBUS          -> buck converter IN (load side of the shunt)
    buck 5V out          -> Nano 5V pin, INA228 VIN, OLED VCC
    SDA (INA228, OLED)   -> Nano A4
    SCL (INA228, OLED)   -> Nano A5
    all GNDs together    (battery -, buck GND, INA228 GND, Nano GND, OLED GND)

  Libraries (Arduino IDE -> Library Manager):
    "Adafruit INA228 Library"  (it also installs "Adafruit BusIO")
    "U8g2"

  How the % works (energy counting + voltage):
    1. At power-on, with the motors idle, read the resting voltage for 2 s
       and look up the starting % in the voltage table.
    2. Starting energy = starting % x battery capacity (Wh).
    3. The INA228 adds up the energy used (Wh). Remaining = start - used.
    4. Safety net: if the voltage says 10% or less, show that instead
       (near empty the voltage drops steeply, so it is reliable there).

  Screen:   MOTOR          26.1V
            70%   [#######   ]
            3.2A    84W    1.2Wh   (Wh = energy used since power-on)
  Serial Monitor (115200 baud) prints one CSV line per second, which you can
  copy into a spreadsheet for the discharge test.
*/

#include <Wire.h>
#include <Adafruit_INA228.h>
#include <U8g2lib.h>

// ======================= SETTINGS =======================
// Which indicator is this?
//   MOTOR pack (2 x 12V in series): keep the line below.
//   HUB battery (1 x 12V):          put // in front of it.
#define MOTOR_INDICATOR

// DEMO: test the OLED without an INA228. Shows fake numbers that drain
// from 100% to ~10% every 2 minutes. Keep the // for the real rover.
// #define DEMO_MODE

const float   SHUNT_OHMS    = 0.015;  // Adafruit INA228 onboard shunt (15 mOhm)
const float   MAX_CURRENT_A = 10.9;   // full scale: 163.84 mV / 15 mOhm = 10.92 A
const uint8_t PERCENT_STEP  = 10;     // round the shown % to this step (1 = exact)
const uint8_t WINDOW_S      = 60;     // voltage % uses the HIGHEST voltage of the last 60 s
const uint8_t START_S       = 2;      // seconds of resting voltage used to set the starting %

// Energy in ONE full 12V battery: 12.8 V x 10 Ah = 128 Wh (rated).
// Replace with the Wh your discharge test measures (full -> BMS cutoff).
const float   CAPACITY_WH_PER_BATTERY = 128.0;

// Voltage -> % for ONE 12V LiFePO4 battery, at rest.
// GENERIC numbers: replace them with your discharge-test results.
// Voltages must go from low to high.
const float   TABLE_V[]   = {10.00, 12.00, 12.80, 12.90, 13.00, 13.05,
                             13.10, 13.20, 13.30, 13.40, 13.60};
const uint8_t TABLE_PCT[] = {    0,    10,    20,    30,    40,    50,
                                60,    70,    80,    90,   100};
// ========================================================

#ifdef MOTOR_INDICATOR
const char    BATTERY_NAME[]      = "MOTOR";
const uint8_t BATTERIES_IN_SERIES = 2;
#else
const char    BATTERY_NAME[]      = "HUB";
const uint8_t BATTERIES_IN_SERIES = 1;
#endif

const uint8_t TABLE_N = sizeof(TABLE_PCT) / sizeof(TABLE_PCT[0]);
const float   CAPACITY_WH = CAPACITY_WH_PER_BATTERY * BATTERIES_IN_SERIES;

Adafruit_INA228 ina;
U8G2_SH1106_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

bool inaOk = false;

// Latest readings
float busV = 0;      // voltage at VBUS (load side of the shunt)
float battV = 0;     // battery voltage = busV + shunt drop
float currentA = 0;  // + means the battery is discharging
float powerW = 0;
float energyWh = 0;  // energy used since counting started
float restV = 0;     // highest battV in the last WINDOW_S seconds
int   voltPct = -1;  // % from the voltage table
int   countPct = -1; // % from energy counting
int   percent = -1;  // % shown on screen (-1 = not ready yet)

bool  counting = false;  // starting point set?
float startWh = 0;       // energy in the battery when counting started
unsigned long inaStartMs = 0;

// Rolling maximum: one slot per second, stored in millivolts.
// Motor current makes the voltage sag; the highest recent reading is
// closest to the true "resting" voltage, so % is based on that.
uint16_t slotMaxMv[WINDOW_S];
uint8_t  slot = 0;

unsigned long lastReadMs = 0, lastSlotMs = 0, lastLogMs = 0, lastRetryMs = 0;

// ---------- INA228 ----------
bool startIna() {
#ifdef DEMO_MODE
  inaStartMs = millis();
  return true;                                 // no sensor needed in demo
#endif
  if (!ina.begin()) return false;              // address 0x40, resets the chip
  ina.setShunt(SHUNT_OHMS, MAX_CURRENT_A);
  ina.setADCRange(0);                          // +/-163.84 mV range (up to ~10.9 A)
  ina.setAveragingCount(INA228_COUNT_64);      // average 64 samples...
  ina.setVoltageConversionTime(INA228_TIME_1052_us);
  ina.setCurrentConversionTime(INA228_TIME_1052_us);  // ...about 5 new readings/s
  inaStartMs = millis();
  return true;
}

// Work out the % shown on screen.
void updatePercent(unsigned long now) {
  voltPct = (restV < 5.0) ? -1 : percentFromVoltage(restV);

  if (!counting) {
    // First START_S seconds: battery should be at rest, use voltage only.
    if (now - inaStartMs >= START_S * 1000UL && voltPct >= 0) {
      startWh = voltPct / 100.0 * CAPACITY_WH;
#ifndef DEMO_MODE
      ina.resetAccumulators();                 // energy used = 0 from here
#endif
      energyWh = 0;
      counting = true;
    } else {
      percent = -1;                            // shows "--%"
      return;
    }
  }

  float leftWh = startWh - energyWh;
  countPct = constrain((int)(leftWh / CAPACITY_WH * 100.0 + 0.5), 0, 100);

  int p = countPct;
  if (voltPct >= 0 && voltPct <= 10 && voltPct < p) p = voltPct;  // near-empty safety net
  percent = roundToStep(p);
}

void readIna() {
#ifdef DEMO_MODE
  // Fake battery: 13.6 V -> 12.0 V per battery over 2 minutes, 3.2 A load.
  float frac = fmod((millis() - inaStartMs) / 1000.0, 120.0) / 120.0;
  battV    = (13.6 - 1.6 * frac) * BATTERIES_IN_SERIES;
  busV     = battV;
  currentA = 3.2;
  powerW   = battV * currentA;
  energyWh = frac * 0.9 * CAPACITY_WH;
  return;
#endif
  busV     = ina.getBusVoltage_V();
  currentA = ina.getCurrent_mA() / 1000.0;
  powerW   = ina.getPower_mW() / 1000.0;
  energyWh = ina.readEnergy() / 3600.0;        // joules -> watt-hours
  battV    = busV + currentA * SHUNT_OHMS;     // add back the drop across the shunt
}

// ---------- voltage -> percent ----------
int percentFromVoltage(float packV) {
  float v = packV / BATTERIES_IN_SERIES;       // volts per 12V battery
  if (v <= TABLE_V[0]) return 0;
  if (v >= TABLE_V[TABLE_N - 1]) return 100;
  for (uint8_t i = 1; i < TABLE_N; i++) {
    if (v <= TABLE_V[i]) {
      float f = (v - TABLE_V[i - 1]) / (TABLE_V[i] - TABLE_V[i - 1]);
      float p = TABLE_PCT[i - 1] + f * (TABLE_PCT[i] - TABLE_PCT[i - 1]);
      return (int)(p + 0.5);
    }
  }
  return 100;
}

int roundToStep(int p) {
  int r = ((p + PERCENT_STEP / 2) / PERCENT_STEP) * PERCENT_STEP;
  return r > 100 ? 100 : r;
}

// ---------- rolling maximum ----------
void updateRollingMax(unsigned long now) {
  if (now - lastSlotMs >= 1000) {              // start a new 1-second slot
    lastSlotMs += 1000;
    slot = (slot + 1) % WINDOW_S;
    slotMaxMv[slot] = 0;
  }
  uint16_t mv = (uint16_t)(battV * 1000.0 + 0.5);
  if (mv > slotMaxMv[slot]) slotMaxMv[slot] = mv;

  uint16_t best = 0;
  for (uint8_t i = 0; i < WINDOW_S; i++)
    if (slotMaxMv[i] > best) best = slotMaxMv[i];
  restV = best / 1000.0;
}

// ---------- display ----------
void drawRight(int y, const char *s) {         // right-aligned text
  oled.drawStr(128 - oled.getStrWidth(s), y, s);
}

void drawScreen() {
  char buf[12], num[8];

  oled.firstPage();
  do {
    if (!inaOk) {
      oled.setFont(u8g2_font_7x13B_tr);
      oled.drawStr(0, 12, BATTERY_NAME);
      oled.drawStr(0, 30, "INA228 not found");
      oled.setFont(u8g2_font_6x10_tr);
      oled.drawStr(0, 46, "check SDA/SCL, VIN,");
      oled.drawStr(0, 58, "GND (address 0x40)");
    } else {
      // Top row: name + battery voltage
      oled.setFont(u8g2_font_7x13B_tr);
      oled.drawStr(0, 12, BATTERY_NAME);
      dtostrf(battV, 4, 1, num);
      snprintf(buf, sizeof(buf), "%sV", num);
      drawRight(12, buf);
      oled.drawHLine(0, 15, 128);

      // Big percent
      oled.setFont(u8g2_font_logisoso24_tr);
      if (percent < 0) snprintf(buf, sizeof(buf), "--%%");
      else             snprintf(buf, sizeof(buf), "%d%%", percent);
      oled.drawStr(0, 46, buf);

      // Battery icon on the right
      oled.drawFrame(76, 22, 46, 22);
      oled.drawBox(122, 28, 4, 10);
      if (percent > 0) oled.drawBox(78, 24, (uint8_t)(42L * percent / 100), 18);

      // Bottom row: current, power, energy used
      oled.setFont(u8g2_font_6x10_tr);
      dtostrf(currentA, 1, 1, num);
      snprintf(buf, sizeof(buf), "%sA", num);
      oled.drawStr(0, 63, buf);
      dtostrf(powerW, 1, 0, num);
      snprintf(buf, sizeof(buf), "%sW", num);
      oled.drawStr(64 - oled.getStrWidth(buf) / 2, 63, buf);
      dtostrf(energyWh, 1, 1, num);
      snprintf(buf, sizeof(buf), "%sWh", num);
      drawRight(63, buf);
    }
  } while (oled.nextPage());
}

// ---------- serial log ----------
void logCsv(unsigned long now) {
  Serial.print(now / 1000);     Serial.print(',');
  Serial.print(busV, 3);        Serial.print(',');
  Serial.print(battV, 3);       Serial.print(',');
  Serial.print(restV, 3);       Serial.print(',');
  Serial.print(currentA, 3);    Serial.print(',');
  Serial.print(powerW, 2);      Serial.print(',');
  Serial.print(energyWh, 3);    Serial.print(',');
  Serial.print(voltPct);        Serial.print(',');
  Serial.print(countPct);       Serial.print(',');
  Serial.println(percent);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setWireTimeout(25000, true);  // a loose I2C wire can't freeze the Arduino
  oled.begin();

  inaOk = startIna();
  Serial.print(F("UTRA battery indicator: "));
  Serial.println(BATTERY_NAME);
  if (!inaOk) Serial.println(F("INA228 not found at 0x40 - check wiring"));
  Serial.println(F("t_s,bus_V,batt_V,rest_V,current_A,power_W,used_Wh,volt_pct,count_pct,shown_pct"));

  lastSlotMs = millis();
  drawScreen();
}

void loop() {
  unsigned long now = millis();

  if (!inaOk) {                                // keep retrying every 2 s
    if (now - lastRetryMs >= 2000) {
      lastRetryMs = now;
      inaOk = startIna();
      if (inaOk) Serial.println(F("INA228 found"));
      drawScreen();
    }
    return;
  }

  if (now - lastReadMs >= 250) {               // 4 updates per second
    lastReadMs = now;
    readIna();
    updateRollingMax(now);
    updatePercent(now);
    drawScreen();
  }

  if (now - lastLogMs >= 1000) {
    lastLogMs = now;
    logCsv(now);
  }
}
