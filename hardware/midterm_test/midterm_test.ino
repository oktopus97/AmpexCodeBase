#define DEBUG 
#include "PubSubClient.h"

String WIFI_SSID = "UPC9D3FAC8";
String WIFI_PASS = "Nwca7njhvnQs";
IPAddress IBOSS_IP(192, 168, 0, 180);
IPAddress BROKER_IP(192, 168, 0,66);

IPAddress SUBNET(255, 255, 255, 0);
IPAddress GATEWAY(192,168, 1, 1);

int BROKER_PORT = 8883;

EspClient * wifi_client;
PubSubClient * mqtt_client = new PubSubClient(*wifi_client);


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("MESSAGE ARRIVED");
  Serial.println(topic);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
}

void reconnect() {
  // Loop until we're reconnected to AP
  while (wifi_client->status() > 4){
    delay(1);
  }
  while (!mqtt_client->connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "P08AmpexIBoss";
    // Attempt to connect
    if (mqtt_client->connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt_client->publish("Status", "moin");
      // ... and resubscribe
      mqtt_client->subscribe("Server");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client->state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {

  pinMode(13, OUTPUT);
  Serial.begin(9600);

  Serial.println("Setting Up");
  Serial.println("Waiting for signal from esp");
  delay(150);
  digitalWrite(13, LOW);

  wifi_client->start_esp();
  Serial.println("Esp Connected");
  delay(250);

  //add to your routine if neccessary
  //Serial.println("Restoring esp");
  //Serial.println(ask_serial((char*)&wifi_client->EspBuffer,"AT+RESTORE", true));
  
  while (!wifi_client->set_local_ip(IBOSS_IP, SUBNET, GATEWAY))
    delay(1000);
      
  while (wifi_client->connect_to_wifi(WIFI_SSID, WIFI_PASS))
    delay(1000); 
    
  
  digitalWrite(13, HIGH);
  Serial.println("Connected to access point");
    
  mqtt_client->setCallback(callback);
  mqtt_client->setServer(BROKER_IP, BROKER_PORT);
  
  reconnect();
}

void loop() {
  Serial.println("loop");
  delay(1000);
  if (!mqtt_client->connected()) {
    reconnect();
  }
  mqtt_client->loop();
  mqtt_client->publish("IBoss", "1.1324, 2.2334, 3.2131, 2.5674, 14.1545, 15.2342, 1.1324, 2.2334, 3.2131, 2.5674, 14.1545, 15.2342, 14.1545, 15.2342");
}
