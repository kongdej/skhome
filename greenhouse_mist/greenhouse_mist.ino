// Smart Home Project - Greenhouse Misting Pump Control
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <MicroGear.h>

// Wifi Settings
const char* ssid = "ZAB";
const char* password = "Gearman1";

// NETWORK: Static IP details...
IPAddress ip(192, 168, 1, 104);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Netpie Settings
#define APPID       "SKHOME"
#define GEARKEY     "rncLwMCQ7202alB"
#define GEARSECRET  "6OynR4LdDUmnubhVxSzpJ2C02"
#define ALIAS       "greenhouse_mist"
#define FEEDID      "GREENHOUSE"

// Relay
int relayPin = D5;
unsigned long previousMillis = 0;      // will store last time LED was updated
const long interval = 15000;           // interval at which to blink (milliseconds)

/*-- Defaults setup pump paramenters ------------------------*/
uint16_t ontime = 10; // minute
uint16_t offtime = 1; // minute
uint16_t tempSp = 35;
uint16_t temperature = 0;
/*----------------------------------------------------------*/

unsigned long intervalStartPump = ontime*60*1000;      
unsigned long intervalStopPump = offtime*60*1000;
unsigned long previousMillisStartPump = 0;
unsigned long previousMillisStopPump = -intervalStopPump;
int timer = 0;

WiFiClient client;
MicroGear microgear(client);

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  char strState[msglen];
  for (int i = 0; i < msglen; i++) {
    strState[i] = (char)msg[i];
  }
  String stateStr = String(strState).substring(0,msglen);
  Serial.print(topic);Serial.print("-->");Serial.println(stateStr);
   
/*  
 *   if (String(topic) == "/SKHOME/greenhouse/mist/cmd"){
    delay(2000);
    microgear.publish("/greenhouse/mist/debug",stateStr);
  }
*/  
  
  if (String(topic) == "/SKHOME/greenhouse/mist/cmd"){
     int state = stateStr.toInt();  
     digitalWrite(relayPin,state ? HIGH:LOW);
     microgear.publish("/greenhouse/mist/status",String(state));
  }
  if (String(topic) == "/SKHOME/greenhouse/mist/getsetting"){
      microgear.publish("/greenhouse/mist/config/sp",String(tempSp));
      microgear.publish("/greenhouse/mist/config/ton",String(ontime));
      microgear.publish("/greenhouse/mist/config/toff",String(offtime));
      int pump_status = digitalRead(relayPin);
      microgear.publish("/greenhouse/mist/status",String(pump_status));
  }
  if (String(topic) == "/SKHOME/greenhouse/mist/set/sp") {
    tempSp = stateStr.toInt();
    microgear.publish("/greenhouse/mist/config/sp",String(tempSp));
  }
  if (String(topic) == "/SKHOME/greenhouse/mist/set/ton") {
    ontime = stateStr.toInt();
    microgear.publish("/greenhouse/mist/config/ton",String(ontime));
  }
  if (String(topic) == "/SKHOME/greenhouse/mist/set/toff") {
    offtime = stateStr.toInt();
    microgear.publish("/greenhouse/mist/config/toff",String(offtime));
  }
  if (String(topic) == "/SKHOME/greenhouse/ds18/t") temperature = stateStr.toInt();
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  microgear.setAlias(ALIAS);
  microgear.subscribe("/greenhouse/#");
}

void setParams() {
  intervalStartPump = ontime*60*1000;      
  intervalStopPump = offtime*60*1000;
  
  Serial.println("Save configuration to SPIFFS.");
  bool result = SPIFFS.begin();
  File f = SPIFFS.open("/config.txt", "w");
  f.print(tempSp);f.print(",");
  f.print(ontime);f.print(",");
  f.println(offtime);
  f.close();
}

void setup() {
  Serial.begin(9600);

  // NETPIE
  microgear.on(MESSAGE,onMsghandler);
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

  //SPTFFS SECTION
  bool result = SPIFFS.begin();
  Serial.println("SPIFFS opened: " + result);
  if(SPIFFS.exists("/config.txt")) {
    readdata();
  }
  else {
    savedata();
  } 
  pinMode(relayPin,OUTPUT);
  digitalWrite(relayPin,LOW);    // relay off
}

void savedata() {
  File f = SPIFFS.open("/config.txt", "w");
  if (!f) {
    Serial.println("file creation failed");
  } 
  else {
    f.print(tempSp);f.print(",");
    f.print(ontime);f.print(",");
    f.println(offtime);
  }
  f.close();
}

void readdata() {
  File f = SPIFFS.open("/config.txt", "r");
  Serial.println("Reading coniguration.");
  while(f.available()) {
    String line = f.readStringUntil('n');
    Serial.println(line);
    tempSp = getValue(line, ',', 0).toInt();
    ontime = getValue(line, ',', 1).toInt();
    offtime = getValue(line, ',', 2).toInt();
  }   
  intervalStartPump = ontime*60*1000;      
  intervalStopPump = offtime*60*1000;
  f.close();
}

String getValue(String data, char separator, int index){
    int maxIndex = data.length() - 1;
    int j = 0;
    String chunkVal = "";
    for (int i = 0; i <= maxIndex && j <= index; i++){
        chunkVal.concat(data[i]);
        if (data[i] == separator){
            j++;
            if (j > index){
                chunkVal.trim();
                return chunkVal;
            }
            chunkVal = "";
        }
        else if ((i == maxIndex) && (j < index)) {
            chunkVal = "";
            return chunkVal;
        }
    }   
}
// END SPTFFS SECTION

bool start=false;

void loop() {
  ArduinoOTA.handle();
  if (microgear.connected()) {
      microgear.loop();
      if (timer >= 1000) {
        // Process loop here...

        // Start pump
        unsigned long currentMillisPump = millis();
        if (temperature >= tempSp) {
          if (start == false) {
            if (currentMillisPump - previousMillisStopPump >= intervalStopPump) {  // delay to start
              previousMillisStartPump = currentMillisPump;
              digitalWrite(relayPin,HIGH);  // start
              start = true;
              microgear.publish("/greenhouse/mist/status","1");
            }
          }
        }
      
        // Stop pump
        currentMillisPump = millis();
        if (start == true) {
          if (currentMillisPump - previousMillisStartPump >= intervalStartPump) {  // time of pump start
            previousMillisStopPump = currentMillisPump;
            digitalWrite(relayPin,LOW); // stop
            start = false;
            microgear.publish("/greenhouse/mist/status","0");
          }
        }
        
//        int pump_status = digitalRead(relayPin);
//        microgear.publish("/greenhouse/mist/status",String(pump_status));
//        delay(500);
//        String configs = String(tempSp)+','+String(ontime)+','+String(offtime);
//        microgear.publish("/greenhouse/mist/config",configs);
/*
        delay(200);
        microgear.publish("/greenhouse/mist/config/sp",String(tempSp));
        delay(200);
        microgear.publish("/greenhouse/mist/config/ton",String(ontime));
        delay(200);
        microgear.publish("/greenhouse/mist/config/toff",String(offtime));
        delay(200);
*/
         // feed
        int pump_status = digitalRead(relayPin);
        String data = "{";
        data += " \"pump_gh_mist\":";
        data += pump_status;
        data += "}";
        microgear.writeFeed(FEEDID,data);
        delay(1000); 
        // end process loop //

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

