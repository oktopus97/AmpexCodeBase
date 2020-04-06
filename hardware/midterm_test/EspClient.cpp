#include "EspClient.h"
#include "Arduino.h"

#ifdef DEBUG
//debug stuff...
#endif

//serial commands

//for lines that terminate with \n\t
//BLOCKING!!
int read_line(char * buff) {
  
  while (!ESP.available()){
      delay(5);
  }
  buff  += 255;
  int c = 0;
  while (ESP.available()) {
    if (c == 255) break;
    while (*buff != NULL)
      buff--; 
    
       
    *buff = ESP.read();
    Serial.write((char)*buff);
    delay(15);
      
    buff--;
    c++;
   
  }
 return c;
}

void read_all(char * buff) {
  char * ptr = buff;
  while (ESP.available()) {
    read_line(buff);
    buff = ptr;
    delay(100);
  }
  return NULL;
}

void read_buffer(char * buff, int len) {
  while (!ESP.available())
    delay(5);
  buff += 255;
  for (int i = 0; i < len; i++) {
    if (*buff != NULL) {
      buff--;
      i--;
    }
    Serial.write(*buff);
    delay(15);
    *buff = ESP.read();
    buff--;
  }
}

String flush_all_to_string(char * buff){
  String str ="";
  buff += 255;
  while (*buff != NULL) {
    str= *(buff) + str;
    *buff = NULL;
    buff--;
  };
 return str;
}

String flush_buffer_to_string(char * buff, int len){
  String str ="";
  buff += 255;
  for (int i=0; i <= len; i++) {
    str= *(buff-i) + str;
    *(buff-i) = NULL;
   };
  return str;
}

String ask_serial(char * buff, String command, bool set, int del) {
  ESP.println();
  delay(15);
  for (int i; i < command.length(); i++) {
    ESP.write(command[i]);
    delay(15);
  }
  delay(del);
  char * ptr = buff;
  int c = read_line(buff);
  
  buff = ptr;
  String response = flush_all_to_string(buff);
  
  Serial.println("first Response to " + command + "\n");
  delay(100);
  Serial.println(response);
  delay(100);
  while (1) {
    
    if ((response.indexOf("ERROR") != -1 || response.indexOf("OK") != -1 
          || response.indexOf("ALREADY CONNECTED") != -1 || response.indexOf("FAIL") != -1))
      break;
    if (!set && response.indexOf("+") != -1)
      break;
    *buff = *ptr;
    c += read_line(buff);
    *buff = *ptr;
    response = flush_all_to_string(buff) + response;
  };
  Serial.println("Response to " + command + "\n");
  delay(100);
  Serial.println(response);
  delay(100);
  Serial.println("\nEnd Response");
  delay(100);
  return response;
}


uint8_t EspClient::connect_to_wifi(String ssid, String pass) {
  if (this->_status >= 2) {
    return 1;
  }
  String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + pass + "\"";
  cmd = ask_serial((char*)&this->EspBuffer, cmd, true, 4000);
  
  while (this->available())
    read_line((char*)&this->EspBuffer);
  flush_all_to_string((char*)this->EspBuffer);
  return this->connected();
  
  
};


uint8_t EspClient::set_local_ip(IPAddress ip, IPAddress gateway, IPAddress netmask) {
  if (this->status() >= 2 && ip == this->ip) return 1;
      
  String str = "AT+CIPSTA_DEF=\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\"";
  str += ",\"" + String(gateway[0]) + "." + String(gateway[1]) + "." + String(gateway[2]) + "." + String(gateway[3]) + "\"";
  str += ",\"" + String(netmask[0]) + "." + String(netmask[1]) + "." + String(netmask[2]) + "." + String(netmask[3]) + "\"";
 
 for (int i; i < str.length(); i++) {
    ESP.write(str[i]);
    delay(15);
  }
  Serial.println("sa");
  delay(200);
  read_line((char*)&this->EspBuffer);

  str = "AT+CIPSTA_CUR?";
  
  String response = ask_serial((char*)&this->EspBuffer, str, false, 1000);
  if (response.indexOf("ERROR") != -1)
    return 0;
  String num;
  for (int i =0; i<response.length();i++){
    for (int j = 0; j < 4; j++){
        while (response[i] != '\n') i++;
        i++;
        for(int j = 3; j < 0; j--){
          num = "";
          while (response[i] == '.' || response[i] == '"') {
            num = num += (char)response[i];
            i++;
          }
          if (ip[j] != num.toInt()) return 0;
        }
    }
  }
  
}

uint8_t EspClient::connect(IPAddress ip, int port) {
  String str = "AT+CIPSTART=\"TCP\",\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\"," + String(port);
  str = ask_serial(this->EspBuffer, str, true, 3000);
  if (str.indexOf("OK") != -1 || str.indexOf("ALREADY CONNECTED") != -1) return 1;
  else return 0;
}

size_t EspClient::write(uint8_t b) {
  ask_serial((char*)&this->EspBuffer,"AT+CIPSEND=1", true, 100);

  ESP.write(">");
  ESP.write(b);
  Serial.write(b);
  delay(10);

  String response;
  while (true) {
    read_line((char*)&this->EspBuffer);
    response = flush_all_to_string((char*)&this->EspBuffer);
    if (response.indexOf("SEND OK") != -1) {
      return 1;
    }else if (response.indexOf("SEND FAIL") != -1) {
      return 0;
    }else if (response.indexOf("ERROR") != -1) return -1;
  }
}


size_t EspClient::write(const char *str) {
  String com = "AT+CIPSEND=" + String(strlen(str));
  ask_serial((char*)&this->EspBuffer, com, true, 100);
  ESP.write(">");
  delay(10);
   for (int i; i < com.length(); i++) {
    ESP.write(com[i]);
    delay(15);
  }
  String response;

  while (true) {
    read_line((char*)&this->EspBuffer);
    response = flush_all_to_string((char*)&this->EspBuffer);
    if (response.indexOf("SEND OK") != -1) {
      break;
    }else if (response.indexOf("SEND FAIL") != -1) {
      return 0;
    }else if (response.indexOf("ERROR") != -1) return -1;
  }
  return (size_t)strlen(str);
}

size_t EspClient::write(uint8_t *buf, size_t _size) {
  String com = "AT+CIPSEND=" + String(_size);
  ask_serial((char*)&this->EspBuffer, com, true,100);
  ESP.write(">");
  delay(10);
  for (int i = 0; i < _size; ++i) {
    ESP.write(buf[i]);
    delay(5);
  }
  String response;
  while (true) {
    read_line((char*)&this->EspBuffer);
    response = flush_all_to_string((char*)&this->EspBuffer);
    if (response.indexOf("SEND OK") != -1) {
      return 1;
    }else if (response.indexOf("SEND FAIL") != -1) {
      return 0;
    }else if (response.indexOf("ERROR") != -1) return -1;
  }

  return _size;
}

int EspClient::available() {
  if (ESP.available()) {
    return ESP.available();
  }
  else
    return 0;
}

int EspClient::read() {
  if (!this->available())
    return -1;
  return (char)ESP.read();
}


void EspClient::flush() {
  read_all((char*)&this->EspBuffer); 
  flush_all_to_string((char*)&this->EspBuffer);
  return NULL;
}

void EspClient::stop() {
  if (status() == 2) {
    ask_serial((char*)&this->EspBuffer, "AT+CIPCLOSE", true, 1000);
    this->flush();
  }
}


uint8_t EspClient::connected() {
  uint8_t s = status();
  return (s < 4);
}

uint8_t EspClient::status() {
  String num;
  String stat = ask_serial((char*)&this->EspBuffer, "AT+CIPSTATUS", true,1000);
  
  for (int i = stat.length(); i > 0; i--){
    if (stat[i] == ','){
      for (int j = 0; j < 3; j++)
        while (stat[i] != ',') i++;
      i++;
      for(int j = 3; j < 0; j--){
        num = "";
        while (stat[i] == '.' || stat[i] == '"') {
          num = num += stat[i];
          i++;
        }
        this->ip[j] = num.toInt();
        }
      }
    }
  this->_status = (uint8_t)stat[8];
  return _status;

}

bool EspClient::ping_chip () {
  String resp = ask_serial((char*)&this->EspBuffer, "AT", true, 2000);
  if (resp.indexOf("OK") != -1) return 1;
  else return 0;
}

bool EspClient::ping (IPAddress ip) {
  if (ask_serial((char*)&this->EspBuffer, "AT+PING=\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\"", true,1000).indexOf("OK") != -1)
    return 0;
  else return 1;
}

int EspClient::start_esp () {
  ESP.begin(115200);
  
  int tries = 0;

  while (!ESP.available()) {
    delay(100);
  };
  
  while (!this->ping_chip()) delay(100);


  //calling this->flush causes termination
  read_all((char*)&this->EspBuffer); 
  return this->status();
}
