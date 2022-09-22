#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

const char* ssid = "LoR Robot";
const char* ROBOT_HTML = "<html><body><h1>LoR Robot</h1></body></html>";



/** Pins definitions */
#define IN_1 16  // D0
#define IN_2 5   // D1
#define ENA 4    // D2

#define IN_3 0  // D3
#define IN_4 2  // D4
#define ENB 14  // D5

#define ENCODER_PIN 12  // D6

#define ECHO 13 // D7
#define TRIG 15 // D8



ESP8266WebServer server(80);

IPAddress localIP(192, 168, 5, 1);
IPAddress gateway(192, 168, 5, 1);
IPAddress mask(255, 255, 255, 0);


/** Convert string response to JSON */
DynamicJsonDocument strToJSON(String data) {
  DynamicJsonDocument document(1024);
  deserializeJson(document, data);

  return document;
}


/** Split string by delimiter */
String split(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}



void motor(int dir, int pwmPin, int pwmSpeed, int in1, int in2) {
    analogWrite(pwmPin, pwmSpeed); // set speed
    if (dir == 1) { // rotate to clockwise
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
    } else if (dir == -1) { // rotate to anti clockwise
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
    } else { // stop motor
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
    }
}


long duration;
double distance;
double ultrasonic() {
    // Clears TRIG pin if HIGH
    digitalWrite(TRIG, LOW);
    delayMicroseconds(10);
    // Sets the TRIG to HIGH (ACTIVE) for 10 microseconds
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    // Reads the ECHO pin
    duration = pulseIn(ECHO, HIGH);
    // Calculating the distance
    distance =
        duration * 0.034 / 2; 

    return distance;
}

/** Calculating u(t) */
double u(int x) {
  double x_ref = 340.0;
  double kp = .1;
  double u = kp * (x_ref - x);
  return u;
}

/** Calculating Angle */
/* 
(centerX, centerY): origine
vector 1: (x, y)
vector 2: (centerX, centerY) 
*/
double vectorAngle(int x, int y) {
  int x0 = 360;
  int y0 = 0;

  int centerX = 360;
  int centerY = 325;

  x = x - x0;
  y = (y - y0)*-1;

  centerX = centerX - x0;
  centerY = (centerY - y0)*-1;


  double dot = x*centerX + y*centerY;      // dot product between [x, y] and [centerX, centerY]
  double det = x*centerY - y*centerX;      // determinant
  double angle = atan2(det, dot);


  // convert angle from radians to degree

  angle = angle * (180 / 3.14);


  return angle;

}


int speed = 90;
void control(int xLaneCenter) {
  if ( 320 <= xLaneCenter && xLaneCenter <= 370 ) {
    motor(1, ENA, speed, IN_1, IN_2);
    motor(1, ENB, speed, IN_3, IN_4);
  } else {
    motor(0, ENA, 0, IN_1, IN_2);
    motor(0, ENB, 0, IN_3, IN_4);
  }
}

void motorControl(int xLane) {
  int signal = (int)u(xLane);
  int speed = 90;
  int s = speed - (int)(signal/2);

  if (signal == 0) {
    motor(1, ENA, speed, IN_1, IN_2);
    motor(1, ENB, speed, IN_3, IN_4);
    Serial.println("0");
  } 
  if (signal > 0) {
    signal = 340 - signal;
    motor(1, ENA, speed * (signal/100), IN_1, IN_2);
    motor(1, ENB, speed, IN_3, IN_4);
    Serial.println("signal < 0");
  }
   if (signal < 0) {
    signal = 340 - (-1*signal);
    motor(1, ENA, speed, IN_1, IN_2);
    motor(1, ENB, speed * (signal/100), IN_3, IN_4);
    Serial.println("signal > 0");
  }

}

/* The main page */
int i = 0;
void handle_root() {

  if (!server.hasArg("plain")) {
    server.send(200, "text/html", ROBOT_HTML);
    return;
  }

  String data = server.arg("plain");
  server.send(200, "text/html", data);
  
  DynamicJsonDocument document = strToJSON(data);

  String xLane = document["x_lane"];
  String yLane = document["y_lane"];
  String angleLane = document["angle_lane"];
  String centerXLane = document["center_X_Lane"];
  String centerYLane = document["center_Y_Lane"];
  double _angleLane = angleLane.toDouble();
  double angle = vectorAngle(xLane.toInt(), yLane.toInt());
  std::string d = "{\"angle\":"+ std::to_string(angle) + "}"; 
  Serial.println(i);
  Serial.println(d.c_str());
  server.send(200, "text/html", d.c_str());
  i = i + 1;

  //Serial.println(data);
  
  //motorControl(centerXLane.toInt());
  //control(centerXLane.toInt());
  // int signal = (int)u(centerXLane.toInt());
  // int speed = 90;
  // int s = speed - (int)(signal/2);
  // Serial.println(signal);
  // if (signal == 0) {
  //   motor(1, ENA, speed, IN_1, IN_2);
  //   motor(1, ENB, speed, IN_3, IN_4);
  //   //Serial.println("0");
  // } 
  // if (signal < 0) {
  //   motor(1, ENA, speed+signal, IN_1, IN_2);
  //   motor(1, ENB, speed, IN_3, IN_4);
  //   //Serial.println("signal < 0");
  // }
  //  if (signal > 0) {
  //   motor(1, ENA, speed, IN_1, IN_2);
  //   motor(1, ENB, speed+signal, IN_3, IN_4);
  //   //Serial.println("signal > 0");
  // }
  
  /** Move Foward */
  // if (60 <= _angleLane && _angleLane <= 80) {
  //   motor(1, ENA, 150, IN_1, IN_2);
  //   motor(1, ENB, 100, IN_3, IN_4);
  // } else {
  //   motor(0, ENA, 0, IN_1, IN_2);
  //   motor(0, ENB, 0, IN_3, IN_4);
  // }

}

/* When Page is not found */
void handle_notFound() {
  server.send(404, "text/html", "Not Found");
}

/* Setup Access */
void setup_AP() {

  /* WiFi Mode Access Point */
  WiFi.mode(WIFI_AP);

  /* Access Point Configuration */
  if (!WiFi.softAPConfig(localIP, gateway, mask))
  {
    Serial.println("AP configuration faild!");
  }

  /* Start Access Point */
  WiFi.softAP(ssid);
}


void setup() {
  /** Encoder Input PIN */
  pinMode(ENCODER_PIN, INPUT);

  /** Ultrasonic setup */
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  /** Pins for the first motor */
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(ENA, OUTPUT);

  /** Pins for the second motor */
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  /** Start Serial at port 115200 */
  Serial.begin(115200);

  /* Setup and configure Access Point */
  setup_AP();

  /* Server configuration */
  server.on("/", handle_root);
  server.onNotFound(handle_notFound);

  /* Server begin */
  server.begin();

}

void loop() {
  server.handleClient();
  //motor(1, ENA, 180, IN_1, IN_2);
  //motor(1, ENB, 70, IN_3, IN_4);
}