#include "vive510.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#define SIGNALPIN1 4 // pin receiving signal from Vive circuit
#define SIGNALPIN2 5 // pin receiving signal from Vive circuit
// 3 works for sure but 5 work kinda if starts after
#define FREQ 3 // in Hz

Vive510 vive1(SIGNALPIN1);
Vive510 vive2(SIGNALPIN2);

// keep last pose to filter out bad data
static int last_x1, last_y1;
static int last_x2, last_y2;
// threshold differeance for throwing out data
int threshold = 100;

// wifi
// uncomment the router SSID and Password that is being used 

// const char* ssid     = "TP-Link_05AF";
// const char* password = "47543454";

const char* ssid     = "TP-Link_E0C8";
const char* password = "52665134";

//const char* ssid     = "TP-Link_FD24"; 
//const char* password = "65512111";

WiFiUDP UDPTestServer;
IPAddress myIP(192, 168, 1, 163);
IPAddress TargetIP(192, 168, 1, 119);
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  // for some reason it breaks if don't include wifi mode
  // likely delay/timing issue
  WiFi.mode(WIFI_STA);
  WiFi.config(myIP, 
    IPAddress(192, 168, 1, 1), 
    IPAddress(255, 255, 255, 0));

  WiFi.begin(ssid, password);

  UDPTestServer.begin(3000);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //  Serial.print(".");
  // }

  vive1.begin(SIGNALPIN1);
  vive2.begin(SIGNALPIN2);
}


void loop() {
  static long int ms = millis();
  static uint16_t x1,y1;
  static uint16_t x2,y2;
  
  if (millis()-ms>1000/FREQ) {
    ms = millis();
    // float delta_x = x1 - x2;
    // float delta_y = y1 - y2;
    // int angle = atan2(delta_y, delta_x) * (180.0/3.1415);
    ets_printf("Vive1 %d %d Vive2 %d %d\n", x1, y1, x2, y2);
    char udpBuffer[20];
    sprintf(udpBuffer, "%d:%d:%d:%d",x1,y1,x2,y2);   
                                                
    UDPTestServer.beginPacket(TargetIP, 3000);
    UDPTestServer.println(udpBuffer);
    UDPTestServer.endPacket();
  }
  
  // every other loop read coords
  
  if (vive1.status() == VIVE_RECEIVING) {
    x1 = vive1.xCoord();
    y1 = vive1.yCoord();
  } else {
    vive1.sync(15);
  }

  if (vive2.status() == VIVE_RECEIVING) {
    x2 = vive2.xCoord();
    y2 = vive2.yCoord();
  } else {
    vive2.sync(15);
  }
  

   // if points too far way then its an error
  if ((last_x1 != 0 && abs(last_x1-x1) > threshold) ||
   (last_y1 != 0 && abs(last_y1-y1) > threshold)) {
    x1 = last_x1;
    y1 = last_y1;
  } else {
    last_x1 = x1;
    last_y1 = y1;
  }

  // if points too far way then its an error
  if ((last_x2 != 0 && abs(last_x2-x2) > threshold) ||
   (last_y2 != 0 && abs(last_y2-y2) > threshold)) {
    x2 = last_x2;
    y2 = last_y2;
  } else {
    last_x2 = x2;
    last_y2 = y2;
  }
  // Serial.printf("count %d switcher %d\n", counter, switcher);
  delay(1);
}
