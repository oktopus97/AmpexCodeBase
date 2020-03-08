//custom wifi client to be used with at commands
#include <WiFi.h>
class EspClient:WiFiClient{
  private:
  String ssid;
  String pass;
  public:
  EspClient(){};
  EspClient(String ssid, String pass);
  uint8_t status();
  uint8_t connect(IPAddress, int);
  uint8_t reconnect();
  size_t write(uint8_t);
  size_t write(const char *str);
  size_t write(uint8_t *buf, size_t size);
  int available();
  int read();
  void flush();
  void stop();
  uint8_t connected();
};

  
String ask_serial(String command);
String read_serial();
int connect_wifi_psk(String ssid, String pass);
int set_local_ip(IPAddress ip, IPAddress, IPAddress);
