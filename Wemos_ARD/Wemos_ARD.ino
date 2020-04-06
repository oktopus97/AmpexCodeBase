#include <ESP8266WiFi.h>
#include <Wire.h>
#include <MQTT.h>

//TODO kconfig make menuconfig for easy configuration
#define CONFIG_WIFI_SSID "UPC9D3FAC8"
#define CONFIG_WIFI_PASS "Nwca7njhvnQs"

IPAddress CONFIG_BROKER_IP(192, 168, 0, 66);
int CONFIG_BROKER_PORT = 1883;
IPAddress CONFIG_MACHINE_IP(192, 168, 0, 232);
IPAddress CONFIG_GATEWAY(192, 168, 0, 1);
IPAddress CONFIG_SUBNET(255,255,255,0);

int DEBUG_LED = 2;

WiFiClient net;
MQTTClient mqtt;

#define ESP8266 0
#define ARDUINO 1
enum i2cCommands:uint8_t{
  CHECKSTATUS,
};

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!mqtt.connect("ESP", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  mqtt.subscribe("/Commands");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

int checkArduino() {
  Wire.beginTransmission(ARDUINO);
  Wire.write(CHECKSTATUS);
  Wire.endTransmission();
  delay(500);
  Wire.requestFrom(ARDUINO, 2);
  if((uint8_t)Wire.read() != CHECKSTATUS) return -1;
  return (int)Wire.read();
}

void setup() {
  Serial.begin(9600);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(DEBUG_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);

  Wire.begin(ESP8266);
  Serial.println("Checking Arduino");
  while (checkArduino() != 1) {
    Serial.write('.');
    delay(100);
  }
  Serial.println("Arduino Connected");

  //connect to wifi
  WiFi.config(CONFIG_MACHINE_IP, CONFIG_GATEWAY, CONFIG_SUBNET);
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.println(""); 
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  mqtt.begin(CONFIG_BROKER_IP.toString().c_str(), net);
  mqtt.onMessage(messageReceived);

  connect();
}

void loop() {
  mqtt.loop();
  delay(10);
  
  if (!mqtt.connected()) connect();
  digitalWrite(BUILTIN_LED, HIGH);
  delay(5000);
  digitalWrite(BUILTIN_LED, LOW);
  
  // put your main code here, to run repeatedly:
  mqtt.publish("/Data", "lololo");
}
