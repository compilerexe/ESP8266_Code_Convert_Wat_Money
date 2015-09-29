#define DEBUG_MODE

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <MqttWrapper.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <EEPROM.h>

const char* ssid     = "NAT.WRTNODE";
const char* pass = "devicenetwork";

int  addr = 0; // Address EEPROM
byte read_EEPROM;

float W3S=0;
int swt=0;
double watt_now;
float total;

MqttWrapper *mqtt;

void connect_wifi()
{
    WiFi.begin(ssid, pass);

    int retries = 0;
    while ((WiFi.status() != WL_CONNECTED))
    {
        Serial.print(".");
        retries++;
        delay(500);
    }

    Serial.println("WIFI CONNECTED ");
}

void reconnect_wifi_if_link_down() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WIFI DISCONNECTED");
        connect_wifi();
    }
}


void callback(const MQTT::Publish& pub) {
    if (pub.payload_string() == "1") {
        Serial.print(" => ");
        Serial.println(pub.payload_string());
        digitalWrite(14, HIGH);
        swt=1;
    }
    else if(pub.payload_string() == "99999") {
        Serial.print(" => ");
        Serial.println(pub.payload_string());
        digitalWrite(14, LOW);
        swt = 0;
        W3S = 0;
        total = 0;
        
    }
    else {
        Serial.print(pub.topic());
        Serial.print(" => ");
        Serial.println(pub.payload_string());
    }
}

void hook_prepare_data(JsonObject** root) {
  JsonObject& data = (*(*root))["d"];
  Watts();
  data["myName"] = "ESP8266-WATT";
  data["money"] = total;;
  data["watt_now"] = watt_now;

}

void setup() {
    Serial.begin(115200);
    pinMode(0, INPUT_PULLUP);
    pinMode(14, OUTPUT);
    
    delay(10);
    Serial.println();
    Serial.println();

    connect_wifi();
    mqtt = new MqttWrapper("192.168.9.1");
//    mqtt = new MqttWrapper("128.199.104.122");
    mqtt->connect(callback);
    mqtt->set_prepare_data_hook(hook_prepare_data);

    
    
}
void Watts (){
      double sum;
      
    for(int i=0;i<500;i++){
    int sensorValue = analogRead(A0)-555;
    if(sensorValue<0)
    sensorValue=-sensorValue;
     sum=sum+sensorValue;
     delay(1);
     }
//     sum=(sum/500)-4;
//     Serial.println(sum);
     sum=sum/500;
     sum=sum*0.031*220;
     Serial.println(sum);
     if(swt==1){
       watt_now = sum - 160;
       sum = sum /(3600/3);
       
       W3S=W3S+sum;
       
       total = W3S;
       
       total = (W3S / 1000) * 8;
       Serial.print("WATT_NOW = ");
       Serial.println(watt_now);
       Serial.print("total = ");
       Serial.println(total);

       if (!SPIFFS.begin()) {
          Serial.println("Failed to mount file system");
          return;
        }
      
        if (!saveConfig()) {
          Serial.println("Failed to save config");
        } else {
          Serial.println("Config saved");
        }
      
        if (!loadConfig()) {
          Serial.println("Failed to load config");
        } else {
          Serial.println("Config loaded");
        }
       
     } else {

        if (!SPIFFS.begin()) {
          Serial.println("Failed to mount file system");
          return;
        }
      
        if (!saveConfig_Clear()) {
          Serial.println("Failed to save config");
        } else {
          Serial.println("Config saved");
        }
      
        if (!loadConfig()) {
          Serial.println("Failed to load config");
        } else {
          Serial.println("Config loaded");
        }
        
     }

    

     
}

void loop() {
    reconnect_wifi_if_link_down();
    mqtt->loop();
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  const char* serverName = json["serverName"];
  const char* accessToken = json["accessToken"];

  Serial.print("Money : ");
  Serial.println(W3S);
  return true;
}

bool saveConfig() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["Money"] = W3S;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
}

bool saveConfig_Clear() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["Money"] = 0;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
}
