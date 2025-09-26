#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

// ====== WiFi Credentials ======
const char *WIFI_SSID = "";  //Write your WIFI SSID here
const char *WIFI_PASS = "";  //Write your WIFI Password here

// -- Pin Definitions --
#define LDR_PIN 34
#define LIGHTS_PIN 27
#define LIGHTBTN_PIN 25
#define MIC_PIN 35
#define VIBRATION_PIN 12
#define RESETBTN_PIN 33

// -- Light Control Variables --
int lightOnThreshold = 3500;
int lightOffThreshold = 2000;
bool lampState = false;

// -- Light override variables --
static bool overrideMode = false;
static bool overrideState = false;
static bool lastLightButtonState = HIGH;

// -- Noise Detection Variables --
const int noiseThreshold = 2046;
bool vibrationOn = false;
unsigned long lastLightChange = 0;
const unsigned long maskDuration = 300;

// -- Impact Detection Variables (improved) --
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
float lastX = 0, lastY = 0, lastZ = 0;
const float impactThresholdG = 1.5;  //Initially 6.0 for 8/16g range
const unsigned long impactHoldMs = 5000;
const unsigned long countDebounceMs = 300;

volatile int impactCount = 0;
bool impactDetected = false;
unsigned long impactEndTime = 0;
unsigned long lastCountTime = 0;
bool lastImpactRaw = false;

const bool IMPACT_DEBUG = false;  //CHECK THIS LATER
bool lisAvailable = false;

const int LDR_SAMPLES = 20;  //Initially 10
int readLDR() {
  long sum = 0;
  for (int i = 0; i < LDR_SAMPLES; i++) {
    sum += analogRead(LDR_PIN);
    delay(2);
  }
  return sum / LDR_SAMPLES;
}

bool initLis() {
  for (int i = 0; i < 5; i++) {
    //if (lis.begin(0x19) || lis.begin(0x18)) {
      if (lis.begin(0x19) || lis.begin(0x18)) {
      lis.setRange(LIS3DH_RANGE_4_G);
      Serial.println("LIS3DH initialized");
      return true;
    }
    Serial.println("Retrying LIS3DH...");
    delay(500);
  }
  return false;
}

// ====== Web Server ======
WebServer server(80);

// --- JSON API for AJAX ---
void handleData() {
  int ldrValue = readLDR(); // smoothed LDR
  int micValue = analogRead(MIC_PIN);
  int deviation = abs(micValue - 2048);

  String json = "{";
  json += "\"ldrValue\":" + String(ldrValue) + ",";
  json += "\"lampState\":" + String(lampState ? 1 : 0) + ",";
  json += "\"overrideMode\":" + String(overrideMode ? 1 : 0) + ",";
  json += "\"overrideState\":" + String(overrideState ? 1 : 0) + ",";
  json += "\"micValue\":" + String(micValue) + ",";
  json += "\"deviation\":" + String(deviation) + ",";
  json += "\"vibrationOn\":" + String(vibrationOn ? 1 : 0) + ",";
  json += "\"impactDetected\":" + String(impactDetected ? 1 : 0) + ",";
  json += "\"impactCount\":" + String(impactCount);
  json += "}";
  
  server.send(200, "application/json", json);
}

// --- HTML Dashboard ---
void handleRoot()
{
  String page = R"rawliteral(
  <!doctype html>
  <html lang="en">
  <head>
    <meta charset="utf-8">
    <title>AeroGuard Helmet Monitoring Dashboard</title>
    <style>
      body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background-color: #dfdfdfff;
        margin: 0;
        padding: 0px;
      }
      header {
        background-color: #2f4f4f;
        color: #fff;
        padding: 15px;
        text-align: center;
        margin-bottom: 20px;
      }
      h1 { text-align: center; color: #d9ddddff; }
      .dashboard {
        display: flex;
        gap: 20px;
        justify-content: center;
        flex-wrap: wrap;
      }
      .helmet-card {
        background-color: #ffffff;
        border-radius: 15px;
        padding-left: 20px;
        padding-right: 20px;
        padding-bottom: 20px;
        margin: 10px;
        width: 220px;
        box-shadow: 0 4px 8px rgba(0,0,0,0.2);
      }
      .helmet-card h2 {
        text-align: center;
        font-size: 1.5em;
        margin-top: 20px;
        margin-bottom: 20px;
        color: #004d40;
      }
      .sensor-box {
        border: 2px solid #004d40;
        border-radius: 10px;
        padding: 10px;
        margin-bottom: 12px;
        text-align: center;
        background-color: #b2ebf2;
        font-weight: bold;
      }

      .ok { border-color: green; background-color: #abeeadff; }

      .warn { border-color: red; border-width: 4px; background-color: #ffcdd2; }

      .red-text {
      color: red;
      font-weight: bold;
      }

      .impact-row {
        display: flex;
        gap: 10px;
      }

      .impact-box {
        flex: 1;
        border: 2px solid #004d40;
        border-radius: 10px;
        padding: 8px;
        text-align: center;
        background-color: #b2ebf2;
        font-weight: bold;
      }
      button {
        padding: 6px 10px;
        margin-top: 5px;
        border-radius: 6px;
        border: none;
        background-color: #004d40;
        color: white;
        cursor: pointer;
      }
      button:hover { background-color: #019777ff; }
    </style>
  </head>
  <body>
    <header>
      <h1>Safety Helmet Monitoring Dashboard</h1>
      <p>IP: )rawliteral" +
                WiFi.localIP().toString() + R"rawliteral(</p>
    </header>
      <div class="dashboard">
        <!-- Helmet 1 -->
        <div class="helmet-card">
          <h2>Helmet 1</h2>
          <div id="lightBox" class="sensor-box">
            Safety Lights:<br>
            <span id="lightStatus">-</span>
          </div>
          <div id="soundBox" class="sensor-box">
            Surrounding Sound:<br>
            <span id="soundStatus">-</span>
          </div>
          <div class="impact-row">
            <div id="impactBox" class="impact-box">
              Impact/Fall:<br>
              <span id="impactStatus">-</span>
            </div>
            <div id="impactCountBox" class="impact-box">
              Impact/Fall Count:<br>
              <span id="impactCount">0</span><br>
              <button onclick="resetImpact()">Reset</button>
            </div>
          </div>
        </div>

        <!-- Helmet 2 (Dummy) -->
        <div class="helmet-card">
          <h2>Helmet 2</h2>
          <div class="sensor-box">
            Safety Lights:<br>
            <span>ON</span>
          </div>
          <div class="sensor-box">
            Surrounding Sound:<br>
            <span>Normal</span>
          </div>
          <div class="impact-row">
            <div class="impact-box">
              Impact/Fall:<br>
              <span>No Impact/Fall Detected</span>
            </div>
            <div class="impact-box">
              Impact/Fall Count:<br>
              <span class="red-text">0</span><br>
              <button>Reset</button>
            </div>
          </div>
        </div>
      </div>

    <script>
    async function fetchData() {
      let res = await fetch("/data");
      let d = await res.json();

      // Safety light
      let lightBox = document.getElementById("lightBox");
      let lightStatus = document.getElementById("lightStatus");
      let lightText = d.lampState ? "ON" : "OFF";
      lightStatus.innerText = lightText;
      lightBox.className = "sensor-box " + (d.lampState ? "warn" : "ok");

      // Surrounding sound
      let soundBox = document.getElementById("soundBox");
      let soundStatus = document.getElementById("soundStatus");
      let soundText = d.vibrationOn ? "Loud" : "Normal";
      soundStatus.innerText = soundText;
      soundBox.className = "sensor-box " + (d.vibrationOn ? "warn" : "ok");

      // Impact
      let impactStatus = document.getElementById("impactStatus");
      if (d.impactDetected) {
        impactStatus.innerHTML = "<span class='red-text'>Impact/Fall Detected!</span>";
      } else {
        impactStatus.innerText = "No Impact/Fall Detected";
      }

      // Impact count
      document.getElementById("impactCount").innerHTML = "<span class='red-text'>" + d.impactCount + "</span>";
    }

    async function resetImpact() {
      await fetch("/resetImpact");
      document.getElementById("impactCount").innerHTML = "<span class='red-text'>0</span>";
    }

    setInterval(fetchData, 500);
    fetchData();
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

// ====== Setup ======
void setup()
{
  pinMode(LIGHTS_PIN, OUTPUT);
  digitalWrite(LIGHTS_PIN, LOW);  
  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW);  
  pinMode(LIGHTBTN_PIN, INPUT_PULLUP);
  pinMode(RESETBTN_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Web routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/favicon.ico", []()
            { server.send(204); }); // avoid "handler not found" for favicon
  server.on("/resetImpact", []()           {
  impactCount = 0;
  server.send(200, "text/plain", "Impact count reset"); });
  server.begin();
  Serial.println("HTTP server started");

  // --- Sensor initialization ---
  lisAvailable = initLis();
  if (!initLis()) {
    Serial.println("LIS3DH not found after retries. Continuing without it.");
  }

  // Initialize lastX/Y/Z
  sensors_event_t ev;
  lis.getEvent(&ev);
  lastX = ev.acceleration.x;
  lastY = ev.acceleration.y;
  lastZ = ev.acceleration.z;
}

// ====== Main Loop ======
void loop()
{
  // -- Mic + Vibration --
  int micValue = analogRead(MIC_PIN);
  int deviation = abs(micValue - 2048);
  if (deviation > noiseThreshold) {
    vibrationOn = true;
    Serial.println("Loud sound detected → Vibration ON");
  }

  // Reset vibration only when RESET button is pressed
  static unsigned long lastResetPress = 0;
  if (digitalRead(RESETBTN_PIN) == LOW && millis() - lastResetPress > 500) {
    vibrationOn = false;
    lastResetPress = millis();
    Serial.println("Switch pressed → Vibration OFF");
  }

  // Apply vibration control
  digitalWrite(VIBRATION_PIN, vibrationOn ? HIGH : LOW);

  // Read LDR with averaging
  int ldrValue = readLDR();
  bool isDark = (ldrValue > lightOnThreshold);
  bool isBright = (ldrValue < lightOffThreshold);

  // Only update lampState if NOT in override
  if (!overrideMode) {
    if (!lampState && isDark) {
      lampState = true;
      Serial.println("Light On (LDR)");
    } else if (lampState && isBright) {
      lampState = false;
      Serial.println("Light Off (LDR)");
    }
  }

  // Handle override button
  bool buttonState = digitalRead(LIGHTBTN_PIN);
  if (buttonState == LOW && lastLightButtonState == HIGH) {
    overrideMode = true;
    overrideState = !overrideState;
    Serial.print("Override toggled → ");
    Serial.println(overrideState ? "ON" : "OFF");
  }
  lastLightButtonState = buttonState;

  // Apply light control
  if (overrideMode) {
    digitalWrite(LIGHTS_PIN, overrideState ? HIGH : LOW);
  } else {
    digitalWrite(LIGHTS_PIN, lampState ? HIGH : LOW);
  }

  // If lights OFF by override but later it gets dark → allow LDR to switch back
  if (!overrideState && isDark) {
    overrideMode = false;
  }

  // -- Impact Detection --
  if (lisAvailable) {
    sensors_event_t event;
    if (lis.getEvent(&event)) {
      float ax = event.acceleration.x;
      float ay = event.acceleration.y;
      float az = event.acceleration.z;

      float dx = ax - lastX;
      float dy = ay - lastY;
      float dz = az - lastZ;

      lastX = ax;
      lastY = ay;
      lastZ = az;

      float deltaMag = sqrt(dx * dx + dy * dy + dz * dz);
      float deltaG = deltaMag / 9.80665f;

      //Serial.print("Accel X: ");
      //Serial.print(ax);
      //Serial.print(" Y: ");
      //erial.print(ay);
      //Serial.print(" Z: ");
      //Serial.print(az);
      //Serial.print(" | deltaG: ");
      //Serial.println(deltaG, 3);

      bool impactRaw = (deltaG >= impactThresholdG);

      if (impactRaw) {
        impactEndTime = millis() + impactHoldMs;
        impactDetected = true;
        Serial.print("Impact detected!");
      }

      if (impactRaw && !lastImpactRaw) {
        if (millis() - lastCountTime > countDebounceMs) {
          impactCount++;
          lastCountTime = millis();
          Serial.print("Impact detected! Count = ");
          Serial.println(impactCount);
        }
      }

      if (impactDetected && millis() > impactEndTime) {
        impactDetected = false;
      }
      lastImpactRaw = impactRaw;
    } else {
      Serial.println("LIS3DH read failed, retrying...");
    }
  }

  // Handle web requests
  server.handleClient();

  delay(1000);
}
