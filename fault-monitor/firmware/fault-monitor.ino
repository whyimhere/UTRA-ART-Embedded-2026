#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ONE_WIRE_BUS 4   // DS18B20 data pin (4.7 kΩ pull-up to 3.3V)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// INA219 on Wire (SDA=21, SCL=22), OLED on Wire1 (SDA=19, SCL=18)
Adafruit_INA219 ina219;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

bool inaOK = false;
bool oledOK = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);    // INA219 bus
  Wire1.begin(19, 18);   // OLED bus

  // Temperature sensor
  sensors.begin();
  Serial.print("DS18B20 sensors found: ");
  Serial.println(sensors.getDeviceCount());

  // INA219
  if (ina219.begin(&Wire)) {
    inaOK = true;
    Serial.println("INA219 ready");
  } else {
    Serial.println("INA219 not found, check wiring");
  }

  // OLED
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledOK = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
  } else {
    Serial.println("OLED not found, check wiring");
  }
}

void loop() {
  // Temperature
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // Voltage / current / power
  float busV = 0, shunt_mV = 0, current_mA = 0, power_mW = 0;
  if (inaOK) {
    busV       = ina219.getBusVoltage_V();
    shunt_mV   = ina219.getShuntVoltage_mV();
    current_mA = ina219.getCurrent_mA();
    power_mW   = ina219.getPower_mW();
  }

  // Serial output
  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.print("Temp: No sensor");
  } else {
    Serial.printf("Temp: %.1f C", tempC);
  }

  if (inaOK) {
    Serial.printf(" | Bus: %.2f V | Shunt: %.2f mV | I: %.1f mA | P: %.1f mW\n",
                  busV, shunt_mV, current_mA, power_mW);
  } else {
    Serial.println(" | INA219: not found");
  }

  // OLED output: two lines
  if (oledOK) {
    display.clearDisplay();
    display.setTextSize(2);

    display.setCursor(0, 8);
    if (inaOK) display.printf("I:%.1fmA", current_mA);
    else       display.print("I: ---");

    display.setCursor(0, 38);
    if (inaOK) display.printf("V:%.2f V", busV);
    else       display.print("V: ---");

    display.display();
  }

  delay(1000);
}
