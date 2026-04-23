#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MAX30105.h"
#include "heartRate.h"

// -------- DISPLAY --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------- PINS --------
#define buzzerPin 12
#define ledPin 13
#define ONE_WIRE_BUS 4 

// -------- SENSORS --------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MAX30105 particleSensor;

// -------- HEART RATE --------
long lastBeat = 0;
float beatsPerMinute;
int beatAvg = 0;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;

// -------- TIMING --------
unsigned long lastBlink = 0;
bool alertState = false;

// -------- STATE TRACKING --------
int lastState = 0; // 0=normal, 1=high temp, 2=low temp

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  sensors.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  particleSensor.begin(Wire, I2C_SPEED_FAST);
  particleSensor.setup();

  // IMPORTANT: stronger LED for better detection
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);
}

void loop() {

  // -------- READ HEART SENSOR --------
  long irValue = particleSensor.getIR();

  // NO FINGER DETECTION
  if (irValue < 10000) {
    beatAvg = 0;
  }

  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute > 30 && beatsPerMinute < 200) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte i = 0; i < RATE_SIZE; i++) {
        beatAvg += rates[i];
      }
      beatAvg /= RATE_SIZE;
    }
  }

  // -------- READ TEMPERATURE --------
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // -------- DISPLAY --------
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("PATIENT MONITOR");

  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print("BPM:");
  display.println(beatAvg > 0 ? beatAvg : 0);

  display.setCursor(0, 42);
  display.print("TMP:");
  display.print(temperature, 1);
  display.println("C");

  display.display();

  // -------- STATE DETECTION --------
  int currentState;

  if (temperature > 37.5) currentState = 1;
  else if (temperature < 35.0) currentState = 2;
  else currentState = 0;

  // reset timing when state changes
  if (currentState != lastState) {
    lastBlink = millis();
    lastState = currentState;
  }

  // -------- ALERT SYSTEM --------
  if (currentState == 1) {

    // HIGH TEMP → FAST, HIGH-PITCH BEEP
    if (millis() - lastBlink > 200) {
      lastBlink = millis();
      alertState = !alertState;

      digitalWrite(ledPin, alertState);

      if (alertState) tone(buzzerPin, 3000);
      else noTone(buzzerPin);
    }

  } 
  else if (currentState == 2) {

    // LOW TEMP → SLOW, LOW-PITCH BEEP
    if (millis() - lastBlink > 600) {
      lastBlink = millis();
      alertState = !alertState;

      digitalWrite(ledPin, alertState);

      if (alertState) tone(buzzerPin, 1000);
      else noTone(buzzerPin);
    }

  } 
  else {
    // NORMAL
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
  }
}
