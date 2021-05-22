#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "ESPWiFiManager.h"

void ESPWiFiManager::handleRoot(){
  Serial.println("Request on /");
  server->send(200, "text/html", webpage);
}

void ESPWiFiManager::handleScan(){
  Serial.println("Request on /scan");
  int n = WiFi.scanNetworks();
  String scanData;
  const size_t arrSize = JSON_ARRAY_SIZE(1024);
  StaticJsonDocument<arrSize> arr;
  JsonArray aplist = arr.to<JsonArray>();
  if (n == 0) { 
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.println(WiFi.SSID(i)+"\t"+WiFi.encryptionType(i)+"\t"+WiFi.RSSI(i));
      const size_t CAPACITY = JSON_OBJECT_SIZE(5);
      StaticJsonDocument<CAPACITY> doc;
      JsonObject ap = doc.to<JsonObject>();
      ap["ssid"] = WiFi.SSID(i);
      if (WiFi.isConnected() && WiFi.SSID(i) == WiFi.SSID()){
        ap["status"] = "Connected";
      } else {
        ap["status"] = WiFi.encryptionType(i)==5?"Secured(WEP)":WiFi.encryptionType(i)==2?"Secured(WPA)":WiFi.encryptionType(i)==4?"Secured(WPA2)":WiFi.encryptionType(i)==7?"Open":"Secured(WPA2/WPA Auto)";
      }
      
      ap["signal"] = WiFi.RSSI(i);
      aplist.add(ap);
    }
    serializeJson(aplist, scanData);
    server->send(200, "application/json", scanData);
  }
  Serial.println("");
}

void ESPWiFiManager::handleConnect(){
  if (server->method() != HTTP_POST) {
    server->send(405, "text/plain", "Method Not Allowed");
  } else {
    String postData = server->arg("plain");
    DynamicJsonDocument doc(1000);
    DeserializationError error = deserializeJson(doc, postData);
    if (error) {
      Serial.println(F("ERROR: deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    } else {
      const char* ssid = doc["ssid"];
      const char* password = doc["password"];
      const char* ipType = doc["ipType"];
      IPAddress ipaddr(doc["ipAddress"][0], doc["ipAddress"][1], doc["ipAddress"][2], doc["ipAddress"][3]);
      IPAddress sbnetMask(doc["subnetMask"][0], doc["subnetMask"][1], doc["subnetMask"][2], doc["subnetMask"][3]);
      IPAddress gateway(doc["gateway"][0], doc["gateway"][1], doc["gateway"][2], doc["gateway"][3]);
      IPAddress primaryDns(doc["primaryDns"][0], doc["primaryDns"][1], doc["primaryDns"][2], doc["primaryDns"][3]);
      IPAddress seconderyDns(doc["seconderyDns"][0], doc["seconderyDns"][1], doc["seconderyDns"][2], doc["seconderyDns"][3]);
      
      if(strcmp(ipType, "dhcp")!=0){
        if (!WiFi.config(ipaddr, gateway, sbnetMask, primaryDns, seconderyDns)) {
          Serial.println("STA Failed to configure");
          const size_t CAPACITY = JSON_OBJECT_SIZE(5);
          StaticJsonDocument<CAPACITY> doc;
          JsonObject resp = doc.to<JsonObject>();
          resp["success"] = "false";
          resp["message"] = "Failed to configure STA!";
          String respData;
          serializeJson(resp, respData);
          server->send(500, "application/json", respData);
          return;
        }
      }
      if(WiFi.getMode() == WIFI_STA){
        const size_t CAPACITY = JSON_OBJECT_SIZE(5);
        StaticJsonDocument<CAPACITY> doc;
        JsonObject resp = doc.to<JsonObject>();
        resp["success"] = "false";
        resp["status"] = "unknown";
        String respData;
        serializeJson(resp, respData);
        server->send(201, "application/json", respData);
        delay(1000);
      }
      WiFi.disconnect();
      delay(500);
      WiFi.begin(ssid, password);
      switch(WiFi.waitForConnectResult()) {
        case WL_CONNECTED:{
          // Writing config to SPIFFS
          File wconf = SPIFFS.open("wificonfig.json", "w");
          if(!wconf)
          {
            Serial.println("Unable to open WiFi configuration file!");
          }
          wconf.print(postData);
          wconf.close();

          // Populating local config struct
          ESPWiFiManager::ssid = strdup(ssid);
          ESPWiFiManager::password = strdup(password);
          ESPWiFiManager::ipType = strdup(ipType);
          ESPWiFiManager::ipAddress=ipaddr;
          ESPWiFiManager::sbnetMask=sbnetMask;
          ESPWiFiManager::gateway=gateway;
          ESPWiFiManager::primaryDns=primaryDns;
          ESPWiFiManager::seconderyDns=seconderyDns;

          // Sending HTTP response
          if(WiFi.getMode() == WIFI_AP_STA){
            const size_t CAPACITY = JSON_OBJECT_SIZE(5);
            StaticJsonDocument<CAPACITY> doc;
            JsonObject resp = doc.to<JsonObject>();
            resp["success"] = "true";
            resp["ip"] = WiFi.localIP();
            String respData;
            serializeJson(resp, respData);
            server->send(201, "application/json", respData);
            delay(1000);
          }
          
          // Shutdown softAP and reconnect in station mode
          WiFi.softAPdisconnect(true);
          WiFi.disconnect();
          WiFi.mode(WIFI_STA);
          if(strcmp(ESPWiFiManager::ipType, "dhcp")!=0){
            if (!WiFi.config(ESPWiFiManager::ipAddress, ESPWiFiManager::gateway, ESPWiFiManager::sbnetMask, ESPWiFiManager::primaryDns, ESPWiFiManager::seconderyDns)) {
              Serial.println("STA Failed to reconfigure");
              return;
            }
          }
          WiFi.begin(ESPWiFiManager::ssid, ESPWiFiManager::password);
          if(WiFi.waitForConnectResult() != WL_CONNECTED){
            Serial.println("STA Failed to reconnect!");
            Serial.println("Restarting Device ...");
            ESP.restart();
          }
          WiFi.setAutoReconnect(true);
          WiFi.persistent(true);
          break;
        }
        case WL_NO_SSID_AVAIL:{
          if(WiFi.getMode() == WIFI_AP_STA){
            const size_t CAPACITY = JSON_OBJECT_SIZE(5);
            StaticJsonDocument<CAPACITY> doc;
            JsonObject resp = doc.to<JsonObject>();
            resp["success"] = "false";
            resp["message"] = "Selected access point is not available!";
            String respData;
            serializeJson(resp, respData);
            server->send(500, "application/json", respData);
          }
          wifiFallback();
          break;
        }
        case WL_CONNECT_FAILED:{
          if(WiFi.getMode() == WIFI_AP_STA){
            const size_t CAPACITY = JSON_OBJECT_SIZE(5);
            StaticJsonDocument<CAPACITY> doc;
            JsonObject resp = doc.to<JsonObject>();
            resp["success"] = "false";
            resp["message"] = "Failed to connect!";
            String respData;
            serializeJson(resp, respData);
            server->send(500, "application/json", respData);
          }
          wifiFallback();
          break;
        }
        case WL_IDLE_STATUS:{
          if(WiFi.getMode() == WIFI_AP_STA){
            const size_t CAPACITY = JSON_OBJECT_SIZE(5);
            StaticJsonDocument<CAPACITY> doc;
            JsonObject resp = doc.to<JsonObject>();
            resp["success"] = "false";
            resp["message"] = "Selected access point is not responding!";
            String respData;
            serializeJson(resp, respData);
            server->send(500, "application/json", respData);
          }
          wifiFallback();
          break;
        }
        case WL_DISCONNECTED:{
          if(WiFi.getMode() == WIFI_AP_STA){
            const size_t CAPACITY = JSON_OBJECT_SIZE(5);
            StaticJsonDocument<CAPACITY> doc;
            JsonObject resp = doc.to<JsonObject>();
            resp["success"] = "false";
            resp["message"] = "Wi-Fi radio is offline on this device!";
            String respData;
            serializeJson(resp, respData);
            server->send(500, "application/json", respData);
          }
          wifiFallback();
          break;
        }
        case -1:{
          if(WiFi.getMode() == WIFI_AP_STA){
            const size_t CAPACITY = JSON_OBJECT_SIZE(5);
            StaticJsonDocument<CAPACITY> doc;
            JsonObject resp = doc.to<JsonObject>();
            resp["success"] = "false";
            resp["message"] = "Timed out while connecting!";
            String respData;
            serializeJson(resp, respData);
            server->send(500, "application/json", respData);
          }
          wifiFallback();
          break;
        }
        default:{
          if(WiFi.getMode() == WIFI_AP_STA){
            const size_t CAPACITY = JSON_OBJECT_SIZE(5);
            StaticJsonDocument<CAPACITY> doc;
            JsonObject resp = doc.to<JsonObject>();
            resp["success"] = "false";
            resp["message"] = "Unknown error occurred!";
            String respData;
            serializeJson(resp, respData);
            server->send(500, "application/json", respData);
          }
          wifiFallback();
          break;
        }
      }
    }
  }
}

bool ESPWiFiManager::getSavedWifiConfig(){
  //try to get saved wifi credentials
  if(SPIFFS.exists("wificonfig.json")){
    //saved credentials exist 
    File wificonfig = SPIFFS.open("wificonfig.json", "r");
    if(!wificonfig){
      Serial.println("Unable to open configuration file to read!");
      return false;
    }
    size_t size = wificonfig.size();
    std::unique_ptr<char[]> buf(new char[size]);
    char* configjson;
    wificonfig.readBytes(buf.get(), size);
    wificonfig.close();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, buf.get());
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    } else {
      
      JsonObject object = doc.as<JsonObject>();
      const char* ssid = object["ssid"];
      ESPWiFiManager::ssid = strdup(ssid);
      const char* password = object["password"];
      ESPWiFiManager::password = strdup(password);
      const char* ipType = object["ipType"];
      ESPWiFiManager::ipType = strdup(ipType);
      IPAddress ipaddr(object["ipAddress"][0], object["ipAddress"][1], object["ipAddress"][2], object["ipAddress"][3]);
      ESPWiFiManager::ipAddress=ipaddr;
      IPAddress sbnetMask(object["subnetMask"][0], object["subnetMask"][1], object["subnetMask"][2], object["subnetMask"][3]);
      ESPWiFiManager::sbnetMask=sbnetMask;
      IPAddress gateway(object["gateway"][0], object["gateway"][1], object["gateway"][2], object["gateway"][3]);
      ESPWiFiManager::gateway=gateway;
      IPAddress primaryDns(object["primaryDns"][0], object["primaryDns"][1], object["primaryDns"][2], object["primaryDns"][3]);
      ESPWiFiManager::primaryDns=primaryDns;
      IPAddress seconderyDns(object["seconderyDns"][0], object["seconderyDns"][1], object["seconderyDns"][2], object["seconderyDns"][3]);
      ESPWiFiManager::seconderyDns=seconderyDns;
      return true;
    }
  } else {
    Serial.println("No saved configuration file to read!");
    return false;
  }
}

void ESPWiFiManager::initHttpServer(){
  server->on("/", std::bind(&ESPWiFiManager::handleRoot, this));
  server->on("/scan", std::bind(&ESPWiFiManager::handleScan, this));
  server->on("/connect", std::bind(&ESPWiFiManager::handleConnect, this));
  server->begin();
}

void ESPWiFiManager::createSoftAp(){
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 2), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  delay(500);
  WiFi.softAP("ESP_" + String(ESP.getChipId()));
}

void ESPWiFiManager::wifiFallback(){
  if(WiFi.getMode() == WIFI_STA){
    if(strcmp(ESPWiFiManager::ipType, "dhcp")!=0){
      if (!WiFi.config(ESPWiFiManager::ipAddress, ESPWiFiManager::gateway, ESPWiFiManager::sbnetMask, ESPWiFiManager::primaryDns, ESPWiFiManager::seconderyDns)) {
        Serial.println("STA Failed to reconfigure");
        ESP.restart();
      }
    }
    WiFi.begin(ESPWiFiManager::ssid, ESPWiFiManager::password);
    if(WiFi.waitForConnectResult() != WL_CONNECTED){
      Serial.println("STA Failed to reconnect!");
      Serial.println("Restarting Device ...");
      ESP.restart();
    }
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  } else {
    createSoftAp();
  }
}

void ESPWiFiManager::initWifiManager(){
  if(!SPIFFS.begin()){
    Serial.println("Error opening SPIFFS");
  }
  if(getSavedWifiConfig()){
    WiFi.mode(WIFI_STA);
    delay(500);
    if(strcmp(ESPWiFiManager::ipType, "dhcp")!= 0){
        if (!WiFi.config(ESPWiFiManager::ipAddress, ESPWiFiManager::gateway, ESPWiFiManager::sbnetMask, ESPWiFiManager::primaryDns, ESPWiFiManager::seconderyDns)) {
          Serial.println("STA Failed to reconfigure");
          ESP.restart();
        }
    }
    WiFi.begin(ESPWiFiManager::ssid, ESPWiFiManager::password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED){
      Serial.println("Failed to connect saved network!");
      createSoftAp();
    } else {
      WiFi.setAutoReconnect(true);
      WiFi.persistent(true);
    }
  } else {
    createSoftAp();
  }
  ESPWiFiManager::server.reset(new ESP8266WebServer(8080));
  initHttpServer();
}

void ESPWiFiManager::setNodeName(char* nodename){
  strcpy(ESPWiFiManager::nodeName, nodename);
}

void ESPWiFiManager::handleHttpRequest(){
  server->handleClient();
}
