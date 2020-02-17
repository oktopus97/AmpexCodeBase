//firmware for esp8266
//to flash connect the rst pin to 5V of arduino

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <IPAddress.h>

#include <string>
#include <iostream>
#include <bits/stdc++.h> 
using namespace std; 

#include "signals.h"

WiFiClient esp;
PubSubClient client(esp);

int sendSignal(int signal){
  //will be changed
  Serial.write(signal);
  return 1;
}

int setupWiFi(String ssid, String pass, IPAddress ip, IPAddress subnet, IPAddress gateway ) {

  delay(10);
  // We start by connecting to a WiFi network

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  randomSeed(micros());
  
  WiFi.config(ip, gateway, subnet);
  return 0;
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

int setupMQTT(IPAddress mqtt_server, String clientName){ //, char * user, char * name ) {
  client.setServer(mqtt_server, 1883);
  //TODO Authenticate
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    clientName += String(random(0xffff), HEX);
    if (client.connect(clientName.c_str())){ //, user, name)) {
      // Once connected, publish an announcement...
      client.publish("status", "Connected");
      client.subscribe("commands");
      return 1;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      return 0;
    }
  }
}

IPAddress ipFromStr(String ip) {
  int components[4];
  int j = 0;
  String buff;
  for (int i = 0; i < ip.length(); ++i) {
    if (ip[i] == '.') {
      components[j] = std::atoi(buff.c_str());
      ++j;
    }else buff += ip[i];
  }
  IPAddress IP(components[0], components[1], components[2], components[3]);
  return IP;
}
  


//runtime functions
void setup() {
  pinMode(2, OUTPUT);
  Serial.begin(9600);
  client.setCallback(callback);

}

unsigned long lastMsg = millis();
int mode = 0; //wifi mode means 1 for esp

void loop() {
  String msg;
  
  int command;
  
  String ssid;
  String password;
  String ipaddr;
  String subnetmask;
  String defgateway;  

  String brokerip;
  String brokerport;
//String mqttip;
//String mqttpass;

  String data;

  String stat;
  
  //Argument parsing
  while (Serial.available()){
    char ch = Serial.read();
    if (ch == '-') {
      switch (Serial.read())
      {
        case 'm':
          //space
          Serial.read();
          ch = Serial.read();
          command =(int)ch;
          
          
        //wifi arguments
        case 's':{
          Serial.read();
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else ssid += ch;
          }
        }
        case 'p':{
          Serial.read();
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else password += ch;
          }
        }
        case 'i':{
          Serial.read();
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else ipaddr += ch;
          }
        }
        case 'S':{
          Serial.read();
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else subnetmask += ch;
          }
        }
        case 'g':{
          Serial.read();
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else defgateway += ch;
          }
        }
        
        //mqtt arguments
        case 'b':{
          Serial.read();
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else brokerip += ch;
          }
         }
        case 'P':{
          Serial.read();
          String brokerport;
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else brokerport += ch;
          }
        }
/*       Tobe implemented  
 *       case 'U':{
 *         Serial.read();
 *         while (1){
 *           ch = Serial.read();
 *           if (ch == ' ') break;
 *           else mqttuser += ch;
 *         }
 *       }
 *       case 'a':{
 *         Serial.read();
 *         while (1){
 *           ch = Serial.read();
 *           if (ch == ' ') break;
 *           else mqttpass += ch;
 *         }
 *       }
 *       
 */
        //publish arguments  
        case 'd':{
          Serial.read();
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else data += ch;
          }
        }
        //status update vars
        case 'u':{
          Serial.read();
          while (1){
            ch = Serial.read();
            if (ch == ' ') break;
            else stat += ch;
          }
        }
      }
    }
  }
  //run the commands from arduino
  switch (command){
    case CONNECTWIFI:{
      mode = 1;
      
      if (setupWiFi(ssid, password, ipFromStr(ipaddr), ipFromStr(subnetmask), ipFromStr(defgateway))){
        Serial.println("OK");
        digitalWrite(2, HIGH);}
      else Serial.println("NOT OK");
    }
    case CONNECTMQTT:
      {
      //pass and auth are not yet used
      if (setupMQTT(ipFromStr(brokerip), "IBoss ESP8266")) Serial.println("OK");
      else Serial.println("NOT OKAY");
      }
    case PUBLISH:{
      if(client.publish("data", data.c_str())) Serial.println("OK");
      else Serial.println("NOT OKAY");
    }
    case SETSTATUS:{
      if(client.publish("status", stat.c_str())) Serial.println("OK");
      else Serial.println("NOT OKAY");}
  }
  
}
