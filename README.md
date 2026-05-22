# ESP32 Patient Monitor 🏥

A real-time embedded health monitoring system that reads heart rate 
and body temperature, displays live data on an OLED screen, and 
triggers audio/visual alerts for abnormal readings.

> ⚠️ This is a student academic project — not intended or certified 
> for real clinical or medical use.

---

## Demo
> 📸 <img width="810" height="1080" alt="IMG-20260423-WA0008" src="https://github.com/user-attachments/assets/d76dfacb-bd6f-4a9e-aa4b-33a321ce79e9" />
<img width="642" height="430" alt="image" src="https://github.com/user-attachments/assets/c1f619cd-921c-44df-af70-c83e3c576ef6" /><img width="843" height="633" alt="image" src="https://github.com/user-attachments/assets/9526ab58-5342-4cb0-bafd-9bdf1aab67e8" />

*

---

## Features
- 💓 Real-time heart rate monitoring via MAX30105 sensor
- 🌡️ Body temperature reading via DS18B20 sensor
- 📺 Live BPM and temperature display on 0.96" OLED screen
- 🚨 Buzzer + LED alert system for abnormal readings:
  - HIGH temperature (> 37.5°C) → fast beep + fast blink
  - LOW temperature  (< 35.0°C) → slow beep + slow blink
- 🔄 4-point moving average filter for stable heart rate readings
- ⏱️ Non-blocking timing using millis() — no delay() in main loop

---

## Hardware Requirements
| Component | Description |
|---|---|
| ESP32 | Main microcontroller |
| MAX30105 | Pulse oximeter / heart rate sensor |
| DS18B20 | Waterproof temperature sensor |
| SSD1306 OLED | 0.96" I2C display (128x64) |
| Buzzer | Passive buzzer for audio alert |
| LED | Visual alert indicator |
| 4.7kΩ Resistor | Pull-up for DS18B20 data line |
| Breadboard + Jumper Wires | Prototyping |

---

## Wiring / Pin Connections
| Component | Pin | ESP32 GPIO |
|---|---|---|
| OLED SSD1306 | SDA | GPIO 21 |
| OLED SSD1306 | SCL | GPIO 22 |
| MAX30105 | SDA | GPIO 21 |
| MAX30105 | SCL | GPIO 22 |
| DS18B20 | DATA | GPIO 4 |
| Buzzer | Signal | GPIO 12 |
| LED | Anode | GPIO 13 |

> 💡 OLED and MAX30105 share the same I2C bus (SDA/SCL).
> DS18B20 requires a 4.7kΩ pull-up resistor between DATA and 3.3V.

---

## Software / Libraries Required
Install all of these via **Arduino IDE Library Manager**:

| Library | Install Name |
|---|---|
| Adafruit GFX | `Adafruit GFX Library` |
| Adafruit SSD1306 | `Adafruit SSD1306` |
| DallasTemperature | `DallasTemperature` |
| OneWire | `OneWire` |
| SparkFun MAX3010x | `SparkFun MAX3010x Pulse and Proximity Sensor Library` |

---

## How to Run
1. Clone or download this repository
2. Open `patient_monitor.ino` in Arduino IDE
3. Install all required libraries listed above
4. Select your board: **Tools → Board → ESP32 Dev Module**
5. Select the correct COM port
6. Upload the sketch
7. Open Serial Monitor at **115200 baud** to see live readings
8. Place finger gently on MAX30105 sensor to get BPM reading

---

## Alert Thresholds
| Condition | Threshold | Alert Type |
|---|---|---|
| High Temperature | > 37.5°C | Fast beep (3000Hz) + fast LED blink |
| Low Temperature | < 35.0°C | Slow beep (1000Hz) + slow LED blink |
| Normal | 35.0°C – 37.5°C | No alert |

---

## Known Issues / Limitations
- Heart rate readings can be unstable if finger is not properly 
  placed or pressed too hard on the sensor
- BPM alert thresholds are not yet implemented — only temperature 
  triggers alerts in this version
- DS18B20 returns -127°C if disconnected — handled in code but 
  sensor must be connected at startup
- No WiFi or cloud connectivity in this version — fully offline/standalone
- OLED may fail silently if I2C address is wrong — default is 0x3C
- Not tested for extended continuous use (battery/heat not evaluated)

---

## What We Learned
- I2C communication and sharing a bus between multiple devices on ESP32
- Rolling average filtering to reduce noise in heart rate signal
- Non-blocking code design using millis() for reliable multitasking
- Reading and validating sensor data (handling -127°C edge cases)
- OLED rendering using Adafruit GFX and SSD1306 libraries
- Teamwork, version control, and collaborative embedded development

---

## Future Improvements
- [ ] Add WiFi connectivity + Blynk IoT dashboard
- [ ] Add SMS emergency alerts via Twilio API
- [ ] Implement SpO2 (blood oxygen level) reading
- [ ] Add BPM alert thresholds (tachycardia / bradycardia)
- [ ] Improve heart rate algorithm for better accuracy
- [ ] Add SD card logging for data history
- [ ] Design a proper PCB instead of breadboard prototype
- [ ] 3D print an enclosure for the hardware

---

## Project Info
| | |
|---|---|
| **Course** | Mechatronics and Robotics Systems |
| **Institution** | Federal University of Technology Minna |
| **Level** | 300L |
| **Group** | Group 2 |
| **Academic Session** | 2025/2026 |

---

## Team Members
| Role | Name |
|---|---|
| **Group Leader** | Toluwanimi Alfred Fadipe |
| **Member** |  Abbas Abdullahi  |
| **Member** |  Carol Audu bandekaji |

---

## License
This project is open source and available under the 
[MIT License](LICENSE).
