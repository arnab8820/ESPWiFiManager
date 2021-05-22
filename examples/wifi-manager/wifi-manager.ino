#include <ESPWiFiManager.h>

// Create an instance of ESPWiFiManager class
ESPWiFiManager wifi;

void setup() {
  // Initialize wifi manager
  // Once initialized, the application will run on port 8080. 
  // MAKE SURE YOUR APPLICATION DOES NOT USE THE SAME PORT 
  // If you connect to the ESP wifi hotspot, visit http://192.168.1.2:8080 from your browser
  // If ESP is already connected to a network, visit http://<esp_device_ip>:8080 from your browser
  
  wifi.initWifiManager();
}

void loop() {
  // Handle incoming http requests to the wifi manager application
  wifi.handleHttpRequest();
}
