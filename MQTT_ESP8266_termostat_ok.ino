/* 
 * Use NodeMCU to drive DHT11 and send temperature/humidity value to MQTT server
 * Tutorial URL http://osoyoo.com/2016/11/24/use-nodemcu-to-send-temperaturehumidity-data-to-mqtt-iot-broker/
 * CopyRight John Yu
 * OLED - https://randomnerdtutorials.com/esp8266-0-96-inch-oled-display-with-arduino-ide/
 * change by Nicu FLORICA (niq_ro) to made DHT22 thermostat
 * 
 */

#include "EEPROM.h" - https://circuits4you.com/2016/12/16/esp8266-internal-eeprom-arduino/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#define DHTTYPE DHT22
#define DHTPIN  14
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 0, 2);


// Update these with values suitable for your network.
const char* ssid = "SSID";
const char* password = "PASSQWORD";
//const char* mqtt_server = "0.0.0.0";
const char* mqtt_server = "192.168.2.111";  // local server (RPi IP)

// https://github.com/ItKindaWorks/ESP8266/issues/9 - not used
const char* mqtt_username = "niq_ro";
const char* mqtt_password = "bererece";

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
float rumid;
#define releu 4 // GPIO4 = D2  // https://arduino.stackexchange.com/questions/25260/basic-question-esp8266-board-pins
#define stins LOW
#define aprins HIGH
#define buton 15   // GPIO15 = D8

void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    /*
    WiFi.mode(WIFI_STA);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
    */
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
   afisaredate();
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
   afisaredate();
  }
  // if MQTT comes a 2 message, display value on OLED
  if(p==2)
  {
  // digitalWrite(BUILTIN_LED, HIGH);
   Serial.println(" 2 - command is to display value on OLED!] ");
   afisaredate();
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
//   if (client.connect(clientId.c_str()))
   if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
     // client.subscribe("Show");
     client.subscribe("hb/command");
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
  pinMode(buton, INPUT);
  Serial.begin(115200);
  Serial.println();  
  EEPROM.begin(512);
  pinMode(releu, OUTPUT);
  digitalWrite(releu, stins);  
  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  afisaretitlu();
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
 //   rtemp = random(200,260)/10.;
 rtemp = dht.readTemperature()-1.;
 rumid = dht.readHumidity();
if (rtemp > dtemp) 
{
  incalzire = 0; 
   digitalWrite(releu, stins);
}
if (rtemp < dtemp-dt) 
{
  incalzire = 1; 
  digitalWrite(releu, aprins);
}
     lastMsg = now;
     String msg="t=";
msg = msg + rtemp;
msg = msg+"gr.C h=" ;
msg = msg + rumid;
     msg = msg+"% tset=" ;
msg= msg+ dtemp;
 msg=msg+"gr.C heater: ";
msg = msg + incalzire;
     char message[58];
     msg.toCharArray(message,80);
     Serial.println(message);
/*
display.clear();
  display.drawString(0, 0,  "Humidity: " + String(rumid) + "%\t"); 
  display.drawString(0, 16, "Temp.     : " + String(rtemp) + "C"); 
  display.drawString(0, 32, "T set        : " + String(dtemp) + "C"); 
  display.drawString(0, 48, "Heater:   " + String(incalzire)); 
  display.display();
 */    
     //publish sensor data to MQTT broker   
 client.publish("hb/_temperature1", String(rtemp).c_str());
 client.publish("hb/_temperature2", String(dtemp).c_str());
 client.publish("hb/_heater", String(incalzire).c_str());
 client.publish("hb/_humidity", String(rumid).c_str());
  }

if (digitalRead(buton) == HIGH)
{
afisaredate();
}
}

void afisaredate()
{
  display.clear();
//  display.drawString(0, 0,  "Humidity:" + String(rumid) + "%\t"); 
  display.drawString(0, 0,  "Humidity");
  if (rumid > 10.) display.drawString(74, 0,  String(rumid) + "%");
  else display.drawString(82, 0,   String(rumid) + "%");
//  display.drawString(0, 16, "Temp.     : " + String(rtemp) + "C"); 
  display.drawString(0, 16, "Temp."); 
  if (rtemp > 10.) display.drawString(74, 16, String(rtemp) + "*C"); 
  else display.drawString(82, 16, String(rtemp) + "C"); 
//  display.drawString(0, 32, "T.set       : " + String(dtemp) + "C"); 
  display.drawString(0, 32, "T.set        "); 
  if (dtemp > 10.) display.drawString(74, 32, String(dtemp) + "*C"); 
  else display.drawString(82, 32, String(dtemp) + "C"); 
//  display.drawString(0, 48, "Heater:   " + String(incalzire)); 
  display.drawString(0, 48, "Heater ="); 
  if (incalzire == 0) display.drawString(74, 48, "OFF"); 
  else display.drawString(82, 48, "ON"); 
  display.display();
  delay(5000);
display.clear();  // clear the display
display.display();
}

void afisaretitlu()
{
  display.clear();
  display.drawString(0, 0,  "MQTT Thermostat 2.0"); 
  display.drawString(5, 20, "original software"); 
  display.drawString(30, 40, "by niq_ro"); 
//  display.drawString(10, 48, "(Nicu FLORICA)"); 
  display.display();
  delay(5000);
display.clear();  // clear the display
display.display();
}
