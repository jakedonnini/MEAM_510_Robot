#include <WiFi.h>
#include <WiFiUdp.h>
#include "Adafruit_VL53L0X.h"
#include <Wire.h>

static uint32_t us; // current time in microsec
static uint32_t lastUpdate1;
static uint32_t lastUpdate2;
static uint32_t lastTimeSensed;
static int relativeAngle; // relative angle of sensed beacon
static uint32_t lastServo;
static uint32_t lastLED;
static uint32_t lastMessageRec;
static uint32_t lastMessageSent;
static uint32_t lastDist;

static int mode = 0; // should be 0
static int dist1, dist2;

// Pins
int flashSensor1 = 7;
int flashSensor2 = 6;
int servoPin = 1;
int ledChannel = 1;
int freq = 50;
int resolution = 12;

#define CHANNEL 0                  // channel can be 1 to 14, channel 0 means current channel.  
#define MAC_RECV  {0xA0,0x76,0x4E,0x19,0x76,0xE0} // receiver MAC address (last digit should be even for STA)

esp_now_peer_info_t peer1 = 
{
  .peer_addr = MAC_RECV, 
  .channel = CHANNEL,
  .encrypt = false,
};

// wifi
// uncomment the router SSID and Password that is being used 

// const char* ssid     = "TP-Link_05AF";
// const char* password = "47543454";

const char* ssid     = "TP-Link_E0C8";
const char* password = "52665134";

//const char* ssid     = "TP-Link_FD24"; 
//const char* password = "65512111";

WiFiUDP UDPTestServer;
IPAddress myIP(192, 168, 1, 151);
IPAddress TargetIP(192, 168, 1, 119);
WiFiServer server(80);

// i2c
// address we will assign if dual sensor is present
#define LOX1_ADDRESS 0x30
#define LOX2_ADDRESS 0x31

// set the pins to shutdown
#define SHT_LOX1 10
#define SHT_LOX2 0

// objects for the vl53l0x
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();

int sda_pin = 4; // GPIO16 as I2C SDA
int scl_pin = 5; // GPIO17 as I2C SCL


// this holds the measurement
VL53L0X_RangingMeasurementData_t measure1;
VL53L0X_RangingMeasurementData_t measure2;

// beacon period [us]
static int WINDOW = 100;
static int RES = 1;
static int SERVOOFF = 250;

/*
    Reset all sensors by setting all of their XSHUT pins low for delay(10), then set all XSHUT high to bring out of reset
    Keep sensor #1 awake by keeping XSHUT pin high
    Put all other sensors into shutdown by pulling XSHUT pins low
    Initialize sensor #1 with lox.begin(new_i2c_address) Pick any number but 0x29 and it must be under 0x7F. Going with 0x30 to 0x3F is probably OK.
    Keep sensor #1 awake, and now bring sensor #2 out of reset by setting its XSHUT pin high.
    Initialize sensor #2 with lox.begin(new_i2c_address) Pick any number but 0x29 and whatever you set the first sensor to
 */
void setID() {
  // all reset
  digitalWrite(SHT_LOX1, LOW);    
  digitalWrite(SHT_LOX2, LOW);
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  // activating LOX1 and resetting LOX2
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);

  // initing LOX1
  if(!lox1.begin(LOX1_ADDRESS)) {
    Serial.println(F("Failed to boot first VL53L0X"));
    while(1);
  }
  delay(10);

  // activating LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  //initing LOX2
  if(!lox2.begin(LOX2_ADDRESS)) {
    Serial.println(F("Failed to boot second VL53L0X"));
    while(1);
  }
}

void read_dual_sensors() {
  Wire.begin();
  
  lox1.rangingTest(&measure1, false); // pass in 'true' to get debug data printout!
  lox2.rangingTest(&measure2, false); // pass in 'true' to get debug data printout!

  // print sensor one reading
  if(measure1.RangeStatus != 4) {     // if not out of range
    dist1 = measure1.RangeMilliMeter;
  } else {
    // know to filter out
    dist1 = 0;
  }
 
  //print sensor two reading
  if(measure2.RangeStatus != 4) {
    dist2 = measure2.RangeMilliMeter;
  } else {
    dist2 = 0;
  }
  Wire.end();

  Serial.printf("dist1: %d dist2: %d\n", dist1, dist2);
}


void senseBeacon(int ch, int FLASHPER) {
  if (mode == 2) {
    WINDOW = 1000;
  } else {
    WINDOW = 100;
  }
  static int oldpin[2];
  static uint32_t oldtime[2];
  int pin[] = {flashSensor1, flashSensor2};
  int reading = digitalRead(pin[ch]);
  if (reading != oldpin[ch]) {
    // float t = 1 / ((us-oldtime[ch]) * 0.000001);
    int t = us-oldtime[ch];
    Serial.print(ch);
    Serial.print(" ");
    Serial.println(t);
    if (t > FLASHPER-WINDOW && t < FLASHPER+WINDOW) { 
      if (ch==0) {
        if (us-lastUpdate1 > 1000000/50) {
          relativeAngle++; // need to scale with freq
          lastUpdate1 = us;
          lastTimeSensed = us;
        }
      } else {
        if (us-lastUpdate2 > 1000000/50) {
          relativeAngle--; // need to scale with freq
          lastUpdate2 = us;
          lastTimeSensed = us;
        }
      }
    }
    oldpin[ch] = reading;
    oldtime[ch] = us;
  }
}

void sendMessage() {
  if (us-lastMessageSent > 1000000/10) {

    char udpBuffer[20];
    sprintf(udpBuffer, "%d:%d:%d",(relativeAngle+200),dist1,dist2);   
                                                
    UDPTestServer.beginPacket(TargetIP, 3000);
    UDPTestServer.println(udpBuffer);
    UDPTestServer.endPacket();
    lastMessageSent = us;
  }
}

// freq of flasher looking for
// 1/23Hz/2=21739 and 1/550Hz/2 = 909 in us
// devide by two because it measures twice
void moveFlasherServo(int period) {
  // us = micros();
  senseBeacon(0, period); // increase relativeAngle if sensed left
  senseBeacon(1, period); // decrease relativeAngle if sensed right

  if (us-lastTimeSensed < 2000000) {
    // update the servo position
    if (us-lastServo > 1000000/10) { // run 10Hz
      relativeAngle = constrain(relativeAngle, -150, 150);
      // Serial.printf("angle pwm %d\n", relativeAngle);
      // update servo pos
      ledcWrite(ledChannel, SERVOOFF+relativeAngle);
      lastServo = us;
    }
  } else {
    relativeAngle = 0;
    lastTimeSensed = us;
  }
  delayMicroseconds(200);
}

const int UDP_PACKET_SIZE = 100; // allow packets up to 100 bytes
byte packetBuffer[UDP_PACKET_SIZE]; // can be up to 65535

void handleUDPServer() {
  int i, cb = UDPTestServer.parsePacket();
  if (cb) {
    UDPTestServer.read(packetBuffer, UDP_PACKET_SIZE);
    int val = (packetBuffer[1]<<8) + packetBuffer[0]
    if (val != 100) {
      mode = val;
    } else {

    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.setPins(sda_pin, scl_pin); // Set the I2C pins before begin
  pinMode(flashSensor1, INPUT);
  pinMode(flashSensor2, INPUT);
  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);

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

  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(servoPin, ledChannel);

  Serial.println(F("Shutdown pins inited..."));

  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);

  Serial.println(F("Both in reset mode...(pins are low)"));
  
  
  Serial.println(F("Starting..."));
  setID();
}

void loop() {
  us = micros();
  if (mode == 1) {
    // look for fake
    if (us-lastDist > 1000000) {
      read_dual_sensors();
      lastDist = us;
    } else {
      moveFlasherServo(909);
    }
    sendMessage();
  } else if (mode == 2) {
    // look for real
    if (us-lastDist > 1000000) {
      read_dual_sensors();
      lastDist = us;
    } else {
      moveFlasherServo(21739);
    }
    sendMessage();
  } else {
    // do nothing
    ledcWrite(ledChannel, SERVOOFF);
    read_dual_sensors(); // need to make function to send message
    sendMessage();
  }

  if (us-lastMessageRec > 10000) {
    handleUDPServer();
    lastMessageRec = us;
  }
}
