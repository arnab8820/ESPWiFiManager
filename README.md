# ESPWiFiManager
**This is a WiFi manager application for ESP8266 boards. With this library, you don't need to hardcode your wifi credentials inside your code and painfully change it everytime you change wifi credentials. This library provides a web interface where you can scan and connect to a wifi network of your choice.**


## Important:
* Once initialized, the application will run on port 8080. 
* MAKE SURE YOUR APPLICATION DOES NOT USE THE SAME PORT 
* If you connect to the ESP wifi hotspot, visit http://192.168.1.2:8080 from your browser
* If ESP is already connected to a network, visit http://<esp_device_ip>:8080 from your browser


### Pros:
* Better and user freindly web interface
* Full ability to scan and connect to a network
* Supports Static IP scenarios
* Auto reconnect to last network
* Seamless integration wit your project
* Only 3 lines of code to get started 

### Dependencies:
This library depend on the following libraries, so make sure you install them too
* ESP8266WiFi
* ArduinoJson
* ESP8266WebServer

### Screenshots:
![screenshot1](https://github.com/arnab8820/ESPWiFiManager/blob/fb81d772ef5657301d675332d1309c2a1e119fdc/screenshots/IMG_20210522_210833.jpg)
![screenshot2](https://github.com/arnab8820/ESPWiFiManager/blob/fb81d772ef5657301d675332d1309c2a1e119fdc/screenshots/IMG_20210522_210916.jpg)
![screenshot3](https://github.com/arnab8820/ESPWiFiManager/blob/fb81d772ef5657301d675332d1309c2a1e119fdc/screenshots/IMG_20210522_210944.jpg)
