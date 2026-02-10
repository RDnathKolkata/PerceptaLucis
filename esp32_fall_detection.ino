/*

Percepta™ Lucis
© 2026 Rajdeep Debnath
CC BY-NC-SA 4.0

  ESP32 Fall Detection - SIMPLE VERSION
  
 * Hardware:
  - ESP32 DevKit
  - MPU6050 (I2C: SDA=GPIO21, SCL=GPIO22)
  - Joystick Module (VRx=GPIO34, VRy=GPIO35, SW=GPIO32)
  
 * What it does:
  - Detects falls with MPU6050
  - Sends alert to laptop (triggers audio on ESP-2)
  - Joystick button = CANCEL alert
  - Joystick directions = Control ultrasonic range (ESP-4)
  - Joystick button press (no fall) = Request time announcement
  
  NO SIM800L
 */

#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <HTTPClient.h>

/* USER config */

// WiFi creds
const char* ssid = "~";
const char* password = "~";

// Server endpoints
const char* laptopAlertUrl = "http://192.168.1.100:8000/fall_alert";
const char* audioAlertUrl = "http://192.168.1.101/alert";

// Pin definitions
const int VRxPin = 34;        // Joystick X-axis
const int VRyPin = 35;        // Joystick Y-axis  
const int buttonPin = 32;     // Joystick button (CANCEL / TIME)

// UART to ESP-4 (Ultrasonic)
const int ULTRASONIC_TX = 26;
const int ULTRASONIC_RX = 27;

// Fall detection parameters
const float FALL_THRESHOLD = 2.5;
const float FREEFALL_THRESHOLD = 0.5;

// Joystick parameters
const int ADC_CENTER = 2048;
const int DEADZONE = 200;
const int UPPER_THRESHOLD = ADC_CENTER + DEADZONE;
const int LOWER_THRESHOLD = ADC_CENTER - DEADZONE;

// Time request cooldown
unsigned long lastTimeRequest = 0;
const unsigned long TIME_REQUEST_COOLDOWN = 2000;

/* ============================================
   GLOBAL VARIABLES
   ============================================ */

MPU6050 mpu;
HardwareSerial ultrasonicSerial(2);

bool fallDetected = false;
unsigned long lastFallCheck = 0;

/* ============================================
   FALL DETECTION
   ============================================ */

bool checkForFall() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  float AccX = ax / 16384.0;
  float AccY = ay / 16384.0;
  float AccZ = az / 16384.0;
  
  float AccMagnitude = sqrt(AccX * AccX + AccY * AccY + AccZ * AccZ);
  
  // Print every second
  if (millis() - lastFallCheck > 1000) {
    Serial.printf("📊 Accel: %.2fg | Fall: %s\n", 
                  AccMagnitude, 
                  fallDetected ? "YES" : "NO");
    lastFallCheck = millis();
  }
  
  // Detect fall
  if (AccMagnitude > FALL_THRESHOLD || AccMagnitude < FREEFALL_THRESHOLD) {
    Serial.printf("⚠️ Abnormal acceleration: %.2fg\n", AccMagnitude);
    return true;
  }
  
  return false;
}

/* JOYSTICK FUNCTIONS */

bool checkCancelButton() {
  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    delay(50);
    if (digitalRead(buttonPin) == LOW) {
      Serial.println("🔘 Button pressed!");
      return true;
    }
  }
  return false;
}

void requestTimeAnnouncement() {
  Serial.println("🕐 Time requested!");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi not connected");
    return;
  }
  
  HTTPClient http;
  http.begin(audioAlertUrl);
  http.setTimeout(3000);
  http.addHeader("Content-Type", "application/json");
  
  // Calculate uptime
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  minutes = minutes % 60;
  seconds = seconds % 60;
  
  String payload = "{\"object\":\"time\",\"distance\":0,\"type\":\"announce_time\",";
  payload += "\"hours\":" + String(hours % 24) + ",";
  payload += "\"minutes\":" + String(minutes) + ",";
  payload += "\"seconds\":" + String(seconds) + "}";
  
  int responseCode = http.POST(payload);
  if (responseCode > 0) {
    Serial.printf("🔊 Time: %02lu:%02lu:%02lu\n", hours % 24, minutes, seconds);
  }
  
  http.end();
}

void handleJoystickCommands() {
  int xVal = analogRead(VRxPin);
  int yVal = analogRead(VRyPin);
  int buttonState = digitalRead(buttonPin);
  
  static unsigned long lastCommand = 0;
  
  // Direction commands (debounced)
  if (millis() - lastCommand >= 500) {
    
    // ⬆️ UP: Increase ultrasonic range
    // if (yVal > UPPER_THRESHOLD) {
    //   ultrasonicSerial.println("CMD_RANGE_UP");
     if (yVal < LOWER_THRESHOLD) {  // LOW value = UP (inverted)
    ultrasonicSerial.println("CMD_RANGE_UP");
}
      Serial.println("📤 RANGE_UP → ESP-4");
      lastCommand = millis();
    }
    // ⬇️ DOWN: Decrease ultrasonic range
    else if (yVal < LOWER_THRESHOLD) {
      ultrasonicSerial.println("CMD_RANGE_DOWN");
      Serial.println("📤 RANGE_DOWN → ESP-4");
      lastCommand = millis();
    }
    
    // ⬅️ LEFT: Focus left sensor
    if (xVal < LOWER_THRESHOLD) {
      ultrasonicSerial.println("CMD_FOCUS_LEFT");
      Serial.println("📤 FOCUS_LEFT → ESP-4");
      lastCommand = millis();
    }
    // ➡️ RIGHT: Focus right sensor
    else if (xVal > UPPER_THRESHOLD) {
      ultrasonicSerial.println("CMD_FOCUS_RIGHT");
      Serial.println("📤 FOCUS_RIGHT → ESP-4");
      lastCommand = millis();
    }
  }
  
  // 🔘 BUTTON: Tell time (only when NO fall detected)
  if (buttonState == LOW && !fallDetected) {
    delay(50);
    if (digitalRead(buttonPin) == LOW) {
      if (millis() - lastTimeRequest > TIME_REQUEST_COOLDOWN) {
        requestTimeAnnouncement();
        lastTimeRequest = millis();
        delay(300);
      }
    }
  }
}

/* ============================================
   NETWORK FUNCTIONS
   ============================================ */

void sendFallAlertToLaptop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi not connected");
    return;
  }
  
  HTTPClient http;
  http.begin(laptopAlertUrl);
  http.setTimeout(3000);
  http.addHeader("Content-Type", "application/json");
  
  String payload = "{\"event\":\"fall_detected\",\"timestamp\":" + String(millis()) + "}";
  
  int responseCode = http.POST(payload);
  if (responseCode > 0) {
    Serial.println("✅ Laptop notified");
  }
  
  http.end();
}

void triggerAudioAlert(const char* alertType) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  http.begin(audioAlertUrl);
  http.setTimeout(3000);
  http.addHeader("Content-Type", "application/json");
  
  String payload = "{\"object\":\"emergency\",\"distance\":0,\"type\":\"" + String(alertType) + "\"}";
  
  int responseCode = http.POST(payload);
  if (responseCode > 0) {
    Serial.printf("🔊 Audio: %s\n", alertType);
  }
  
  http.end();
}

/* ============================================
   SETUP
   ============================================ */

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║   🚑 FALL DETECTION (SIMPLE) 🚑      ║");
  Serial.println("╚═══════════════════════════════════════╝\n");
  
  // Joystick
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("✅ Joystick ready");
  
  // MPU6050
  Wire.begin();
  mpu.initialize();
  
  if (mpu.testConnection()) {
    Serial.println("✅ MPU6050 connected");
  } else {
    Serial.println("❌ MPU6050 FAILED!");
    while (true) delay(1000);
  }
  
  // UART to Ultrasonic ESP32
  ultrasonicSerial.begin(115200, SERIAL_8N1, ULTRASONIC_RX, ULTRASONIC_TX);
  Serial.println("✅ UART to ESP-4 ready");
  
  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("📡 WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi connected");
    Serial.printf("📍 IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("⚠️  WiFi failed");
  }
  
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  ✅ READY - MONITORING FALLS          ║");
  Serial.println("╚═══════════════════════════════════════╝\n");
}

/* ============================================
   MAIN LOOP
   ============================================ */

void loop() {
  // Check for fall
  if (!fallDetected && checkForFall()) {
    Serial.println("\n⚠️⚠️⚠️ FALL DETECTED! ⚠️⚠️⚠️");
    
    fallDetected = true;
    
    // Send alerts
    sendFallAlertToLaptop();
    triggerAudioAlert("fall_alert");
    
    Serial.println("🔘 Press button to CANCEL alert\n");
  }
  
  // If fall detected, check for cancel
  if (fallDetected && checkCancelButton()) {
    Serial.println("✅ Fall alert CANCELED by user\n");
    
    fallDetected = false;
    triggerAudioAlert("fall_canceled");
    
    // Small cooldown
    delay(2000);
  }
  
  // Handle joystick
  handleJoystickCommands();
  
  delay(100);
}




// ESP-3