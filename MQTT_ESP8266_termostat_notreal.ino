/* 
 * based on material from http://osoyoo.com/2016/11/24/use-nodemcu-to-send-temperaturehumidity-data-to-mqtt-iot-broker/ (CopyRight John Yu)
 * MQTT Thermostat using Mosquitto server 
 * writted by Nicu FLORICA (niq_ro) to made DHT22 thermostat
 */

#include "EEPROM.h" - https://circuits4you.com/2016/12/16/esp8266-internal-eeprom-arduino/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#define DHTTYPE DHT22
#define DHTPIN  14
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

// Update these with values suitable for your network.
const char* ssid = "yourssid";
const char* password = "yourpassword";
const char* mqtt_server = "192.168.1.105"; // local server
//const char* mqtt_server = "tehniq.go.ro";
//const char* mqtt_server = "broker.mqtt-dashboard.com";
//const char* mqtt_server = "iot.eclipse.org";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
float dtemp = 24.;
int dtemp1 = 240;
float rtemp;
int incalzire = 0;
float dt = 0.2;
float temin = 16;
float temax = 32;

void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    WiFi.mode(WIFI_STA);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Command is : [");
  Serial.print(topic);
  int p =(char)payload[0]-'0';

  // if MQTT comes a 0 message, decrease temperature
  if(p==0) 
  {
   Serial.println(" 0 - command is to decease desired temperature!]");
   Serial.print(" Desired temperature: " );
   dtemp1 = dtemp1 - 1;
   if (dtemp1 < temin*2.) dtemp1 = temin*2.;
   EEPROM.write(0, dtemp1);
   EEPROM.commit();
// EEPROM.end();
   dtemp = dtemp1/2.;
   Serial.print(dtemp);
   Serial.println(" gr.C");
  } 
  // if MQTT comes a 1 message, increase temperature
  if(p==1)
  {
  // digitalWrite(BUILTIN_LED, HIGH);
    Serial.println(" 1 - command is to increase desired temperature!] ");
   Serial.print(" Desired temperature is: " );
   dtemp1 = dtemp1 +1;
   if (dtemp1 > temax*2.) dtemp1 = temax*2.;
   EEPROM.write(0, dtemp1);
 EEPROM.commit();
 //EEPROM.end();
   dtemp = dtemp1/2.;
   Serial.print(dtemp);
   Serial.println(" gr.C");
  }
  Serial.println();
} //end callback

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to        if (client.connect(clientId,userName,passWord))
   if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
      client.subscribe("Show");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(6000);
    }
  }
} //end reconnect()

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(1000);
  dtemp1 = EEPROM.read(0);
  if ((dtemp1 > temax*2.) or (dtemp1 < temin*2.)) dtemp1 = (temin+temax);
  dtemp = dtemp1/2.;
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  // read DHT11 sensor every 6 seconds
  if (now - lastMsg > 6000) {
    rtemp = random(200,260)/10.;
if (rtemp > dtemp + dt) 
{
  incalzire = 0; 
}
if (rtemp < dtemp) 
{
  incalzire = 1; 
}
     lastMsg = now;
     String msg="real temperature: ";
//     msg= msg+ dht.readTemperature();

msg= msg+ rtemp;
     msg = msg+"gr.C ;desired temperature: " ;
//     msg=msg+dht.readHumidity() ;
msg= msg+ dtemp;
     msg=msg+"gr.C centrala ";
     msg = msg + incalzire;
     char message[58];
     msg.toCharArray(message,80);
     Serial.println(message);
     //publish sensor data to MQTT broker   
 client.publish("hb/_temperature1", String(rtemp).c_str());
 client.publish("hb/_temperature2", String(dtemp).c_str());
 client.publish("hb/_temperature3", String(incalzire).c_str());
  }
}
