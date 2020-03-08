#include "EspClient.h"
#include "Arduino.h"

  
String read_serial() {
 
  if (Serial2.available()) {
    String return_str;   
    while (1) {
      return_str += (char)Serial2.read();      
      if (return_str.indexOf("\n") != -1){
          Serial.println(return_str);
          break;
      }
    }
    if (return_str.indexOf("busy") != -1)
      return read_serial();
      
    return return_str;
    }
    else {
    return "";
  }
};


String ask_serial(String command){
  Serial2.println(command);
  delay(500);
  String response = read_serial();
  return response;
};

int _status = 0;

int connect_wifi_psk(String ssid, String pass) {
  String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"";
  Serial2.println(cmd);
  delay(2000);
  cmd = read_serial();
  cmd += read_serial();
  if (cmd.indexOf("DISCONNECT") != -1)
    delay(4000);
    
  while (1) { 
    cmd += read_serial();
    if (cmd.indexOf("CONNECTED") != -1){
      delay(3000);
      while(Serial2.available()) cmd += read_serial();
      if (cmd.indexOf("GOT IP") == -1) {
        return connect_wifi_psk(ssid, pass);
      }
      while (1) {
        delay(500);
        cmd += read_serial();
        if (cmd.indexOf("OK") != -1) break;
      }
      _status = 1;
      return 1;
     } else if (cmd.indexOf("OK") != -1){
      return 0;
     }
     
  }
    
};

int set_local_ip(IPAddress ip, IPAddress gateway, IPAddress netmask) {
  String str = "AT+CIPSTA_DEF=\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\"";
  str += ",\"" + String(gateway[0]) + "." + String(gateway[1]) + "." + String(gateway[2]) + "." + String(gateway[3]) + "\"";
  str += ",\"" + String(netmask[0]) + "." + String(netmask[1]) + "." + String(netmask[2]) + "." + String(netmask[3]) + "\"";

  Serial2.println(str);
  str = read_serial();
  int timer = millis();
  
  while (1) {
    str += read_serial();
    delay(500);
    if (str.indexOf("OK") != -1 || millis()- timer > 5000) break;
  };

  
  if (str.indexOf("ERROR") != -1) {
      Serial.println("1");
      while(Serial2.available()) read_serial();
      return 0;
  }
  if (str.indexOf("OK") != -1) {
    while(Serial2.available()) {
      read_serial();
      delay(1000);
    }
    return 1;
  } else
    str += read_serial();
  }

EspClient::EspClient(String ssid, String pass){
  this->ssid = ssid;
  this->pass = pass;
}
uint8_t EspClient::connect(IPAddress ip, int port) {
  Serial2.flush();
  String str = "AT+CIPSTART=\"TCP\",\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\"," + String(port);
  Serial2.println(str);
  delay(5000);
  read_serial();
  str = read_serial();
  while (1) {
    str += read_serial();
  if (str.indexOf("OK") != -1 || str.indexOf("CONNECT") != -1) { 
    delay(500);
    this->flush();
    
    _status = 2;
	  return 1;
  } else if (str.indexOf("ERROR") != -1)
    delay(500);
    this->flush();
    return 0;  
  }
}

size_t EspClient::write(uint8_t b) {
	Serial2.println("AT+CIPSEND=1");

  while(read_serial().indexOf("OK") != -1) delay(10);
  Serial2.write(">");
  Serial2.write(b);
  
  while(read_serial().indexOf("SEND") != -1) delay(10);
 return 1;
}

size_t EspClient::write(const char *str) {
	String com = "AT+CIPSEND=" + String(strlen(str));
	Serial2.println(com);
	while(read_serial().indexOf("OK") != -1) delay(10);
 Serial2.write(">");
	Serial2.println(str);
  while(read_serial().indexOf("SEND") != -1) delay(10);
  return (size_t)strlen(str);
}

size_t EspClient::write(uint8_t *buf, size_t _size) {
  String com = "AT+CIPSEND=" + String(_size);
  Serial2.println(com);
  while(read_serial().indexOf("OK") != -1) delay(10);
  delay(100);
  Serial2.write(">");
  for (int i = 0; i < _size; ++i){
    Serial2.write(buf[i]);
    delay(1);
}

    
  while(read_serial().indexOf("SEND") != -1) delay(500);
  return _size;
}

int EspClient::available() {
  if(Serial2.available()){
   return Serial2.available();}
  else
   return 0;
}

int EspClient::read() {
  if (!available())
    return -1;
  return (int)Serial2.read(); 
 }


void EspClient::flush() {
  while (available())
    read_serial();
}

void EspClient::stop() {
  if (status() == 2) {
  ask_serial("AT+CIPCLOSE");
  this->flush();
  }
}

uint8_t EspClient::connected() {
  uint8_t s = status();
  if (s == 5)
    connect_wifi_psk(this->ssid, this->pass);
  return (s < 4 && s > 1);
  }

uint8_t EspClient::status() {
    if (_status < 2) return _status;
    String status = ask_serial("AT+CIPSTATUS");
    this->flush();
    
    return (uint8_t)status[8];
}
