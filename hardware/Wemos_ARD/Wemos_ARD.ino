#include <ESP8266WiFi.h>
#include "config.h"
#include <Adafruit_MCP9808.h>


int MQTT_LED = 2;
int RESET_PIN = 16;

WiFiClient net;



//---------------------------i2c variables----------------------
#define ESP8266                   0
#define ARDUINO                   1
#define MCPT                      2

#define BUFFER_SIZE               64
#define SUB_BUFFER_SIZE           16
#define COMMAND_SUB_BUFFER        BUFFER_SIZE - SUB_BUFFER_SIZE

uint8_t _Buffer[BUFFER_SIZE];

Status should_status;
Status currentStatus;

//-----------i2c funcs---------------
#include <Wire.h>
//all for ardunio communication

void reset_arduino() {
  LOG("RESETTING ARD", WARN);
  digitalWrite(RESET_PIN, HIGH);
  delay(200);
  digitalWrite(RESET_PIN, HIGH);
}
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
  int tries = 5;
  while (true) {
    if (tries < 1) {
      reset_arduino();
      LOG("Arduino Connection Failed", ERR);
      return -1;
    }
    if (Wire.available())
      break;
    else {
      delay(300);
      tries--;
    }
  }
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
  if (!recv_i2c((uint8_t*)&_Buffer[COMMAND_SUB_BUFFER], ARDUINO, expected_bytes)) setStat(INIT);
}
uint8_t pingArduino() {
  uint8_t command = (uint8_t)PING;
  dialogArduino(&command,1,1);
  uint8_t ret = (uint8_t)_Buffer[COMMAND_SUB_BUFFER];
  clear_buffer((uint8_t*)(&_Buffer + COMMAND_SUB_BUFFER), 1);
  if (ret != 1) should_status = INIT;
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
  if (mqtt.connected()) return mqtt.connected();
  else serverConnected = false;

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


  mqtt.subscribe("/Commands/#");

  publishtoInfoServer("IBLOCK CONNECT", true, 1);
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
  while((int)*payload != 59) {

    //ascii --> 59 is ';' , 0 is '\0'

    ret *= 10;
    ret += (uint8_t)*payload - 48;
    payload++;
    if ((int)*payload == 59 || (int)*payload == 0) break;
  }

  return ret;

}



//-----------log commands-------------------
#include <SPI.h>
#include <SD.h>
const int chip_select = D8;
Status getStatusfromSD(){
  if (SD.exists("/Status.txt")) {
    File _s = SD.open("/Status.txt");
    int _state = _s.read();
    _s.close();
    return (Status)_state;
  } else 
    return READY;
};

void setStatusfromSD() {
  should_status = getStatusfromSD();
  setStat(should_status);
}

void saveStatus() {
  if (!SD.exists("/Status.txt")) {
     SD.mkdir("/Status.txt");
  }
  File _s = SD.open("Status.txt");
  _s.print('\b');
  _s.print(should_status);
}

int start_time = millis();

void LOG(const char * msg, int level) {
  //TODO add sd card
  String lg ="[" + String(millis()-start_time) + "];[" + String(currentStatus) +"];[" + String(level) + "];[" + msg +"]";
  //all logs are printed to serial, also saved.. TODO
  Serial.println(lg);
  File logger = SD.open("/log.txt", FILE_WRITE);
  logger.println(lg);
  logger.close();
  
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
  if (mqtt.connected() && master == REMOTE) return 1;
  if (master == NONE) {
      return false;
    }
  t = tries;
  while (true) {
  if (master == USB) {
      Serial.println('#');
      Serial.println(PINGR);
      Serial.println('*');
      delay(3000);
      checkSerial();
    }


    t--;
    if (pinged) return 1;
    else if (t < 1 && !pinged) {
      LOG("PING FAILED");
      if (serverConnected) master = REMOTE;
      else master = NONE;
      return 0;
    }
  }

}
//-----------------------Serial commands for usb----------------
void setIntfromString(int * global_int, const char * int_to_set, int len) {
  * global_int = getIntFromString(int_to_set, len);
}
int getIntFromString(const char *int_string, int len) {
  int ret = ((int)*int_string - 48);
  len--;
  int_string++;
  while (len > 0) {
    ret = 10 * ret;
    ret += ((int)*int_string - 48);
    
    int_string++;
    len--;
    delay(20);
  }
  return ret;
}

void setIpAddressfromString(IPAddress * global_ip, const char * line) {
  IPAddress glob =  * global_ip;
  LOG((String(glob[1]) + String(glob[2]) + String(glob[3]) + String(glob[4])).c_str()); 
  const char * l = line;
  char * initial_ptr  = (char *)malloc(4);
  char * p = (char*)initial_ptr;
  memset(p, 0, 4);

  int len = 0;

  for (int i=0; i<4; i++) {
    
    while (true) {
    * p = * l;
    p++;
    len++;
    l++;
    if (*l == '%' || *l == '.') {
      l++;

      break;
    }
    }
    
    glob[i] = getIntFromString((const char *)initial_ptr, len);
    delay(20);
    p = (char*)initial_ptr;
    memset(p, 0, 4);
    len = 0;
  }
}

String getLine(){
  String l;
  int start = millis();
  while (!Serial.available()) {
    delay(100);
  }
  do {
    l = Serial.readStringUntil('\n');
  } while(l.length() == 0);
  if (l == "\n") return getLine();
  return l;
}

int checkSerial() {
  if (!Serial.available()) return 0;
  String line = getLine();
  if (line[0] == '#'){
    do {
      line = getLine();
    } while(line[0] != '*');
    return 0;
  }
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
      Serial.println("?");
      delay(50);

      line = getLine();
      LOG(line.c_str());
      setIpAddressfromString((IPAddress*)&CONFIG_MACHINE_IP, line.c_str());
      Serial.println('^');
      Serial.println("?");
      delay(50);
      line = getLine();
      setIpAddressfromString((IPAddress*)&CONFIG_GATEWAY, line.c_str());
      Serial.println('^');
      Serial.println("?");
      delay(50);
      line = getLine();
      setIpAddressfromString((IPAddress*)&CONFIG_SUBNET, line.c_str());
      connectWiFi(5);
      Serial.println('^');
      break;
    case SET_BROKER:
      if (mqtt.connected()) mqtt.disconnect();
      Serial.println("?");
      delay(50);
      line = getLine();
      setIpAddressfromString((IPAddress*)&CONFIG_BROKER_IP, line.c_str());
      Serial.println('^');
      Serial.println("?");
      delay(50);
      line = getLine();
      setIntfromString(&CONFIG_BROKER_PORT, line.c_str(), 4);
      connectMQTT(5);
      Serial.println('^');
      break;
    case SET_WIFI:{
      LOG("Setting WiFi");
      Serial.println("?");
      delay(50);
      line = getLine();
      CONFIG_WIFI_SSID = line;
      Serial.println('^');
      Serial.println("?");
      delay(50);
      line = getLine();
      CONFIG_WIFI_PASS = line;
      connectWiFi(5);
      Serial.println('^');
      break;
    }
    case PINGR:
      LOG("SERIAL PING REQUESTED");
      Serial.println(PINGA);
      delay(20);
      if (master!=USB) {
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
      delay(15);
      Serial.println(INVALID);
      Serial.println("$");
      return 0;
  }
  Serial.println("OK");
  Serial.println('$');
  delay(2000);
  return 1;
}

uint8_t setStat(uint8_t stat) {
  should_status = (Status)stat;
  
  uint8_t * command = (uint8_t*)malloc(3);
  * command = (uint8_t)SETSTATUS;
  * (command + 1) = stat;
  * (command + 2) = '\0';
  dialogArduino(command,2,1);
  
  Status ret = (Status)_Buffer[COMMAND_SUB_BUFFER];
  
  while (ret != stat) {
    dialogArduino(command,2,1);
    ret = (Status)_Buffer[COMMAND_SUB_BUFFER];
  };
  
  LOG(("Status set to " + String(ret)).c_str());
  currentStatus = ret;
  saveStatus();
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

  if (topic.indexOf("Status") != -1) {
    if (pyld.indexOf("Online") != -1){
      if (master == NONE) {
          master = REMOTE;
          return;
        }
        return;
    }

    else if (master == REMOTE){
      master = NONE;
      return;
    }
  }
  payload = (char*)pyld.c_str();
  uint8_t first_byte = getArgsfromPayload();
  switch (first_byte) {
    case SET_STAT:
      payload++;
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

      pinged = true;
      break;
    default:
      publishtoInfoServer(String(INVALID).c_str(), 0, 0);
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
Adafruit_MCP9808 ambient = Adafruit_MCP9808();
void i2c_callback() {
  Wire.requestFrom(ARDUINO, 1 + sizeof(float) * NOSENSORS);
  uint8_t *ptr = (uint8_t*)&_Buffer;
  if (!recv_i2c(ptr, ARDUINO, 0)) {
    setStat(INIT);
    return;
  }
  uint8_t _stat = *ptr;
  ptr++;
  //read the sensor floats to buffer
  float readings[NOSENSORS];
  readings[0] = ambient.readTempC();
  
  for (int i =1; i < NOSENSORS; i++){
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
    if ((int)readings[i] >= 10) ptr++;
    if ((int)readings[i] >= 100) ptr++;

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

bool arduinoConnected = false;

bool connectArduino() {
    int tries = 0;
    while((int)pingArduino()!=1){
      delay(100);
      tries++;
      if (tries > 50) {
        tries = 0;
        arduinoConnected = false;
        LOG("FAT1 ARDUINO NOT FOUND", ERR);
        return false;
      }
    }
    arduinoConnected = true;
    arduinoLight(true);
    return true;
};


bool connectController() {
  checkSerial();
  if (CONFIG_MODE==MQTT) {
    //connect to wifi
    LOG("Connecting WiFi");
    if (WiFi.status() != WL_CONNECTED && !connectWiFi(50)) {
      
      digitalWrite(BUILTIN_LED, LOW);
      LOG("WiFi Connection failed", WARN);
    } else {


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

//     procedures

//run on all other states (other than ready and init) 
//as the communications stay the same
void _default(){
  if (!pingController(5)){
    LOG("Controller Not Found", WARN);
    if (!connectController()) {
      LOG("Setting READY status", WARN);
      should_status=READY;
      master = NONE;
      setStat(should_status);
    }
  }
}

void _init() {  
  if (!SD.begin(chip_select)) {
    return;
  }
  if (arduinoConnected) {
    setStatusfromSD();
    return;
  } else {
    should_status = INIT;
    //initialize hw
    while (!connectArduino()) delay(100);
    setStatusfromSD(); 
  }
}

void _ready() {
  if (master == NONE) {
    connectController();
  }else {
    _default();
  }
}



void setup() {
  Serial.begin(9600);
  ambient.begin();
  
  //TODO SET LEDs
  pinMode(BUILTIN_LED, OUTPUT);

  //TODO reset mechanism
  pinMode(RESET_PIN, OUTPUT);

  Wire.begin();
  
  mqtt.begin(CONFIG_BROKER_IP.toString().c_str(), net);
  mqtt.onMessage(messageReceived);
  mqtt.setWill("/Info", "IBLOCK DISCONNECT", true, 1);

  setStatusfromSD();
    
}
void loop() {
  mqtt.loop();
  checkSerial();

  setStat(should_status);
  
  i2c_callback();
  
  delay(10);
  
  if ((int)pingArduino() != 1) {
    setStat(INIT);
  }else if (ambient.readTempC() > 65) {
    LOG("TEMPERATURE TOO HIGH");
    setStat(INIT); 
  }else {
    delay(3000);
  }
  
  switch (should_status) {
    case INIT:
      _init();
      break;
    case READY:
      _ready();
    default:
      _default();
      break;
  }
}
