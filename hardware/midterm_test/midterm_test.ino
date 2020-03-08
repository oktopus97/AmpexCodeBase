#include "PubSubClient.h"

String WIFI_SSID = "o2-WLAN00";
String WIFI_PASS = "C4XVVED2UTNFN7P7";
IPAddress IBOSS_IP(192, 168, 1, 12);
IPAddress BROKER_IP(192, 168, 1, 34);

IPAddress SUBNET(255, 255, 255, 0);
IPAddress GATEWAY(192,168, 1, 1);

int BROKER_PORT = 8883;

EspClient wifi_client(WIFI_SSID, WIFI_PASS);
PubSubClient mqtt_client(wifi_client);


int led = 13;
bool on = 1;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("MESSAGE ARRIVED");
  Serial.println(topic);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  if (!on){
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
  }
}

void reconnect() {
  // Loop until we're reconnected to AP
  while (wifi_client.status() > 4){
    delay(1);
  }
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    Serial2.println("AT+PING=\"" + String(BROKER_IP[0]) + "." + String(BROKER_IP[1]) + "." + String(BROKER_IP[2]) + "." + String(BROKER_IP[3]) + "\"");
    delay(1000);
    
    while (Serial2.available()) read_serial();

    String clientId = "P08AmpexIBoss";
    // Attempt to connect
    if (mqtt_client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt_client.publish("Status", "moin");
      // ... and resubscribe
      mqtt_client.subscribe("Server");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  Serial2.begin(115200);

  digitalWrite(led, LOW);
  
  Serial.println("Setting Up");
  Serial.println("Waiting for signal from esp");
  String lines;
  while (1) {
    if(!Serial2.available()){
      Serial2.println("AT");
      delay(2500);
      continue;
    }
    lines = read_serial();
    lines += read_serial();
    if (lines.indexOf("OK") != -1)  
      break;
  }
  Serial2.println("AT+RESTORE");
  lines += read_serial();
  while (1) {
    lines += read_serial();
    if (lines.indexOf("OK") != -1) break;
  };
  while (Serial2.available()) { 
    delay(100);
    read_serial();
  }
  delay(2000);
  
  bool connection = 0;
  //single connection mode
  Serial2.println("AT+CIPMUX=0");
  lines = read_serial();
  while (1) {
    lines += read_serial();
    
    if (lines.indexOf("OK") != -1) break;
  };
  delay(1000);
  Serial2.flush();
  
  Serial2.println("AT+CWMODE=1");
  lines = read_serial();
  while (1) {
    lines += read_serial();
    delay(500);
    if (lines.indexOf("OK") != -1) break;
  };
  delay(1000);
  Serial2.flush();
  while (!connection){
    if (set_local_ip(IBOSS_IP, GATEWAY, SUBNET))
      connection = 1;
    else Serial.println("ip address set failed trying again");
    }
  Serial2.flush();
  
  connection = 0;
  while (!connection){
      if (connect_wifi_psk(WIFI_SSID, WIFI_PASS)){
        connection = 1;
      } else
      Serial.println("wifi connection failed trying again");
      
    }
  Serial2.flush();
  
  digitalWrite(led, HIGH);
  Serial.println("Connected to access point");

  
  
  mqtt_client.setServer(BROKER_IP, BROKER_PORT);
  mqtt_client.setCallback(callback);

  reconnect();
  
  pinMode(led, INPUT);
  
}

void loop() {
  Serial.println("loop");

  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();
  mqtt_client.publish("IBoss", "1.1324, 2.2334, 3.2131, 2.5674, 14.1545, 15.2342, 1.1324, 2.2334, 3.2131, 2.5674, 14.1545, 15.2342, 14.1545, 15.2342");
  

  delay(1000);
  // put your main code here, to run repeatedly:

}
