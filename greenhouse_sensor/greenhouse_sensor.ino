// Smart Home project - Greenhouse Sensors
// Written by kongdej srisamran, pudza maker club

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SHT1x.h>
#include <MicroGear.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Wifi Settings
const char* ssid     = "ZAB";
const char* password = "Gearman1";

// NETWORK: Static IP details...
IPAddress ip(192, 168, 1, 102);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Netpie Settings
#define APPID       "SKHOME"
#define GEARKEY     "KHazXa72xG6QRme"
#define GEARSECRET  "nkjB8z7PmCl1F9IjLlYQe9rs8"
#define ALIAS       "greenhouse_sensor"
#define FEEDID      "GREENHOUSE"

// define pin
#define clockPin      D1      // SHT10 CLOCK
#define dataPin       D2      // SHT10 DATA
#define ONE_WIRE_BUS  D3      // DS18B20
#define DHTPIN        D4      // DHT 21 (AM2301)
#define DHTTYPE       DHT21   // DHT 21 (AM2301)

SHT1x sht1x(dataPin, clockPin);
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
WiFiClient client;
MicroGear microgear(client);
int timer = 0;

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  microgear.setAlias(ALIAS);
}

void setup() {
  Serial.begin(9600);

  // NETPIE
  microgear.on(CONNECTED,onConnected);
  
  
  WiFi.config(ip, gateway, subnet); // Static IP Setup Info Here...
  if (WiFi.begin(ssid, password)) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    
    // OTA
    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    //uncomment the line below if you want to reset token -->
    microgear.resetToken();
    microgear.init(GEARKEY, GEARSECRET, ALIAS);
    microgear.connect(APPID);
  }

  dht.begin();
  ds18b20.begin();
}

void loop() {
  ArduinoOTA.handle();
  if (microgear.connected()) {
      microgear.loop();
      if (timer >= 1000) {

        delay(500);
        float dht_h = dht.readHumidity();
        float dht_t = dht.readTemperature();        
        if (isnan(dht_h) || isnan(dht_t)) {
          Serial.println("Failed to read from DHT sensor!");  
        }
        else {
          Serial.print("DHT Humidity: ");Serial.print(dht_h);Serial.print("%\t");
          microgear.publish("/greenhouse/dht/t",String(dht_t));
  
          delay(500);
          Serial.print("DHT Temperature: ");Serial.print(dht_t);Serial.println('C');
          microgear.publish("/greenhouse/dht/h",String(dht_h));
        }
              
        delay(500);
        ds18b20.requestTemperatures(); // Send the command to get temperatures
        float ds18b20_t = ds18b20.getTempCByIndex(0); 
        Serial.print("DS18 Temperature: ");Serial.print(ds18b20_t);Serial.println("C");
        microgear.publish("/greenhouse/ds18/t",String(ds18b20_t));
      
        delay(500);
        float sht_t = sht1x.readTemperatureC();
        float sht_h = sht1x.readHumidity();
        Serial.print("SHT Temperature: ");Serial.print(sht_t);Serial.print("C \t");
        microgear.publish("/greenhouse/sht/t",String(sht_t));

        delay(500);
        Serial.print("SHT Humidity: ");Serial.print(sht_h);Serial.println("%");
        microgear.publish("/greenhouse/sht/h",String(sht_h));
        
        Serial.println("------------------------------------");

        String data = "{";
        data += " \"dht_t\":";
        data += dht_t;
        data += ",\"dht_h\":";
        data += dht_h;
        data += ",\"sht_t\":";
        data += sht_t;
        data += ",\"sht_h\":";
        data += sht_h;
        data += ",\"ds18_t\":";
        data += ds18b20_t;
        data += "}";
        microgear.writeFeed(FEEDID,data);
        timer = 0;
      } 
      else {
        timer += 100;
      }
    }
    else {
      Serial.println("connection lost, reconnect...");
      if (timer >= 5000) {
        microgear.connect(APPID);
        timer = 0;
      }
      else {
        timer += 100;
      }
    }
    delay(100);
}
