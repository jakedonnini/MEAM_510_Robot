#include <Arduino.h>
#include <driver/ledc.h>
#include "html510.h"
#include <cmath>
#include "website.h"
#include "nipple.h"
HTML510Server h(80);


// ****************************** VIVE ***************************************
WiFiUDP UDPTestServer;
static int vive_x1,vive_y1,vive_x2,vive_y2;
IPAddress VIVE_IP(192, 168, 1, 163);

const int UDP_PACKET_SIZE = 100; // allow packets up to 100 bytes

// ***************************************************************************
// ******************************** IR Scanner *******************************
static int IR_Angle;
// dist1 is front dist2 is side
static int dist1, dist2;
static int searchCounter;
IPAddress IR_IP(192, 168, 1, 151);

void fncUdpSend(short int i) // send 2 byte int i
{
  byte udpBuffer[2];
  udpBuffer[0] = i & 0xff; // send 1st (LSB) byte of i
  udpBuffer[1] = i>>8; // send 2nd (MSB) byte of i
  UDPTestServer.beginPacket(IR_IP, 3000);
    UDPTestServer.write(udpBuffer, 2); // send 2 bytes in udpBuffer
  UDPTestServer.endPacket();
}

// ***************************************************************************
// pins
const int PWMPinR = 0; // was 4
const int dirPinR = 6;

const int PWMPinL = 1;
const int dirPinL = 7;

const int encodeR = 9; // was 5
const int encodeL = 10; // need to fix from 8

// gripper pin
const int gripPin = 4;
const int gripChannel = 4;

// LEDC settings
const int ledcChannelR = 0; // was 4
const int ledcChannelL = 1;
const int ledcResolution = 12; // 8-bit resolution for duty cycle
const int ledcFreq = 50; // was 80

// encoders
static int speedR;
static int speedL;
static int targetSpeed;
static int direction;
static int lastEncodeValR = 0;
static int lastEncodeValL = 0;
static int posR = 0;
static int posL = 0;
static int R;
static int L;
static unsigned long lastTimeEncode;
static unsigned long lastTimeUDP;
static float timeInterval = 100.0;
static int maxControl = 500;
static int maxPID = 1000 - maxControl;
#define PI 3.14159

// navigation
static bool reachedGoal = true;
static bool lookingForTrophie = false;
static float robotAngle;
static int policeX, policeY;
static int target_X, target_Y;

// secondary functions
static int grabState = 0;
static int wallState = 0;
static int searchState = 0;
static int moveState = 0;
static int policeState = 0;

// ****************************** Police *************************************
#define UDPPORT 2510 // port for game obj transmission
WiFiUDP UDPServer;

// uncomment the router SSID and Password that is being used 

// const char* ssid     = "TP-Link_05AF";
// const char* password = "47543454";

const char* ssid     = "TP-Link_E0C8";
const char* password = "52665134";

//const char* ssid     = "TP-Link_FD24"; 
//const char* password = "65512111";

void handleUDPServerPolice() {
   const int UDP_PACKET_SIZE = 14; // can be up to 65535          
   uint8_t packetBufferPol[UDP_PACKET_SIZE];

   int cb = UDPServer.parsePacket(); // if there is no message cb=0
   if (cb) {
      int x,y;
      packetBufferPol[13]=0; // null terminate string

    UDPServer.read(packetBufferPol, UDP_PACKET_SIZE);
      int team = atoi((char *)packetBufferPol);
      x = atoi((char *)packetBufferPol+3); // ##,####,#### 2nd indexed char
      y = atoi((char *)packetBufferPol+8); // ##,####,#### 7th indexed char
      // Serial.print("From Team ");
      // Serial.println((char *)packetBufferPol);
      if (team == 0) {
        policeX = x;
        policeY = y;
      }
      // Serial.println(x);
      // Serial.println(y);
   }
}

IPAddress ipTarget(192, 168, 1, 255); // 255 => broadcast
int teamNumber = 3;

void UdpSend(int x, int y) {
  char udpBuffer[20];
  sprintf(udpBuffer, "%02d:%4d,%4d",teamNumber,x,y);   
                                              
  UDPServer.beginPacket(ipTarget, UDPPORT);
  UDPServer.println(udpBuffer);
  UDPServer.endPacket();
  Serial.println(udpBuffer);
}

// ***************************************************************************



void handleRoot(){
  // web or nip
  h.sendhtml(web);
}

void handleDir(){
  // set the global dir
  direction = h.getVal();
  if (direction >= 45 && direction < 135) { // back
    direction = 2;
  } else if (direction >= 135 && direction < 225) { // right
    direction = 1;
  } else if (direction >= 225 && direction < 315) { // forward
    direction = 0;
  } else if ((direction >= 315 && direction <= 360) || (direction >= 0 && direction < 45)) {
    direction = 3;
  }
  h.sendplain(String(direction));
}

void handleSpeed(){
  // set the target speed
  targetSpeed = map(h.getVal(), 0, 170, 0, maxControl);
  h.sendplain(String(targetSpeed));
}

void handlePos() {
  int i = h.getVal();
  String data = String(vive_x1) + " " + String(vive_y1) + " " + String(vive_x2)
                + " " + String(vive_y2) + " " + String(policeX) + " " + String(policeY) + " " +
                String(dist1) + " " + String(dist2);
  h.sendplain(data);
}

void handleGrab() {
  grabState = h.getVal();
  if (grabState == 1) {
    h.sendplain("Closed");
  } else {
    h.sendplain("Open");
  }
}

void handleWall() {
  wallState = h.getVal();
  if (wallState == 1) {
    h.sendplain("ON");
  } else {
    h.sendplain("OFF");
  }
}

void handleSearch() {
  int type = h.getVal();
  // 1 for fake 2 for real 0 for stop
  startTrophie(type);
  if (type == 1) {
    searchState = 1;
    h.sendplain("Searching Fake...");
  } else if (type == 2) {
    searchState = 1;
    h.sendplain("Searching Real...");
  } else {
    searchState = 0;
    h.sendplain("Not Searching");
  }
}

void handleMoveX() {
  moveState = 1;
  target_X = h.getVal();
  h.sendplain("GOOD_X");
}

void handleMoveY() {
  target_Y = h.getVal();
  h.sendplain("GOOD_Y");
}

void handlePolice() {
  policeState = h.getVal();
  Serial.println(policeState);
  if (policeState == 1) {
    h.sendplain("ON");
  } else {
    h.sendplain("OFF");
  }
}

// get data from UDP
void handleUDPServer() {
  const int UDP_PACKET_SIZE_2 = 25;
  uint8_t packetBuffer[UDP_PACKET_SIZE_2];
  int i, cb = UDPTestServer.parsePacket();
  if (cb) {
    IPAddress IP = UDPTestServer.remoteIP();
    UDPTestServer.read(packetBuffer, UDP_PACKET_SIZE_2);

    // Vive sends each value as a different message staring with
    // 1 to indicate the start of a message and then updates the coords

    if (IP == VIVE_IP) {
      String vive_str = (char *)packetBuffer;
      String strs[4];
      //Serial.println(vive_str);
      int StringCount = 0;
      // Split the string into substrings
      while (vive_str.length() > 0) {
        int index = vive_str.indexOf(':');
        if (index == -1) // No space found
        {
          strs[StringCount++] = vive_str;
          break;
        }
        else
        {
          strs[StringCount++] = vive_str.substring(0, index);
          vive_str = vive_str.substring(index+1);
        }
      }
      for (int i=0; i < 4; i++) {
        if (i == 0) {
          vive_x1 = strs[i].toInt();
        } else if (i == 1) {
          vive_y1 = strs[i].toInt();
        } else if (i == 2) {
          vive_x2 = strs[i].toInt();
        } else if (i == 3) {
          vive_y2 = strs[i].toInt();
        } 
      }
      robotAngle = findAngle();
      Serial.printf("Vive1 %d %d Vive2 %d %d police %d %d\n", vive_x1, vive_y1, vive_x2, vive_y2, policeX, policeY);
    }

    if (IP == IR_IP) {
      String ir_str = (char *)packetBuffer;
      String strs[3];
      int StringCount = 0;
      // Split the string into substrings
      while (ir_str.length() > 0) {
        int index = ir_str.indexOf(':');
        if (index == -1) // No space found
        {
          strs[StringCount++] = ir_str;
          break;
        }
        else
        {
          strs[StringCount++] = ir_str.substring(0, index);
          ir_str = ir_str.substring(index+1);
        }
      }
      for (int i=0; i < 3; i++) {
        if (i == 0) {
          IR_Angle = strs[i].toInt()-200;
        } else if (i == 1) {
          dist1 = strs[i].toInt();
        } else if (i == 2) {
          dist2 = strs[i].toInt();
        }
      }
      //Serial.printf("IR angle %d dist1: %d dist2: %d\n", IR_Angle, dist1, dist2);
    }
  }
}

float PWM_to_RPS(int x) {
  float y = 0.73 * log(6.9 * x - 736.96) - 1.48;
  return y;
}

int RPS_to_PWM(float x) {
  float y = 0.145 * exp(1.37 * x + 2.03) + 106.8;
  return (int)y;
}

static float summedErrorR = 0;
static int past_error_signR;
static float lastSensorR = 0;
static float summedErrorL = 0;
static int past_error_signL;
static float lastSensorL = 0;

int PID_control(int desired, int sensor, int ch) {
  if (desired == 0) {
    return 0;
  }

  float summedError;
  float lastSensor;
  int past_error_sign;
  if (ch == 0) {
    summedError = summedErrorR;
    past_error_sign = past_error_signR;
    lastSensor = lastSensorR;
  } else {
    summedError = summedErrorL;
    past_error_sign = past_error_signL;
    lastSensor = lastSensorL;
  }

  float KP = 2.5;
  float KI = 0; // 0.5
  float KD = 0; // 0.8

  float desired_RPS = PWM_to_RPS(desired);
  // 12 counts per rev * 1:34 gear ratio = 408 per rev
  // devide by time interval (0.1s) for rotations per sec
  float actual_RPS = (sensor / 408.0) / (0.001*timeInterval);

  float velocity = actual_RPS - lastSensor;

  float error = desired_RPS - actual_RPS;
  float u = KP * error + KI * summedError + KD * velocity;

  //Serial.printf("error %d summederror %d u %d\n", error, summedError, (int)u);
  // Serial.print(summedError);
  // Serial.print(" ");
  // Serial.print(desired_RPS);
  // Serial.print(" ");
  // Serial.println(u);
  // Serial.printf("desired %f actual %f error %f\n", desired_RPS, actual_RPS, error);

  // Anti-windup check
  if (abs(u) < maxPID) {
    summedError += 1.0 * error;
  }
  int err_sign = error/abs(error);
  // Serial.printf("current sign %d last sign %d\n", err_sign, past_error_sign);
  // Prevent integral windup
  if (err_sign != past_error_sign) {
    past_error_sign = err_sign;
    summedError = 0;
  }
  if (summedError > maxPID/4) {
    summedError = maxPID;
  }
  if (summedError < -maxPID/4) {
    summedError = -maxPID;
  }

  // Bound the output
  int u_PWM =  RPS_to_PWM(u);
  u_PWM = constrain(u_PWM, -maxPID, maxPID);

  // no feedback
  // return 0;

  lastSensor = actual_RPS;

  if (ch == 0) {
    summedErrorR = summedError;
    past_error_signR = past_error_sign;
    lastSensorR = lastSensor;
  } else {
    summedErrorL = summedError;
    past_error_signL = past_error_sign;
    lastSensorL = lastSensor;
  }

  return u_PWM;
}

void controlMotor(int direction, int speed) {
  // dir: 0->forward 1->right 2->back 3->left
  // Serial.printf("dir %d speed %d\n", direction, speed);
  // Serial.printf("speedR %d speedL %d\n", speedR, speedL);

  // Back
  if (direction == 2) {
  
    digitalWrite(dirPinR, LOW);
    ledcWrite(ledcChannelR, speed+speedR);
    
    digitalWrite(dirPinL, HIGH);
    ledcWrite(ledcChannelL, speed+speedL);
  }
  // Right
  else if (direction == 1) {
    digitalWrite(dirPinR, HIGH);
    ledcWrite(ledcChannelR, speed+speedR);
    
    digitalWrite(dirPinL, HIGH);
    ledcWrite(ledcChannelL, speed+speedL);
  }
  // forward
  else if (direction == 0) {
    digitalWrite(dirPinR, HIGH);
    ledcWrite(ledcChannelR, speed+speedR);

    digitalWrite(dirPinL, LOW);
    ledcWrite(ledcChannelL, speed+speedL);
  }
  // Left
  else if (direction == 3) {
    digitalWrite(dirPinR, LOW);
    ledcWrite(ledcChannelR, speed+speedR);
    
    digitalWrite(dirPinL, LOW);
    ledcWrite(ledcChannelL, speed+speedL);
  }
  // Stop
  else {
    digitalWrite(dirPinR, LOW);
    ledcWrite(ledcChannelR, 0);
    
    digitalWrite(dirPinL, LOW);
    ledcWrite(ledcChannelL, 0);
  }
}

static float pwm_del = 0;

void wallFollow() {
  int avg_speed = 300;
  float Kp = 1.2; 
  float Kd = 0.7;
  int distance_desired = 200;
  pwm_del = Kp * (distance_desired-dist2) - Kd * (R-L);
  
  if (dist1 > 200 || dist1 == 0) {
    // Serial.println(pwm_del);
    pwm_del = constrain(pwm_del, -150, 150);
    digitalWrite(dirPinR, HIGH);
    ledcWrite(ledcChannelR, avg_speed-pwm_del);

    digitalWrite(dirPinL, LOW);
    ledcWrite(ledcChannelL, avg_speed+pwm_del);
  } else {
    digitalWrite(dirPinR, LOW);
    ledcWrite(ledcChannelR, avg_speed);

    digitalWrite(dirPinL, LOW);
    ledcWrite(ledcChannelL, avg_speed/2);
  }
}

static int moveDone = true;

void moveTo(int x, int y) {
  float delta_x = vive_x1-x;
  float delta_y = vive_y1-y;
  float angleToPoint = atan2(delta_y, delta_x) * (180.0/PI);
  float angleDiff = angleToPoint - robotAngle;
  float distanceToTarget = sqrt(delta_x*delta_x + delta_y*delta_y);

  // turn until angles match
    // used to determine if turn is optimal
    float angle_sum = angleToPoint + robotAngle;
    if (angleDiff > 180) {
      angleDiff -= 360;
      angle_sum += 360;
    } else if (angleDiff < -180) {
      angleDiff += 360;
      angle_sum -= 360;
    }

  if (abs(angleDiff) < 25) {
    // drive forward
    if (abs(distanceToTarget) < 300) {
      Serial.println("done");
      moveDone = true;
      // made it to destination
      targetSpeed = 0;
      moveState = 0;
      controlMotor(0, targetSpeed);
    } else {
      targetSpeed = (int)(180); // (int)(100 * 0.01 * distanceToTarget);
      Serial.printf("distance: %f speed: %d\n", distanceToTarget, targetSpeed);
      controlMotor(0, targetSpeed);
    }
    
  } else {
    if (abs(distanceToTarget) < 300) {
      Serial.println("done");
      moveDone = true;
      // made it to destination
      targetSpeed = 0;
      moveState = 0;
      controlMotor(0, targetSpeed);
    }
    Serial.printf("desired angle: %f actual angle: %f angle diff: %f sum: %f\n", angleToPoint, robotAngle, angleDiff, angle_sum);
    if (angleDiff < 0 && angle_sum < 360) {
      // turn left at set turn speed
      // 100 is very slow
      digitalWrite(dirPinR, LOW);
      ledcWrite(ledcChannelR, 150);

      digitalWrite(dirPinL, LOW);
      ledcWrite(ledcChannelL, 150);
    } else {
      // turn right
      digitalWrite(dirPinR, HIGH);
      ledcWrite(ledcChannelR, 150);

      digitalWrite(dirPinL, HIGH);
      ledcWrite(ledcChannelL, 150);
    }
  }
}

float findAngle() {
  float delta_x = vive_x1-vive_x2;
  float delta_y = vive_y1-vive_y2;
  float angle = atan2(delta_y, delta_x) * (180.0/PI) + 180;
  // Serial.println(angle);
  return angle;
}

// 1 for fake 2 for real 0 for stop
// only run this once per state update
void startTrophie(int target) {
  if (target == 1 || target == 2) {
    lookingForTrophie = true;
  } else {
    lookingForTrophie = false;
  }
  Serial.println(target);
  fncUdpSend(target);
}

static int searchX = 4000;
static int searchY = 4000;
static bool searchForward = true;
static int spinCount = 0;

// run cont until found
void findTrophie() {
  // may want to send out of range value to indicate no hit
  if (IR_Angle == 0) {
    if (spinCount < 100 && moveDone) {
      Serial.println(spinCount);
      digitalWrite(dirPinR, LOW);
      ledcWrite(ledcChannelR, 200);

      digitalWrite(dirPinL, LOW);
      ledcWrite(ledcChannelL, 200);
      delay(1);
      spinCount++;
    } else {
      spinCount = 0;
      Serial.printf("x: %d, %d\n", searchX, searchY);
      moveDone = false;
      moveTo(searchX, searchY);
      // if (moveDone) {
      //   if (searchForward) {
      //     searchX += 1000;
      //   } else {
      //     searchX -= 1000;
      //   }
      //   if (searchX > 4000) {
      //     searchForward = false;
      //   } else if (searchX < 4000) {
      //     searchForward = true;
      //   }
      // }
    }
  } else {
    // found something
    if (abs(IR_Angle) < 30) {
      if (dist1 > 50) {
        targetSpeed = 200;
        controlMotor(0, targetSpeed);
      } else {
        // close gripper
        gripper(1);
        lookingForTrophie = false;
      }
      
    } else {
      if (IR_Angle > 0) {
        // turn right
        digitalWrite(dirPinR, LOW);
        ledcWrite(ledcChannelR, 150);

        digitalWrite(dirPinL, LOW);
        ledcWrite(ledcChannelL, 150);
      } else {
        // turn left
        digitalWrite(dirPinR, HIGH);
        ledcWrite(ledcChannelR, 150);

        digitalWrite(dirPinL, HIGH);
        ledcWrite(ledcChannelL, 150);
      }
    }
  }

}

void updateEncoder() {
  int sensorR = digitalRead(encodeR);
  int sensorL = digitalRead(encodeL);
  if (sensorR != lastEncodeValR) {
    lastEncodeValR = sensorR;
    posR += 1;
  }
  if (sensorL != lastEncodeValL) {
    lastEncodeValL = sensorL;
    posL += 1;
  }
}

int getVelR() {
  int vel = posR;
  posR = 0;
  return vel;
}

int getVelL() {
  int vel = posL;
  posL = 0;
  return vel;
}

// 0 for open 1 for close
void gripper(int x) {
  if (x == 1) {
    //Serial.println(x);
    ledcWrite(gripChannel, 265);
  } else {
    ledcWrite(gripChannel, 130);
  }
}

void pushPolice() {
  Serial.printf("looking for police dist1: %d\n", dist1);
  // if at police car
  if (dist1 > 100) {
    moveTo(policeX, policeY);
  } else {
    // push forward max power
    digitalWrite(dirPinR, HIGH);
    ledcWrite(ledcChannelR, 1000);

    digitalWrite(dirPinL, LOW);
    ledcWrite(ledcChannelL, 1000);
  }
}

void setup() {
  // Initialize LED pin as an output
  pinMode(PWMPinR, OUTPUT);
  pinMode(PWMPinL, OUTPUT);
  pinMode(dirPinR, OUTPUT);
  pinMode(dirPinL, OUTPUT);
  pinMode(encodeR, INPUT);
  pinMode(encodeL, INPUT);
  Serial.begin(115200);

  // Configure LEDC
  ledcSetup(ledcChannelR, ledcFreq, ledcResolution);
  ledcAttachPin(PWMPinR, ledcChannelR);
  ledcAttachPin(PWMPinL, ledcChannelL);
  ledcSetup(ledcChannelR, ledcFreq, ledcResolution);
  ledcWriteTone(ledcChannelR, ledcFreq);
  ledcSetup(ledcChannelL, ledcFreq, ledcResolution);
  ledcWriteTone(ledcChannelL, ledcFreq);
  // ledcWriteTone(gripChannel, ledcFreq);
  delay(100);

  // stop motor
  targetSpeed = 0;
  controlMotor(0, targetSpeed);

  // give vive time to catch up
  delay(3000);

  IPAddress myIP(192,168,1,119);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(myIP,IPAddress(192,168,1,1),
              IPAddress(255,255,255,0));

  UDPTestServer.begin(3000);
  UDPServer.begin(UDPPORT);

  Serial.println(WiFi.status());
  
  h.begin();
  h.attachHandler("/ ",handleRoot);
  // for nip
  h.attachHandler("/speed?val=", handleSpeed);
  h.attachHandler("/dir?val=", handleDir);
  // for web
  h.attachHandler("/pos?val=", handlePos);
  h.attachHandler("/police?val=", handlePolice);
  h.attachHandler("/grab?val=", handleGrab);
  h.attachHandler("/wall?val=", handleWall);
  h.attachHandler("/search?val=", handleSearch);
  h.attachHandler("/moveX?val=", handleMoveX);
  h.attachHandler("/moveY?val=", handleMoveY);

  // start open
  grabState = 0;
  ledcAttachPin(gripPin, gripChannel);
  ledcSetup(gripChannel, ledcFreq, ledcResolution);
  delay(100);
}

void loop() {

  //turn if hit wall
  // if (dist1 < 200 && dist1 != 0) {
  //   digitalWrite(dirPinR, LOW);
  //   ledcWrite(ledcChannelR, 300);

  //   digitalWrite(dirPinL, HIGH);
  //   ledcWrite(ledcChannelL, 0);
  // }

  // toggle the grabber
  gripper(grabState);

  
  
  // highest priority
  if (moveState == 1) {
    // cancel other tasks
    searchState = 0;
    wallState = 0;
    moveTo(target_X, target_Y);
  } else if (wallState == 1) {
    searchState = 0;
    policeState = 0;
    moveState = 0;
    wallFollow();
  } else if (searchState == 1) {
    wallState = 0;
    policeState = 0;
    moveState = 0;
    findTrophie();
  } else if (policeState == 1) {
    moveState = 0;
    wallState = 0;
    searchState = 0;
    pushPolice();
  } else {
    searchState = 0;
    wallState = 0;
    policeState = 0;
    // Serial.println(targetSpeed);
    targetSpeed = 0;
    controlMotor(0, targetSpeed);
  }

  // vive traking and dist update
  handleUDPServer();

  if (millis() - lastTimeEncode > timeInterval) {
    UdpSend(vive_x1, vive_y1);
    lastTimeEncode = millis();
    R = getVelR();
    L = getVelL();
    //Serial.printf("R %d L %d\n", R, L);
    // target speed in PWM
    speedR = PID_control(targetSpeed, R, 0);
    speedL = PID_control(targetSpeed, L, 1);
    // Serial.printf("RightPI %d leftPI %d\n", speedR, speedL);
  }
  updateEncoder();

  // controlMotor(direction, targetSpeed);
  handleUDPServerPolice();

  h.serve();
}
