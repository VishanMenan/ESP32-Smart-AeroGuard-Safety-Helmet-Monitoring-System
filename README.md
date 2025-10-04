# ESP32 Smart AeroGuard Safety Helmet Monitoring System (IoT + C++)

An IoT-based safety system built on the ESP32 to monitor helmet conditions in real-time. The project integrates multiple sensors to detect light levels, loud sounds, impacts and falls, with live monitoring available through a web server dashboard.

## 🚀 Features
- **Automatic Light Control**: LDR-based headlamp that switches on/off based on ambient light, along with manual override.
- **Sound Detection**: Microphone sensor triggers vibration feedback when loud noises are detected.
- **Impact & Fall Detection**: LIS3DH accelerometer monitors sudden impacts and falls, with counter.
- **Manual Overrides**: Physical buttons allow light and vibration reset overrides.
- **Web Dashboard**: Real-time monitoring of sensor states and events via ESP32’s built-in web server.

## 🛠️ Technologies Used
- IoT + C++ (Arduino framework)
- ESP32 Microcontroller  
- LIS3DH Accelerometer  
- MAX4466 Microphone sensor  
- LDR
- Web server (ESP32 Wifi)
- HTML, CSS, JavaScript

## ⚙️ Setup Instructions
1. Clone this repository:
```bash
git clone https://github.com/yourusername/ESP32-AeroGuard-Helmet-Monitoring-System.git
```

2. Open the project in Arduino IDE or VS Code + PlatformIO.

3. Connect your ESP32 board and configure the correct COM port.

4. Upload the code and open the Serial Monitor (115200 baud).

5. Connect to the ESP32 WiFi access point to access the web dashboard.

## 📌 Future Improvements
- Long-range connectivity with MQTT/Cloud integration.
- Mobile app for remote monitoring  
- LIS3DH Accelerometer  
- Improved helmet design with proper mounting and protection.  
- Integration of camera for visual defect detection
- Use higher-quality microphone sensor to reduce voltage noise and improve reliability.

---
