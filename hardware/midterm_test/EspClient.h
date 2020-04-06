//custom wifi client to be used with at commands
#include <WiFi.h>
#define ESP Serial2
class EspClient:WiFiClient{
  private:
  IPAddress ip;
  IPAddress gateway;
  IPAddress netmask;
  public:
  int _status = 5;
  char EspBuffer[256];
  EspClient(){};
  int start_esp();
  uint8_t status();
  uint8_t connect(IPAddress, int);
  uint8_t connect_to_wifi(String, String);
  uint8_t set_local_ip(IPAddress ip, IPAddress gateway, IPAddress netmask);
  uint8_t reconnect();
  bool ping(IPAddress);
  bool ping_chip();
  size_t write(uint8_t);
  size_t write(const char *str);
  size_t write(uint8_t *buf, size_t size);
  int available();
  int read();
  void flush();
  void stop();
  uint8_t connected();
};

  
String ask_serial(char * buff, String command, bool set, int del);
String flush_buffer_to_string(char * buff, int len);
void read_buffer(char * buff, int len);
int read_line(char * buff);
int connect_wifi_psk(String ssid, String pass);
String flush_all_to_string(char * buff);
void read_all(char * buff);
