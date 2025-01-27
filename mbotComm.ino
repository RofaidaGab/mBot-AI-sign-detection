#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HardwareSerial.h>
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"


// Arduino communication
const char* arduino_ip = "192.168.97.90"; // Replace with your Arduino's IP
const int arduino_port = 8080;



// WiFi credentials
const char* ssid = "Rofy";
const char* password = "rofaidahotnet";
const int port = 8080;
WiFiServer server(port);

HardwareSerial uartSerial(0); // Use UART0 for communication with Arduino

void setup() {
  Serial.begin(115200);
  uartSerial.begin(9600, SERIAL_8N1, 3, 1); // Configure UART0: RX = GPIO3, TX = GPIO1
  Serial.setDebugOutput(true);
  Serial.println();

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // WiFi setup
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
  server.begin();

  // Start the camera server
  extern void startCameraServer(); // Use the definition from app_httpd.cpp
  startCameraServer();
}

void loop() {
  static WiFiClient client;
  
  if (!client.connected()) {
    client = server.available();
  }
  
  if (client && client.connected()) {
    if (client.available()) {
      String command = client.readStringUntil('\n');
      command.trim();
      Serial.println("Received command: " + command);

      // Process commands and send to Arduino via UART
      if (command == "start") {
        Serial.println("Action: Sending 'start' to Arduino via UART");
        sendToArduino("start");
      } else if (command == "stop") {
        Serial.println("Action: Sending 'stop' to Arduino via UART");
        sendToArduino("stop");
      } else if (command.startsWith("c-road")) {
        int commaIndex = command.indexOf(',');
        if (commaIndex > 0) {
          int diameter = command.substring(commaIndex + 1).toInt();
          Serial.printf("Action: Sending 'c-road' with diameter %d to Arduino via UART\n", diameter);
          sendToArduino("c-road," + String(diameter));
        }
      } else if (command == "c-road-end") {
        Serial.println("Action: Sending 'c-road-end' to Arduino via UART");
        sendToArduino("c-road-end");
      }
    }
  }
}

void sendToArduino(String message) {
  static String lastCommand = "";  // Avoid sending duplicate commands

  if (message != lastCommand) {  // Only send if the command is new
    uartSerial.println(message);  // Send the command via UART
    Serial.printf("Command sent to Arduino via UART: %s\n", message.c_str());
    lastCommand = message;  // Update the last sent command
  } else {
    Serial.printf("Command '%s' already sent to Arduino. Skipping duplicate.\n", message.c_str());
  }
}
