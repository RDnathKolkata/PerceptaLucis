// /*
//  * ESP32 Fall Detection System
//  * 
//  * Hardware:
//  * - ESP32 DevKit
//  * - MPU6050 (I2C: SDA=GPIO21, SCL=GPIO22)
//  * - Joystick Module (VRx=GPIO34, VRy=GPIO35, SW=GPIO32)
//  * - SIM800L (TX=GPIO17, RX=GPIO16, RST=GPIO4)
//  * 
//  * Features:
//  * - Fall detection using MPU6050
//  * - 15-second countdown with joystick cancel
//  * - Emergency SMS + voice call via SIM800L
//  * - WiFi alerts to laptop and audio ESP32
//  * - Automatic 15s pause after event
//  * 
//  * Author: Your Name
//  * Date: January 2026
//  */

// #include <Wire.h>
// #include <MPU6050.h>
// #include <WiFi.h>
// #include <HTTPClient.h>

// /* user config */

// HardwareSerial ultrasonicSerial(2);  // UART to ESP-4
// const int ULTRASONIC_TX = 26;
// const int ULTRASONIC_RX = 27;

// unsigned long lastTimeRequest = 0;
// const unsigned long TIME_REQUEST_COOLDOWN = 2000;

// #define ULTRASONIC_TX 17
// #define ULTRASONIC_RX 16
// HardwareSerial ultrasonicSerial(2);  // Use UART2

// // In setup(), add:
// ultrasonicSerial.begin(115200, SERIAL_8N1, ULTRASONIC_RX, ULTRASONIC_TX);
// Serial.println("✅ UART to ultrasonic ESP32 initialized");

// // Replace handleJoystickCommands() with:
// void handleJoystickCommands() {
//   int xVal = analogRead(VRxPin);
//   int yVal = analogRead(VRyPin);
  
//   static unsigned long lastCommand = 0;
//   if (millis() - lastCommand < 500) return;  // Debounce
  
//   // Y-axis (Up/Down)
//   if (yVal > UPPER_THRESHOLD) {
//     ultrasonicSerial.println("CMD_RANGE_UP");
//     Serial.println("📤 Sent to ultrasonic: CMD_RANGE_UP");
//     lastCommand = millis();
//   }
//   else if (yVal < LOWER_THRESHOLD) {
//     ultrasonicSerial.println("CMD_RANGE_DOWN");
//     Serial.println("📤 Sent to ultrasonic: CMD_RANGE_DOWN");
//     lastCommand = millis();
//   }
  
//   // X-axis (Left/Right)
//   if (xVal > UPPER_THRESHOLD) {
//     ultrasonicSerial.println("CMD_FOCUS_RIGHT");
//     Serial.println("📤 Sent to ultrasonic: CMD_FOCUS_RIGHT");
//     lastCommand = millis();
//   }
//   else if (xVal < LOWER_THRESHOLD) {
//     ultrasonicSerial.println("CMD_FOCUS_LEFT");
//     Serial.println("📤 Sent to ultrasonic: CMD_FOCUS_LEFT");
//     lastCommand = millis();
//   }
// }

// // WiFi creds
// const char* ssid = "YOUR_WIFI_NAME";
// const char* password = "YOUR_WIFI_PASSWORD";

// // Emergency contact (with country code)
// const char* emergencyPhone = "+918420046163";  

// // Server endpoints
// const char* laptopAlertUrl = "http://192.168.1.100:8000/fall_alert";  // Change IP
// const char* audioAlertUrl = "http://192.168.1.101/alert";             // Change IP

// // Pin definitions
// const int VRxPin = 34;        // Joystick X-axis
// const int VRyPin = 35;        // Joystick Y-axis  
// const int buttonPin = 32;     // Joystick button
// const int SIM800_TX = 17;     // Connect to SIM800L RX
// const int SIM800_RX = 16;     // Connect to SIM800L TX
// const int SIM800_RST = 4;     // SIM800L Reset pin

// // Fall detection parameters
// const float FALL_THRESHOLD = 2.5;      // High acceleration threshold (g)
// const float FREEFALL_THRESHOLD = 0.5;  // Low acceleration threshold (g)
// const unsigned long COUNTDOWN_TIME = 15000; // 15 seconds in milliseconds

// // Joystick parameters
// const int ADC_CENTER = 2048;
// const int DEADZONE = 200;
// const int UPPER_THRESHOLD = ADC_CENTER + DEADZONE;
// const int LOWER_THRESHOLD = ADC_CENTER - DEADZONE;

// /* the globe varies ig */

// MPU6050 mpu;
// HardwareSerial sim800(1); // Use UART1 for SIM800L

// // System state machine
// enum SystemState {
//   STATE_MONITORING,
//   STATE_COUNTDOWN,
//   STATE_CALLING,
//   STATE_PAUSED
// };

// SystemState currentState = STATE_MONITORING;
// unsigned long countdownStartTime = 0;
// unsigned long pauseStartTime = 0;
// unsigned long lastFallCheck = 0;
// bool emergencyCanceled = false;

// /* khul ja sim sim */

// void sim800_init() {
//   pinMode(SIM800_RST, OUTPUT);
//   digitalWrite(SIM800_RST, HIGH);
//   delay(100);
  
//   sim800.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX);
  
//   Serial.println("🔄 Initializing SIM800L...");
//   delay(3000);
  
//   // Reset module
//   digitalWrite(SIM800_RST, LOW);
//   delay(100);
//   digitalWrite(SIM800_RST, HIGH);
//   delay(3000);
  
//   // Check if module responds
//   sim800.println("AT");
//   delay(1000);
  
//   // Read response
//   while (sim800.available()) {
//     String response = sim800.readString();
//     if (response.indexOf("OK") >= 0) {
//       Serial.println("✅ SIM800L responding");
//     }
//   }
  
//   // Set SMS to text mode
//   sim800.println("AT+CMGF=1");
//   delay(1000);
  
//   // Check signal strength
//   sim800.println("AT+CSQ");
//   delay(1000);
//   while (sim800.available()) {
//     Serial.println(sim800.readString());
//   }
  
//   Serial.println("✅ SIM800L initialized");
// }

// void sendSMS(const char* number, const char* message) {
//   Serial.printf("📤 Sending SMS to %s\n", number);
//   Serial.printf("   Message: %s\n", message);
  
//   sim800.print("AT+CMGS=\"");
//   sim800.print(number);
//   sim800.println("\"");
//   delay(1000);
  
//   sim800.print(message);
//   delay(100);
  
//   sim800.write(26); // Ctrl+Z to send SMS
//   delay(5000);
  
//   // Read response
//   while (sim800.available()) {
//     Serial.println(sim800.readString());
//   }
  
//   Serial.println("✅ SMS sent");
// }

// void makeCall(const char* number) {
//   Serial.printf("📞 Calling %s\n", number);
  
//   sim800.print("ATD");
//   sim800.print(number);
//   sim800.println(";");
//   delay(1000);
  
//   // Read response
//   while (sim800.available()) {
//     Serial.println(sim800.readString());
//   }
  
//   Serial.println("✅ Call initiated - ringing for 30 seconds");
  
//   // Ring for 30 seconds
//   delay(30000);
  
//   // Hang up
//   sim800.println("ATH");
//   delay(1000);
  
//   Serial.println("📵 Call ended");
// }

// /* ============================================
//    FALL DETECTION
//    ============================================ */

// bool checkForFall() {
//   int16_t ax, ay, az;
//   mpu.getAcceleration(&ax, &ay, &az);
  
//   // Convert raw values to g's (±2g range, 16384 LSB/g)
//   float AccX = ax / 16384.0;
//   float AccY = ay / 16384.0;
//   float AccZ = az / 16384.0;
  
//   // Calculate total acceleration magnitude
//   float AccMagnitude = sqrt(AccX * AccX + AccY * AccY + AccZ * AccZ);
  
//   // Print acceleration every second for monitoring
//   if (millis() - lastFallCheck > 1000) {
//     Serial.printf("📊 Acceleration: %.2fg | State: ", AccMagnitude);
//     switch(currentState) {
//       case STATE_MONITORING: Serial.println("MONITORING"); break;
//       case STATE_COUNTDOWN: Serial.println("COUNTDOWN"); break;
//       case STATE_CALLING: Serial.println("CALLING"); break;
//       case STATE_PAUSED: Serial.println("PAUSED"); break;
//     }
//     lastFallCheck = millis();
//   }
  
//   // Detect fall:
//   // 1. High impact (sudden acceleration spike)
//   // 2. Free fall (near-zero acceleration)
//   if (AccMagnitude > FALL_THRESHOLD || AccMagnitude < FREEFALL_THRESHOLD) {
//     Serial.printf("⚠️ Abnormal acceleration detected: %.2fg\n", AccMagnitude);
//     return true;
//   }
  
//   return false;
// }

// /* ============================================
//    JOYSTICK FUNCTIONS
//    ============================================ */

// bool checkCancelButton() {
//   int buttonState = digitalRead(buttonPin);
//   if (buttonState == LOW) {
//     delay(50); // Debounce
//     if (digitalRead(buttonPin) == LOW) {
//       Serial.println("🔘 Cancel button pressed!");
//       return true;
//     }
//   }
//   return false;
// }

// void handleJoystickCommands() {
//   int xVal = analogRead(VRxPin);
//   int yVal = analogRead(VRyPin);
  
//   // Optional: Use joystick directions for other features
//   static unsigned long lastCommand = 0;
//   if (millis() - lastCommand < 500) return; // Debounce
  
//   if (yVal > UPPER_THRESHOLD) {
//     Serial.println("🕐 Joystick UP");
//     lastCommand = millis();
//   }
//   else if (yVal < LOWER_THRESHOLD) {
//     Serial.println("📳 Joystick DOWN");
//     lastCommand = millis();
//   }
  
//   if (xVal > UPPER_THRESHOLD) {
//     Serial.println("➡️ Joystick RIGHT");
//     lastCommand = millis();
//   }
//   else if (xVal < LOWER_THRESHOLD) {
//     Serial.println("⬅️ Joystick LEFT");
//     lastCommand = millis();
//   }
// }

// /* ============================================
//    NETWORK FUNCTIONS
//    ============================================ */

// void sendFallAlertToLaptop() {
//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println("⚠️ WiFi not connected - skipping laptop alert");
//     return;
//   }
  
//   HTTPClient http;
//   http.begin(laptopAlertUrl);
//   http.setTimeout(3000);
//   http.addHeader("Content-Type", "application/json");
  
//   String payload = "{\"event\":\"fall_detected\",\"timestamp\":" + String(millis()) + "}";
  
//   int responseCode = http.POST(payload);
//   if (responseCode > 0) {
//     Serial.printf("✅ Fall alert sent to laptop (HTTP %d)\n", responseCode);
//   } else {
//     Serial.printf("⚠️ Failed to send fall alert (Error: %s)\n", 
//                   http.errorToString(responseCode).c_str());
//   }
  
//   http.end();
// }

// void triggerAudioAlert(const char* alertType) {
//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println("⚠️ WiFi not connected - skipping audio alert");
//     return;
//   }
  
//   HTTPClient http;
//   http.begin(audioAlertUrl);
//   http.setTimeout(3000);
//   http.addHeader("Content-Type", "application/json");
  
//   String payload = "{\"object\":\"emergency\",\"distance\":0,\"type\":\"" + String(alertType) + "\"}";
  
//   int responseCode = http.POST(payload);
//   if (responseCode > 0) {
//     Serial.printf("🔊 Audio alert triggered: %s (HTTP %d)\n", alertType, responseCode);
//   } else {
//     Serial.printf("⚠️ Failed to trigger audio (Error: %s)\n", 
//                   http.errorToString(responseCode).c_str());
//   }
  
//   http.end();
// }

// /* ============================================
//    STATE MACHINE HANDLERS
//    ============================================ */

// void handleMonitoringState() {
//   // Check for fall
//   if (checkForFall()) {
//     Serial.println("\n╔════════════════════════════════════╗");
//     Serial.println("║  ⚠️  FALL DETECTED!  ⚠️           ║");
//     Serial.println("╚════════════════════════════════════╝\n");
    
//     // Transition to countdown
//     currentState = STATE_COUNTDOWN;
//     countdownStartTime = millis();
//     emergencyCanceled = false;
    
//     // Alert systems
//     sendFallAlertToLaptop();
//     triggerAudioAlert("fall_countdown");
    
//     Serial.println("⏱️  15 SECOND COUNTDOWN STARTED");
//     Serial.println("🔘 Press joystick button to CANCEL emergency call\n");
//   }
  
//   // Handle joystick commands (optional features)
//   handleJoystickCommands();
// }

// void handleCountdownState() {
//   unsigned long elapsed = millis() - countdownStartTime;
//   unsigned long remaining = COUNTDOWN_TIME - elapsed;
  
//   // Print countdown every second
//   static unsigned long lastPrint = 0;
//   if (millis() - lastPrint >= 1000) {
//     Serial.printf("⏱️  Calling emergency in %lu seconds... (Press button to CANCEL)\n", 
//                   remaining / 1000);
//     lastPrint = millis();
//   }
  
//   // Check for cancel button
//   if (checkCancelButton()) {
//     Serial.println("\n╔════════════════════════════════════╗");
//     Serial.println("║  ✅ EMERGENCY CANCELED BY USER     ║");
//     Serial.println("╚════════════════════════════════════╝\n");
    
//     emergencyCanceled = true;
    
//     // Transition to paused state
//     currentState = STATE_PAUSED;
//     pauseStartTime = millis();
    
//     triggerAudioAlert("emergency_canceled");
    
//     Serial.println("⏸️  System will resume monitoring in 15 seconds\n");
//     return;
//   }
  
//   // Countdown finished - make emergency call
//   if (elapsed >= COUNTDOWN_TIME) {
//     Serial.println("\n╔════════════════════════════════════╗");
//     Serial.println("║  🚨 INITIATING EMERGENCY CALL 🚨   ║");
//     Serial.println("╚════════════════════════════════════╝\n");
    
//     currentState = STATE_CALLING;
//     triggerAudioAlert("calling_emergency");
//   }
// }

// void handleCallingState() {
//   Serial.println("📞 EMERGENCY PROTOCOL ACTIVATED\n");
  
//   // Send SMS first
//   String smsMessage = "EMERGENCY ALERT: Fall detected! User may need immediate assistance. Device time: ";
//   smsMessage += String(millis() / 1000);
//   smsMessage += "s";
  
//   sendSMS(emergencyPhone, smsMessage.c_str());
  
//   delay(2000);
  
//   // Make voice call
//   makeCall(emergencyPhone);
  
//   Serial.println("\n✅ Emergency protocol completed");
//   Serial.println("⏸️  Pausing system for 15 seconds to prevent repeated alerts\n");
  
//   // Transition to paused state
//   currentState = STATE_PAUSED;
//   pauseStartTime = millis();
// }

// void handlePausedState() {
//   unsigned long elapsed = millis() - pauseStartTime;
//   unsigned long remaining = 15000 - elapsed;
  
//   // Print status every 5 seconds
//   static unsigned long lastPrint = 0;
//   if (millis() - lastPrint >= 5000) {
//     Serial.printf("⏸️  System paused for %lu more seconds\n", remaining / 1000);
//     lastPrint = millis();
//   }
  
//   // Resume monitoring after 15 seconds
//   if (elapsed >= 15000) {
//     Serial.println("\n╔════════════════════════════════════╗");
//     Serial.println("║  ✅ RESUMING FALL DETECTION       ║");
//     Serial.println("╚════════════════════════════════════╝\n");
    
//     currentState = STATE_MONITORING;
//     triggerAudioAlert("monitoring_resumed");
//   }
// }

// /* ============================================
//    SETUP
//    ============================================ */

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
  
//   Serial.println("\n");
//   Serial.println("╔═══════════════════════════════════════╗");
//   Serial.println("║   🚑 ESP32 FALL DETECTION SYSTEM 🚑  ║");
//   Serial.println("╚═══════════════════════════════════════╝");
//   Serial.println();
  
//   // Initialize joystick button
//   pinMode(buttonPin, INPUT_PULLUP);
//   Serial.println("✅ Joystick initialized");
  
//   // Initialize MPU6050
//   Wire.begin();
//   mpu.initialize();
  
//   if (mpu.testConnection()) {
//     Serial.println("✅ MPU6050 connected");
//   } else {
//     Serial.println("❌ MPU6050 connection FAILED!");
//     Serial.println("   Check I2C wiring: SDA=GPIO21, SCL=GPIO22");
//     while (true) {
//       delay(1000);
//     }
//   }
  
//   // Initialize WiFi (optional - system works without it)
//   WiFi.begin(ssid, password);
//   Serial.print("📡 Connecting to WiFi");
  
//   int attempts = 0;
//   while (WiFi.status() != WL_CONNECTED && attempts < 20) {
//     delay(500);
//     Serial.print(".");
//     attempts++;
//   }
//   Serial.println();
  
//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("✅ WiFi connected");
//     Serial.printf("📍 IP Address: %s\n", WiFi.localIP().toString().c_str());
//   } else {
//     Serial.println("⚠️  WiFi connection failed");
//     Serial.println("   System will operate in OFFLINE mode");
//     Serial.println("   Emergency calls will still work via SIM800L");
//   }
  
//   // Initialize SIM800L
//   sim800_init();
  
//   Serial.println();
//   Serial.println("╔═══════════════════════════════════════╗");
//   Serial.println("║  ✅ SYSTEM READY                      ║");
//   Serial.println("║  📊 MONITORING FOR FALLS...           ║");
//   Serial.println("╚═══════════════════════════════════════╝");
//   Serial.println();
  
//   currentState = STATE_MONITORING;
// }

// /* ============================================
//    MAIN LOOP
//    ============================================ */

// void loop() {
//   // State machine
//   switch (currentState) {
//     case STATE_MONITORING:
//       handleMonitoringState();
//       delay(100);
//       break;
      
//     case STATE_COUNTDOWN:
//       handleCountdownState();
//       delay(100);
//       break;
      
//     case STATE_CALLING:
//       handleCallingState();
//       break;
      
//     case STATE_PAUSED:
//       handlePausedState();
//       delay(1000);
//       break;
//   }
  
//   // Check WiFi connection periodically
//   static unsigned long lastWiFiCheck = 0;
//   if (millis() - lastWiFiCheck > 30000) { // Every 30 seconds
//     if (WiFi.status() != WL_CONNECTED) {
//       Serial.println( WiFi disconnected, attempting reconnection...");
//       WiFi.reconnect();
//     }
//     lastWiFiCheck = millis();
//   }
// }
