//connection parameters
#include <IPAddress.h>

//modes
#define SERIAL 0
#define WIFI   1
#define MODE WIFI

//Test boolean
#define TEST   1

//wifi vars (don't compile them if not necessary)
#if MODE == WIFI

#define WIFISSID "o2-WLAN00"
#define WIFIPASS "C4XVVED2UTNFN7P7"
#define MQTTUSER "try"
#define MQTTPASS "try"

String IP = "192.168.5.10";
String SUBNET = "255.255.255.0";
String GATEWAY = "192.168.5.254";

String BROKER = "192.168.5.1";
#define PORT 1883

#define BUFFER_SIZE (50)
#endif
