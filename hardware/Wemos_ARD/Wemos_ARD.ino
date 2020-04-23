#include <ESP8266WiFi.h>
#include "config.h"

int MQTT_LED = 2;

WiFiClient net;



//---------------------------i2c variables----------------------
#define ESP8266                   0
#define ARDUINO                   1

#define BUFFER_SIZE               64
#define SUB_BUFFER_SIZE           16
#define COMMAND_SUB_BUFFER        BUFFER_SIZE - SUB_BUFFER_SIZE

uint8_t _Buffer[BUFFER_SIZE];

Status should_status;
Status currentStatus;

//-----------i2c funcs---------------
#include <Wire.h>
//all for ardunio communication


void clear_buffer(uint8_t * buff, int len) {
  uint8_t *ptr = buff;
  for (ptr; (ptr - buff) < len; ptr++) *ptr = -1;
};

void send_i2c(uint8_t * buff, int len) {
  Wire.beginTransmission(ARDUINO);
  for (int i=0; i < len; i++) {
    Wire.write(*(buff + i));
    delay(15);
    *(buff + i) = -1;
    
  }
  Wire.endTransmission();
  delay(500);
};

int recv_i2c(uint8_t * buff, int port, int howMany) {
  uint8_t * ptr = buff;

  if (howMany){
    Wire.requestFrom(port, howMany);
  }
  delay(100);
  while (Wire.available()) {
    *ptr = Wire.read();
    ptr++;
  }
  return ptr -buff;
};
void dialogArduino(uint8_t * command,int len, int expected_bytes) {
  //uses default buffer
  //future implementation may accept buffer as input
  uint8_t * ptr = command;
  for (int i=0; i < len ; i++){
    _Buffer[COMMAND_SUB_BUFFER+i] = *ptr;
    ptr++;
  }
  send_i2c((uint8_t *)&_Buffer[COMMAND_SUB_BUFFER], len);
  recv_i2c((uint8_t*)&_Buffer[COMMAND_SUB_BUFFER], ARDUINO, expected_bytes);
}
uint8_t pingArduino() {
  uint8_t command = (uint8_t)PING;
  dialogArduino(&command,1,1);
  uint8_t ret = (uint8_t)_Buffer[COMMAND_SUB_BUFFER];
  clear_buffer((uint8_t*)(&_Buffer + COMMAND_SUB_BUFFER), 1);
  return ret;
}



uint8_t arduinoLight(bool _on) {
  uint8_t * command;
  LOG("Arduino Light");
  LOG(String(_on).c_str());
  command = (uint8_t*)malloc(4);
  *command = (uint8_t)GPIO_WRITE;
  *(command + 1) = (uint8_t) 'd';
  *(command + 2) = 13;
  *(command + 3) = (uint8_t)_on;
  dialogArduino(command, 4,1);
  uint8_t ret = _Buffer[0];

  clear_buffer((uint8_t*)&_Buffer, 1);
  return ret;
}

//--------mqtt funcs-----------------
#include <MQTT.h>
MQTTClient mqtt;
//all for wifi
bool serverConnected = false;
bool connectMQTT(uint8_t tries) {
  uint8_t t = tries;
  LOG("connecting to mqtt server...");
  bool c = (bool)mqtt.connect("ESP", "try", "try");
  while (!c) {
    delay(500);
    tries--;
    c = (bool)mqtt.connect("ESP", "try", "try");
    if (tries < 1) {
      return c;
    }

  }

  LOG("connected to mqtt!");
  serverConnected = true;

  mqtt.subscribe("/Commands");
  //connection message has a qos of 2
  //this means it will reach the server at least once
  publishtoInfoServer("IBLOCK CONNECT", false, 2);
  return 1;
}



void publishtoServer(const char * topic, const char * data, bool retain, int qos){
  mqtt.publish(topic, data, retain, qos);
};


void publishtoServer(const char * str){
  publishtoServer("/Data", str, true, 0);
};

void publishtoServer(const char * str, bool retain, int qos){
  publishtoServer("/Data", str,retain,qos);
};

void publishtoErrorServer(const char * str, bool retain, int qos){
  publishtoServer("/Error", str, retain, qos);
};
void publishtoInfoServer(const char * str, bool retain, int qos){
  publishtoServer("/Info", str, retain, qos);
};

char * payload; // will be set when a message is received, easier parsing
uint8_t getArgsfromPayload(){
  if ((int)*payload == 0) return 255;

  while((int)*payload == 59 || (int)*payload ==32) payload++;
  uint8_t ret;

  if((int)*payload > 57) {
     ret = (uint8_t)*payload;
     payload++;
     return ret;
  }

  ret = (uint8_t)*payload - 48;
  payload++;

  //loop not breakin w/o if clause
  while(true) {

    //ascii --> 59 is ';' , 0 is '\0' 

    ret *= 10;
    ret += (uint8_t)*payload - 48;
    payload++;
    if ((int)*payload == 59 || (int)*payload == 0) break;
  }

  return ret;

}

                       

//-----------log----------------------

int start_time = millis();


void LOG(const char * msg, int level) {
  //TODO add sd card
  String lg ="[" + String(millis()-start_time) + "];[" + String(currentStatus) +"];[" + String(level) + "];[" + msg +"]";
  //all logs are printed to serial, also saved.. TODO
  Serial.println(lg);
  if (serverConnected) {
    if (level == 2) publishtoErrorServer(lg.c_str(), false, 1);
    else if (level == 1) publishtoServer(lg.c_str());
  }
}

void LOG(const char *msg) {
  LOG(msg, DEBUG);
}

#define NOSENSORS 4


//set pinged in the desired calback
bool pinged;
enum controller:uint8_t {
  NONE,
  USB,
  REMOTE
};
controller master = NONE;

bool mqttMsgRecv = false;
bool pingController(uint8_t tries) {
  uint8_t t = tries;
  pinged = false;
  uint8_t wifi_failed = false;
  
  while (true) { 
    if (master == NONE) {
      return false;
    }else if (master == REMOTE) {
      mqttMsgRecv = false;
      publishtoInfoServer(String(PINGR).c_str(), false, 1);
      delay(1000);
      while(true) {
        delay(500);
        t--;
        if (mqttMsgRecv) {
          t = tries;
          break;
        } else if (t < 1) {
          master = NONE;
          return 0;
        }
      }
    } else if (master == USB) {
      Serial.println('#');
      Serial.println(PINGR);
      Serial.println('*');
      delay(5000);
      checkSerial();
    }
    
    t--;
    if (pinged) return 1;
    else if (t < 1 && !pinged) {
      LOG("PING FAILED");
      master = NONE;
      return 0;
    }
  }

} 
//-----------------------Serial commands for usb----------------
void setIntfromString(int * global_int, const char * int_to_set) { 
  * global_int = 0;
  int digit_ctr = 0;
  while (* int_to_set != '\0') {
    int_to_set++;
    digit_ctr++;
  }
  do {
    * global_int += 10 * ((int)*int_to_set - 48);
    digit_ctr--;
    delay(20);
  }while (digit_ctr > 0);
    
}

void setIpAddressfromString(IPAddress * global_ip, const char * line) {
  IPAddress glob =  * global_ip;
  char * initial_ptr  = (char *)malloc(4);
  char * p = (char*)initial_ptr;
  
  for (int i=0; i<4; i++) {
    do {
    * p = * line;
    delay(20);
    p++;
    line++;
    } while (*line != '\"' || *line != '.');
    
    setIntfromString((int*)&glob[i], (const char *)initial_ptr);
    delay(20);    
    p = initial_ptr;
  }
}

String getLine(){
  String l = Serial.readStringUntil('\n');
  return l;
}

int checkSerial() {
  if (!Serial.available()) return 0;
  String line = getLine();
  LOG("Recieved from USB");
  LOG(line.c_str());
  uint8_t command = (int)line[0] - 48;
  Serial.println('^');
  Serial.println(command);
  delay(20);
  
  switch (command) {
    case SET_STAT:
      setStat((Status)((int)line[1]-48));
      break;
    case SET_IP: 
      line = getLine();
      setIpAddressfromString((IPAddress*)&CONFIG_MACHINE_IP, line.c_str());
      Serial.println("?");
      delay(50);
      line = getLine();
      setIpAddressfromString((IPAddress*)&CONFIG_GATEWAY, line.c_str());
      Serial.println("?");
      delay(50);
      line = getLine();
      setIpAddressfromString((IPAddress*)&CONFIG_SUBNET, line.c_str());
      break;
    case SET_BROKER:
      line = getLine();
      setIpAddressfromString((IPAddress*)&CONFIG_BROKER_IP, line.c_str());
      Serial.println("?");
      delay(50);
      line = getLine().c_str();
      setIntfromString(&CONFIG_BROKER_PORT, line.c_str());
      break;
    case SET_WIFI:{ 
      LOG("Setting WiFi");
      line = getLine();
      CONFIG_WIFI_SSID = line;
      Serial.println("?");
      delay(50);
      line = getLine();
      CONFIG_WIFI_PASS = line;
      break;
    }
    case SET_MODE:
      LOG("SETTING MODE");
      LOG((const char *)&line[1]);
      CONFIG_MODE = (ConnectionMode)((int)line[1]-48);
      delay(15);
      break;
      
    case PINGR:
      LOG("SERIAL PING REQUESTED");
      Serial.println(PINGA);
      delay(20);
      if (master==NONE) {
        LOG("INIT USB CONNECTION");
        master=USB;
      }
      break;
    case PINGA:
      LOG("SERIAL PING RECV");
      pinged = true;
      break;
    default:
      LOG("INVALID COMMAND", WARN);
      LOG(String(command).c_str(), WARN);
      Serial.flush();
      return 0;
  }
  Serial.println("OK");
  Serial.println('$');
  delay(2000);
  return 1;
}

uint8_t setStat(uint8_t stat) {
  should_status = (Status)stat;
  LOG("Setting Status");
  LOG(String(should_status).c_str());
  uint8_t * command = (uint8_t*)malloc(3);
  * command = (uint8_t)SETSTATUS;
  * (command + 1) = stat;
  * (command + 2) = '\0';
  dialogArduino(command,2,1);
  Status ret = (Status)_Buffer[COMMAND_SUB_BUFFER];
  while (ret != stat) {
    LOG((const char *)command);
    dialogArduino(command,2,1);
    Status ret = (Status)_Buffer[COMMAND_SUB_BUFFER];
  };
  LOG(("STATUS SET -> " + String(ret)).c_str());
  currentStatus = ret;
  if (should_status != currentStatus) return -1;
  clear_buffer((uint8_t*)&_Buffer, 1);
  return currentStatus;
}

 //--------------------mqtt callback------------------
void messageReceived(String &topic, String &pyld) {
  mqttMsgRecv = true;
  LOG("incoming ");
  LOG("topic: ");
  LOG(topic.c_str());
  LOG("payload");
  LOG(pyld.c_str());

  if (master == USB) {
    LOG("USB IS CONNECTED, MQTT COMMANDS ARE DISABLED", 2);
    return;
  }
  payload = (char*)pyld.c_str();
  uint8_t first_byte = getArgsfromPayload();
  switch (first_byte) {
    case SET_STAT:
      payload++;
      LOG(("SETTING STATUS " + String((uint8_t)*payload-48)).c_str());
      setStat((uint8_t)*payload-48);
      break;
    case GPIOREAD:{
      
      uint8_t *command = (uint8_t*)malloc(4);
      
      *command = (uint8_t)GPIO_READ;
      *(command + 1) = getArgsfromPayload();
      *(command + 2) = getArgsfromPayload();
      *(command + 3) = getArgsfromPayload();
      if (*command == 255 || *(command + 1) == 255 || *(command + 2) == 255 ||*(command + 3) == 255) {
        LOG("please give enough arguments",ERR);
        return; 
      }
      dialogArduino(command, 4,1);
      if (payload[1] == 'd')
        LOG(String(_Buffer[0]).c_str(),DATA);
      else{
        char * ptr = (char*)malloc(8);
        sprintf(ptr, "%f" ,(float*)&_Buffer);
        LOG(ptr, DATA);
      }
      clear_buffer((uint8_t*)&_Buffer, 1);
      break;
    }
      
    case GPIOWRITE:{
      uint8_t*command = (uint8_t*)malloc(4);
      *command = (uint8_t)GPIO_WRITE;
      *(command + 1) = getArgsfromPayload();
      *(command + 2) = getArgsfromPayload();
      *(command + 3) = getArgsfromPayload();
      if (*command == 255 || *(command + 1) == 255 || *(command + 2) == 255 ||*(command + 3) == 255) {
        LOG("Not enough args received",ERR);
        return; 
      }
      dialogArduino(command, 4,1);
      LOG(String(_Buffer[0]).c_str(), DATA);
      clear_buffer((uint8_t*)&_Buffer, 1);
      break;
    }
    case PINGR:{
      LOG("MQTT PING RECEIVED");
      publishtoInfoServer(String(PINGA).c_str(), 0, 0);
      break;
    }
    case PINGA:
      LOG("PINGED FROM MQTT SERVER");
      if (master == NONE) {
        master = REMOTE;
        break;
      }
      pinged = true;
      break;
    default:
      LOG("UNRECOGNIZED COMMAND", 2);
      LOG(payload,2);
  }
  Serial.flush();
  payload -= pyld.length();


  
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}


//--------------i2ccallback---------------
void i2c_callback() {
  Wire.requestFrom(ARDUINO, 1 + sizeof(float) * NOSENSORS);
  uint8_t *ptr = (uint8_t*)&_Buffer;
  recv_i2c(ptr, ARDUINO, 0);
  uint8_t _stat = *ptr;
  if (_stat == SENSORERR) publishtoErrorServer("FAT2", true, 1);;
  ptr++;
  //read the sensor floats to buffer
  float readings[NOSENSORS];
  for (int i; i < NOSENSORS; i++){
    memcpy((float *)&readings[i], ptr, sizeof(readings[0]));
    ptr += sizeof(readings[0]);
  }

  //allocate 1 space for the status, 1 for the termiating char, and 11 for floats and trailing spaces
  const char * post = (const char *)malloc(2 + (9 * NOSENSORS));
  //print them to the memory
  ptr = (uint8_t*)post;
  for (int i=0; i < NOSENSORS-1; i++) {
    sprintf((char*)ptr, "%03.6f, ", readings[i]);
    ptr += 9;
    if ((int)readings[i] > 10) ptr++;
    if ((int)readings[i] > 100) ptr++;
    
  }
  sprintf((char*)ptr, "%03.6f", readings[NOSENSORS-1]);
  LOG(post,DATA);
 
  
}


//-------------main---------
auto IPAddress2String = [](const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
};

bool connectWiFi(int max_tries) {
  WiFi.config(CONFIG_MACHINE_IP, CONFIG_GATEWAY, CONFIG_SUBNET);
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);
  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tries ++;
    if (tries > max_tries) {
	    LOG("WIFI CONNECTION FAILED", 2);
	    LOG("ERR 2: MAX TRIES", 2);
	    return false;
    }
  }
  LOG("IP address: ");
  LOG(IPAddress2String(WiFi.localIP()).c_str());
  return true;
}

void connectArduino() {
    int tries = 0;
    while((int)pingArduino()!=1){
      delay(100);
      tries++;
      if (tries > 50) {
        tries = 0;
        LOG("FAT1 ARDUINO NOT FOUND", ERR);
        LOG("Trying again",  ERR);
      }
    }
    while (should_status != setStat(should_status)) delay(100);
    arduinoLight(true);
};


bool connectController() {
  checkSerial();
  if (CONFIG_MODE=MQTT) {
    //connect to wifi
    LOG("Connecting WiFi");
    if (WiFi.status() != WL_CONNECTED && !connectWiFi(50)) { 
      digitalWrite(BUILTIN_LED, LOW);
      LOG("WiFi Connection failed", WARN);
    } else if (!serverConnected) {
      mqtt.begin(CONFIG_BROKER_IP.toString().c_str(), net);
      mqtt.onMessage(messageReceived);
    
      if (!connectMQTT(5)){
        LOG("ERR 3, NO SERVER FOUND", ERR);  
        digitalWrite(BUILTIN_LED, LOW);
      }
    }
  }  
  if (!pingController(10)){
    return 0;
  } else return 1;
}

void setup() {
  Serial.begin(9600);
  pinMode(BUILTIN_LED, OUTPUT);
  if (CONFIG_MODE == MQTT) digitalWrite(BUILTIN_LED, HIGH);
  else digitalWrite(BUILTIN_LED, LOW);
  Wire.begin();
  
  should_status = INIT;
  connectArduino();
  
  connectController();
}

void loop() {
  checkSerial();
  if (!pingController(10)){
    LOG("Controller Not Found", WARN);
    if (!connectController()) {
      LOG("Setting init status", WARN);
      should_status=INIT;
      setStat(should_status);
    }
  }
  i2c_callback();
  delay(10);
  if ((int)pingArduino() != 1) {
    arduinoLight(false);
    LOG("Arduino disconnected");
    connectArduino();
  }  
  delay(3000);

  if (master != NONE) {
     //loop
    delay(1);
  }
}
