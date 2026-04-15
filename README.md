# IoT Smart Circuit Breaker (ESP32)

A 3rd-year B.Tech project focusing on real-time AC monitoring and automated protection. This system uses an ESP32 to monitor Voltage, Current, and Power, providing a digital "trip" mechanism to protect appliances from electrical faults.

## ⚡ Core Logic
The system monitors AC parameters via **ZMPT101B** and **ACS712** sensors. If parameters exceed safe thresholds, the ESP32 triggers a fault state, cutting power through a dual-channel relay and activating a buzzer.

### Trip Thresholds:
* **Overvoltage:** > 250V
* **Undervoltage:** < 180V
* **Overcurrent:** > 5.0A

## 🛠️ Hardware Setup
| Component | Pin (ESP32) | Function |
| :--- | :--- | :--- |
| ZMPT101B | GPIO 35 | Voltage Sensing |
| ACS712 | GPIO 34 | Current Sensing |
| Relay 1 | GPIO 26 | Main Load (Bulb 1) |
| Relay 2 | GPIO 27 | Aux Load (Bulb 2) |
| Buzzer | GPIO 25 | Fault Alarm |
| SSD1306 | I2C (21/22) | Local Status Display |
| Hi-Link | - | 220V AC to 5V DC Power |

## 🌐 Features
* **Web Dashboard:** Built with Tailwind CSS and Lucide icons. Allows remote monitoring and manual relay control.
* **Independent Safety:** Protection logic runs on a 1-second interrupt loop, independent of the WiFi/Web server status to ensure reliability.
* **Auto-Calibration:** 5-second settling period on boot to avoid "ghost trips" from inrush current.
* **OLED Feedback:** Live local telemetry for V, I, and W.

## 🚀 Quick Start
1. **Libraries:** Install `EmonLib`, `Adafruit_SSD1306`, and `Adafruit_GFX`.
2. **WiFi:** Update `ssid` and `password` in the `.ino` file.
3. **Calibration:** - Adjust `emon1.voltage(ZMPT_PIN, 234.26, 1.7)` based on your multimeter reading.
   - Adjust `emon1.current(ACS_PIN, 11.1)` for current accuracy.
4. **Access:** Open the IP address shown in the Serial Monitor (115200 baud) to view the dashboard.

## ⚠️ Safety Note
This project involves high-voltage AC (220V). Ensure all connections are insulated. Use a 500mA fuse between the mains and the Hi-Link module for added protection.

---
**B.Tech IoT Specialization Project | PSIT**
