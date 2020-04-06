#include <ESP8266WiFi.h>
#include <Wire.h>
#include <MQTT.h>


//TODO kconfig make menuconfig for easy configurAtion
#define CONFIG_WIFI_SSID "UPC9D3FAC8"
#define CONFIG_WIFI_PASS "Nwca7njhvnQs"

IPAddress CONFIG_BROKER_IP(192, 168, 0, 66);
int CONFIG_BROKER_PORT = 1883;
IPAddress CONFIG_MACHINE_IP(192, 168, 0, 232);
IPAddress CONFIG_GATEWAY(192, 168, 0, 1);
IPAddress CONFIG_SUBNET(255,255,255,0);

int MQTT_LED = 2;

WiFiClient net;
MQTTClient mqtt;


//---------------------------i2c variables----------------------
#define ESP8266 0
#define ARDUINO 1

#define BUFFER_SIZE 64
#define COMMAND_SUB_BUFFER BUFFER_SIZE-16

uint8_t _Buffer[BUFFER_SIZE];
enum i2cCommands:uint8_t{
  PING=1,
  SETSTATUS,
  GPIO_READ,
  GPIO_WRITE
};
//------status------
enum Status:uint8_t{
  READY=1,
  SENSORERR,
  WAITING
  //spinning usw.
  
};
//non volatile addresses
int _state = 16;

Status should_status;
Status currentStatus;

uint8_t setStatus(uint8_t stat) {
  should_status = (Status)stat;
  LOG("Setting Status");
  LOG(String(should_status).c_str());
  uint8_t * command = (uint8_t*)malloc(2);
  * command = (uint8_t)SETSTATUS;
  *(command + 1) = (uint8_t)should_status;
  dialogArduino(command,2,1);
  Status ret = (Status)_Buffer[COMMAND_SUB_BUFFER];
  currentStatus = ret;
  if (should_status != currentStatus) return -1;
  clear_buffer((uint8_t*)(&_Buffer + currentStatus), 1);
  return currentStatus;
}

//-----------log----------------------
int start_time = millis();
void LOG(const char * msg, int level) {
  //TODO add sd card
  String lg ="[" + String(millis()-start_time) + "][" + String(level) + "][" + String(currentStatus) +"]" + msg;
  //all logs are printed to serial, also saved.. TODO
  Serial.println(lg);
  if (level <= 1) 
    publishtoServer(lg.c_str());
}

void LOG(const char *msg) {
  LOG(msg, 4);
}

#define NOSENSORS 4

 


//-------------------i2c commands-----------------
void clear_buffer(uint8_t * buff, int len) {
  uint8_t *ptr = buff;
  for (ptr; (ptr - buff) < len; ptr++) *ptr = -1;
};

void send_i2c(uint8_t * buff, int len) {
  LOG(("Sending " + String(len) + " bytes").c_str());
  Wire.beginTransmission(ARDUINO);
  for (int i=0; i < len; i++) {
    Wire.write(*(buff + i));
    delay(15);
  }
  Wire.endTransmission();
  LOG("End of send");
  
  delay(500);
  clear_buffer(buff, len);
};

int recv_i2c(uint8_t * buff, int port, int howMany) {
  uint8_t * ptr = buff;
  
  if (howMany){  
    Wire.requestFrom(port, howMany);
  }  
  delay(100);
  LOG("Receiving");
  while (Wire.available()) {    
    *ptr = Wire.read();
    ptr++;
  }
  LOG(("Received " + String(ptr-buff) + " Bytes").c_str());
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
  LOG("Pinging Arduino");
  uint8_t command = (uint8_t)PING;
  dialogArduino(&command,1,1);
  uint8_t ret = (uint8_t)_Buffer[COMMAND_SUB_BUFFER];
  clear_buffer((uint8_t*)(&_Buffer + COMMAND_SUB_BUFFER), 1);
  return ret;
}



uint8_t arduinoLight(bool _on) {
  uint8_t * command;
  command = (uint8_t*)malloc(4);
  *command = (uint8_t)GPIO_WRITE;
  *(command + 1) = (uint8_t) 'd';
  *(command + 2) = 13;
  *(command + 3) = 0;
  dialogArduino(command, 4,1);
  uint8_t ret = _Buffer[0];
  
  clear_buffer((uint8_t*)&_Buffer, 1);
  return ret;
}


//-------------------mqtt--------------------------------------
enum MQTTCommands:uint8_t {
  SET_STATUS=1,
  GPIOREAD,
  GPIOWRITE,
  ENDING
};
void connect() {
  LOG("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  LOG("connecting to mqtt server...");
  while (!mqtt.connect("ESP", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }

  LOG("connected to mqtt!");

  mqtt.subscribe("/Commands");
  digitalWrite(MQTT_LED, 1);
}
void publishtoServer(const char * str){
  digitalWrite(BUILTIN_LED, 0);
  mqtt.publish("/Data", str);
  digitalWrite(BUILTIN_LED, 1);
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
  while((int)*payload != 59 && (int)*payload != 0) {
    
    //ascii --> 59 is ';' , 0 is '\0' 

    ret *= 10;
    ret += (uint8_t)*payload - 48;
    payload++;
    if ((int)*payload == 59 || (int)*payload == 0) break;
  }

  return ret;
  
}
 //--------------------mqtt callback------------------
void messageReceived(String &topic, String &pyld) {
  LOG("incoming ");
  LOG("topic: ");
  LOG(topic.c_str());
  LOG("payload");
  LOG(pyld.c_str());
  
  payload = (char*)pyld.c_str();
  uint8_t first_byte = getArgsfromPayload();
  switch (first_byte) {
    case SET_STATUS:
      setStatus((uint8_t)*payload-48);
      break;
    case GPIOREAD:{
      
      uint8_t *command = (uint8_t*)malloc(4);
      
      *command = (uint8_t)GPIO_READ;
      *(command + 1) = getArgsfromPayload();
      *(command + 2) = getArgsfromPayload();
      *(command + 3) = getArgsfromPayload();
      if (*command == 255 || *(command + 1) == 255 || *(command + 2) == 255 ||*(command + 3) == 255) {
        LOG("please give enough arguments",1);
        return; 
      }
      dialogArduino(command, 4,1);
      if (payload[1] == 'd')
        LOG(String(_Buffer[0]).c_str(),1);
      else{
        char * ptr = (char*)malloc(8);
        sprintf(ptr, "%f" ,(float*)&_Buffer);
        LOG(ptr, 1);
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
        LOG("please give enough arguments",1);
        return; 
      }
      dialogArduino(command, 4,1);
      LOG(String(_Buffer[0]).c_str(), 1);
      clear_buffer((uint8_t*)&_Buffer, 1);
      break;
    }
    case ENDING:
      setStatus(WAITING);
      break;
  }
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
  if (_stat == SENSORERR) mqtt.publish("/Data", "SENSOR ERROR");;
  ptr++;
  //read the sensor floats to buffer
  float readings[NOSENSORS];
  for (int i; i < NOSENSORS; i++){
    memcpy((float *)&readings[i], ptr, sizeof(readings[0]));
    ptr += sizeof(readings[0]);
  }

  //allocate 1 space for the status, 1 for the termiating char, and 9 for floats and trailing spaces
  const char * post = (const char *)malloc(2 + (9 * NOSENSORS));
  //print them to the memory
  ptr = (uint8_t*)post;
  * ptr = (uint8_t)*String(_stat).c_str();
  ptr++;
  *ptr = (uint8_t) ' ';
  ptr++;
  for (int i=0; i < NOSENSORS; i++) {
    
    sprintf((char*)ptr, "%f ", readings[i]);
    ptr += 9;
  }
  LOG(post,1);
  
}


//-------------main---------
auto IPAddress2String = [](const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
};
void connectWiFi() {
  WiFi.config(CONFIG_MACHINE_IP, CONFIG_GATEWAY, CONFIG_SUBNET);
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);
  LOG("Connecting WiFi",2);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  LOG("WiFi connected",2);
  LOG("IP address: ",2);
  LOG(IPAddress2String(WiFi.localIP()).c_str(),2);
}
void connectArduino() {
    while((int)pingArduino()!=1) delay(100);
    while (should_status != setStatus(should_status)) delay(100);
    LOG("Arduino Connected",2);
    arduinoLight(true);
}
void setup() {/*
  EEPROM.begin(512);
  if (EEPROM.read(_state) != 255){
    
    should_status = (Status)EEPROM.read(_state);
  }else  save to sd card*/
  should_status = READY;
  Serial.begin(9600);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(MQTT_LED, OUTPUT);  
  Wire.begin();
  
  connectArduino();

  //connect to wifi
  connectWiFi();
  
  mqtt.begin(CONFIG_BROKER_IP.toString().c_str(), net);
  mqtt.onMessage(messageReceived);

  connect();
  
}

void loop() {
  while (WiFi.status() != WL_CONNECTED) connectWiFi();
  mqtt.loop();
  i2c_callback();
  delay(10);
  if ((int)pingArduino() != 1) {
    arduinoLight(false);
    LOG("Arduino disconnected",2);
    connectArduino();
  }
  
  if (!mqtt.connected()){
    digitalWrite(MQTT_LED, 0);
    connect();
  }
  delay(5000);
}
