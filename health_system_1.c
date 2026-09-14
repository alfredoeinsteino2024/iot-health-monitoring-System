#define BLYNK_TEMPLATE_ID "TMPL2S_w_i-P6"
#define BLYNK_TEMPLATE_NAME "Health System"
#define BLYNK_AUTH_TOKEN "Qc4vXnXXXXXXXXXXXXXXXU1"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MAX30105.h"
#include <HTTPClient.h>

// -------- TIMERS --------
unsigned long lastDisplay      = 0;
unsigned long lastBlink        = 0;
unsigned long lastTempRead     = 0;
unsigned long lastSmsTime      = 0;
unsigned long lastWifiAttempt  = 0;
unsigned long startupTime      = 0;

// -------- WIFI --------
char ssid[] = "EINSTEIN-ROSEN BRIDGE";
char pass[] = "I LOVE YOU";

// -------- TWILIO --------
const char* twilio_sid   = "AC4a4f4e7d96127c3e1c66b9a035544db8";
const char* twilio_token = "09ad2d03bc5a19d6ddaa3e92aa9df5cb";
const char* from_number  = "+12182316531";
const char* to_number    = "+2349016115583";

// -------- DISPLAY --------
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// -------- PINS --------
#define buzzerPin     12
#define ledPin        13
#define ONE_WIRE_BUS   4

// -------- SENSORS --------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MAX30105 sensor;
bool sensorReady = false;

// -------- HEART RATE VARIABLES --------
#define RATE_SIZE 8                  // Average over 8 beats for stability
float   rates[RATE_SIZE];            // Rolling BPM storage array
byte    rateSpot         = 0;        // Current position in array
float   beatsPerMinute   = 0;        // BPM from single beat
float   bpm              = 0;        // Final smoothed/averaged BPM
long    lastBeat         = 0;
long    prevValue        = 0;
bool    rising           = false;

// 4-point moving average buffer for IR smoothing
long    irBuffer[4]      = {0, 0, 0, 0};
int     bufIndex         = 0;

// -------- OTHER VARIABLES --------
float temperature = 0;
bool  alertState  = false;
bool  alertSent   = false;

// ======================================================
//  SMS FUNCTION
// ======================================================
void sendTwilioSMS(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("SMS FAILED — No WiFi");
    return;
  }

  HTTPClient http;
  String url = "https://api.twilio.com/2010-04-01/Accounts/"
               + String(twilio_sid) + "/Messages.json";

  http.begin(url);
  http.setAuthorization(twilio_sid, twilio_token);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String payload = "To="    + String(to_number)   +
                   "&From=" + String(from_number)  +
                   "&Body=" + message;

  int code = http.POST(payload);
  Serial.print("SMS Status Code: ");
  Serial.println(code);

  if (code == 201) {
    Serial.println("SMS DELIVERED SUCCESSFULLY");
  } else {
    Serial.println("SMS FAILED — check Twilio credentials");
  }

  http.end();
}

// ======================================================
//  SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin,    OUTPUT);
  digitalWrite(ledPin,    LOW);
  digitalWrite(buzzerPin, LOW);

  // ---- OLED ----
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED FAILED — check wiring");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("SYSTEM STARTING...");
  display.display();
  delay(2000);

  // ---- DS18B20 ----
  sensors.begin();
  Serial.println("DS18B20 OK");

  // ---- MAX30105 ----
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 NOT FOUND — check wiring");
  } else {
    sensor.setup();
    sensor.setPulseAmplitudeRed(0x1F);  // Increased red LED for better signal
    sensor.setPulseAmplitudeGreen(0);   // Green LED off — not needed
    sensorReady = true;
    Serial.println("MAX30105 OK");
  }

  // ---- Initialize BPM array to zero ----
  for (byte x = 0; x < RATE_SIZE; x++) rates[x] = 0;

  // ---- BLYNK + WIFI ----
  Blynk.config(BLYNK_AUTH_TOKEN);
  WiFi.begin(ssid, pass);
  lastWifiAttempt = millis();

  // Show connecting message on OLED
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Connecting WiFi...");
  display.display();

  // Wait up to 10 seconds for WiFi
  int wifiWait = 0;
  while (WiFi.status() != WL_CONNECTED && wifiWait < 20) {
    delay(500);
    Serial.print(".");
    wifiWait++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi CONNECTED");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi NOT connected — will retry in loop");
  }

  // ---- STARTUP TIMER ----
  startupTime = millis();

  // ---- READY SCREEN ----
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 15);
  display.println("SYSTEM READY!");
  display.setCursor(5, 35);
  display.println(WiFi.status() == WL_CONNECTED ? "WiFi: CONNECTED" : "WiFi: NO CONNECT");
  display.display();
  delay(2000);

  Serial.println("Setup complete — monitoring started");
}

// ======================================================
//  LOOP
// ======================================================
void loop() {

  // -------- WIFI MANAGEMENT --------
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiAttempt > 10000) {
      lastWifiAttempt = millis();
      Serial.println("Reconnecting WiFi...");
      WiFi.disconnect();
      WiFi.begin(ssid, pass);
    }
  } else {
    if (!Blynk.connected()) {
      Blynk.connect(1000);
    }
    Blynk.run();
  }

  // -------- HEART RATE (Improved) --------
  if (sensorReady) {
    long rawIR = sensor.getIR();

    // Step 1 — 4-point moving average to smooth out noise
    irBuffer[bufIndex] = rawIR;
    bufIndex = (bufIndex + 1) % 4;
    long ir = (irBuffer[0] + irBuffer[1] + irBuffer[2] + irBuffer[3]) / 4;

    if (ir > 50000) { // finger is present

      // Detect rising edge of pulse wave
      if (ir > prevValue) rising = true;

      // Detect falling edge = peak has passed = one heartbeat
      if (ir < prevValue && rising) {
        rising = false;

        long now   = millis();
        long delta = now - lastBeat;
        lastBeat   = now;

        // Only accept realistic beat intervals (30–200 BPM)
        if (delta > 300 && delta < 2000) {
          beatsPerMinute = 60.0 / (delta / 1000.0);

          // Step 2 — Store in rolling 8-beat array
          rates[rateSpot++] = beatsPerMinute;
          rateSpot %= RATE_SIZE;

          // Step 3 — Average all valid stored beats
          float total    = 0;
          byte  validCount = 0;
          for (byte x = 0; x < RATE_SIZE; x++) {
            if (rates[x] > 0) {
              total += rates[x];
              validCount++;
            }
          }

          if (validCount > 0) {
            bpm = total / validCount; // Stable averaged BPM
            Serial.print("BPM: ");
            Serial.println(bpm);
          }
        }
      }

    } else {
      // No finger — reset everything cleanly
      bpm            = 0;
      beatsPerMinute = 0;
      rateSpot       = 0;
      rising         = false;
      for (byte x = 0; x < RATE_SIZE; x++) rates[x] = 0;
    }

    prevValue = ir;
  }

  // -------- TEMPERATURE + BLYNK UPLOAD --------
  if (millis() - lastTempRead > 2000) {
    lastTempRead = millis();

    sensors.requestTemperatures();
    float rawTemp = sensors.getTempCByIndex(0);

    // Only store valid readings
    if (rawTemp != DEVICE_DISCONNECTED_C && rawTemp != -127.0) {
      temperature = rawTemp;
    }

    // Push to Blynk only when valid + connected
    if (WiFi.status() == WL_CONNECTED && Blynk.connected()) {
      if (temperature != 0 && temperature != -127.0) {
        Blynk.virtualWrite(V1, temperature);  // V1 = Temperature
      }
      if (bpm > 0) {
        Blynk.virtualWrite(V2, bpm);          // V2 = Heart Rate
      }
    }
  }

  // -------- ALERT CONDITIONS --------
  bool abnormal = (temperature > 37.5   ||
                   temperature < 35.0   ||
                   bpm > 120            ||
                   (bpm > 0 && bpm < 50));

  // Suppress all alerts during warm-up period
  if (millis() - startupTime < 60000) {
    abnormal = false;
  }

  if (abnormal) {

    // Severe = fast blink + high pitch
    // Mild   = slow blink + low pitch
    int interval = (temperature > 37.5 || bpm > 120) ? 200 : 600;
    int freq     = (temperature > 37.5 || bpm > 120) ? 3000 : 1000;

    if (millis() - lastBlink > interval) {
      lastBlink  = millis();
      alertState = !alertState;
      digitalWrite(ledPin, alertState);
      if (alertState) tone(buzzerPin, freq);
      else            noTone(buzzerPin);
    }

    // First SMS fires INSTANTLY — then every 2 minutes
    if (!alertSent || millis() - lastSmsTime > 120000) {
      lastSmsTime = millis();
      alertSent   = true;

      String condition = "";
      if (temperature > 37.5)       condition += "HIGH FEVER ";
      if (temperature < 35.0)       condition += "LOW TEMP ";
      if (bpm > 120)                condition += "FAST HEART RATE ";
      if (bpm > 0 && bpm < 50)     condition += "SLOW HEART RATE ";

      String msg = "EMERGENCY ALERT!\n"
                   "Condition: "  + condition             + "\n"
                   "Temp: "       + String(temperature, 1) + "C\n"
                   "BPM: "        + String((int)bpm)       + "\n"
                   "Please check the patient NOW!";

      sendTwilioSMS(msg);
      Serial.println("SMS SENT: " + msg);
    }

  } else {
    // All normal — reset everything
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
    alertState = false;
    alertSent  = false; // Reset so next emergency fires immediately
  }

  // -------- OLED DISPLAY --------
  if (millis() - lastDisplay > 500) {
    lastDisplay = millis();

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // --- Header ---
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("-- IOT MONITOR --");

    // --- BPM ---
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print("BPM:");
    display.println(bpm > 0 ? String((int)bpm) : "--");

    // --- Temperature ---
    display.setCursor(0, 42);
    display.print("TMP:");
    if (temperature != 0 && temperature != -127.0) {
      display.print(temperature, 1);
      display.write(247); // degree symbol °
      display.println("C");
    } else {
      display.println("--");
    }

    // --- Status bar at bottom ---
    display.setTextSize(1);
    display.setCursor(0, 56);
    if (millis() - startupTime < 60000) {
      display.println("WARMING UP...");
    } else if (alertState) {
      display.println("!! ALERT !!");
    } else {
      display.println(WiFi.status() == WL_CONNECTED ? "WiFi OK" : "No WiFi");
    }

    display.display();
  }
}
