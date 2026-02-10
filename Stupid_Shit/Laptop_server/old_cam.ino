// #include "esp_camera.h"
// #include <WiFi.h>
// #include <HTTPClient.h>

// /*  USER CONFIG  */

// const char* ssid     = "YOUR_WIFI_NAME";
// const char* password = "YOUR_WIFI_PASSWORD";

// // Laptop YOLO server endpoint
// const char* serverUrl = "http://LAPTOP_IP:8000/frame";


// // ESP32-CAM pin definition
// #define CAMERA_MODEL_AI_THINKER
// #include "camera_pins.h"

// /* Camera */
// void setupCamera() {
//   camera_config_t config;
//   config.ledc_channel = LEDC_CHANNEL_0;
//   config.ledc_timer   = LEDC_TIMER_0;
//   config.pin_d0       = Y2_GPIO_NUM;
//   config.pin_d1       = Y3_GPIO_NUM;
//   config.pin_d2       = Y4_GPIO_NUM;
//   config.pin_d3       = Y5_GPIO_NUM;
//   config.pin_d4       = Y6_GPIO_NUM;
//   config.pin_d5       = Y7_GPIO_NUM;
//   config.pin_d6       = Y8_GPIO_NUM;
//   config.pin_d7       = Y9_GPIO_NUM;
//   config.pin_xclk     = XCLK_GPIO_NUM;
//   config.pin_pclk     = PCLK_GPIO_NUM;
//   config.pin_vsync    = VSYNC_GPIO_NUM;
//   config.pin_href     = HREF_GPIO_NUM;
//   config.pin_sccb_sda = SIOD_GPIO_NUM;
//   config.pin_sccb_scl = SIOC_GPIO_NUM;
//   config.pin_pwdn     = PWDN_GPIO_NUM;
//   config.pin_reset    = RESET_GPIO_NUM;

//   config.xclk_freq_hz = 20000000;
//   config.pixel_format = PIXFORMAT_JPEG;

//   // IMPORTANT: Stable + fast
//   config.frame_size   = FRAMESIZE_QVGA; // 320x240
//   config.jpeg_quality = 35;             // prolly needs tuning cuz no way 35
//   config.fb_count     = 1;

//   esp_err_t err = esp_camera_init(&config);
//   if (err != ESP_OK) {
//     Serial.printf("Camera init failed: 0x%x\n", err);
//     while (true);
//   }

//   Serial.println("Camera initialized");
// }

// /* ---------- Setup ---------- */
// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   Serial.println("\nESP32-CAM Sender Starting");

//   // WiFi
//   WiFi.begin(ssid, password);
//   Serial.print("📡 Connecting to WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.println("\nWiFi connected");
//   Serial.print("ESP32-CAM IP: ");
//   Serial.println(WiFi.localIP());

//   setupCamera();
// }

// /* Main */
// void loop() {
//   camera_fb_t *fb = esp_camera_fb_get();
//   if (!fb) {
//     Serial.println("Camera capture failed");
//     delay(200);
//     return;
//   }

//   HTTPClient http;
//   http.begin(serverUrl);
//   http.addHeader("Content-Type", "image/jpeg");

//   int responseCode = http.POST(fb->buf, fb->len);

//   if (responseCode > 0) {
//     Serial.println("Frame sent");
//   } else {
//     Serial.println("Failed to send frame");
//   }

//   http.end();
//   esp_camera_fb_return(fb);

//   delay(200); // ~5 FPS (CRITICAL FOR STABILITY)
// }







