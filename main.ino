#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "NewPing.h"
#include <time.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <FS.h>

// Watson IoT connection details
#define MQTT_HOST "4bkej5.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 8883
#define MQTT_DEVICEID "d:4bkej5:ESP32S:dev01"
#define MQTT_USER "use-token-auth"
#define MQTT_TOKEN "93dash93cow10"
#define MQTT_TOPIC "iot-2/evt/status/fmt/json"
#define MQTT_TOPIC_DISPLAY "iot-2/cmd/display/fmt/json"
#define MQTT_TOPIC_INTERVAL "iot-2/cmd/interval/fmt/json"
#define CA_CERT_FILE "/rootCA_certificate.der"
#define KEY_FILE "/SecuredDev01_key.key"
#define CERT_FILE "/SecuredDev01_crt.der"

#define TZ_OFFSET -5
#define TX_DST  60

/* Store WiFi credentials in a character array */
char ssid[] = "Mike's Phone";
char pass[] = "likemike";

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 64
#define LOGO16_GLCD_WIDTH  128
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};

#if (SSD1306_LCDHEIGHT != 64)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

/* MQTT objects */
void callback(char* topic, byte* payload, unsigned int length);
WiFiClientSecure wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

/* JSON variables that hold data I guess... */
StaticJsonBuffer<100> jsonBuffer;
JsonObject& payload = jsonBuffer.createObject();
JsonObject& status = payload.createNestedObject("d");
StaticJsonBuffer<100> jsonReceiveBuffer;
static char msg[50];

//PIR Sensors
int inputPin = 26;               // choose the input pin (for front PIR sensor)
int inputPin2 = 25;             // choose the input pin (for back PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;                    // variable for reading the front pin status
int val2 = 0;                   // variable for reading the back pin status

// RANGE SETTINGS
#define CLOSE 5
int FAR = 100;
#define CENTER 65

int32_t ReportingInterval = 1;

// defines pins numbers for HC SR04 Sensor
const int trigPin1 = 12; // TRIGGER PIN   Sensor left
const int echoPin1 = 13; // ECHO PIN      Sensor left
const int trigPin2 = 2; // TRIGGER PIN   Sensor center
const int echoPin2 = 15; // ECHO PIN      Sensor center
const int trigPin3 = 32; // TRIGGER PIN   Sensor right
const int echoPin3 = 33; // ECHO PIN      Sensor right

NewPing sonar1(trigPin1, echoPin1, FAR);
NewPing sonar2(trigPin2, echoPin2, FAR);
NewPing sonar3(trigPin3, echoPin3, FAR);

float t = 0.0; // distance
float t2 = 0.0;
float t3 = 0.0;
float duration;
float duration2;
float duration3;
float distance;
float distance2;
float distance3;

//Vibration Motors
#define Left_Motor 19
#define Right_Motor 18

void callback(char* topic, byte* payload, unsigned int length) {
  /* Handle message that arrived*/
  Serial.print("Message arrived[");
  Serial.print(topic);
  Serial.print("] : ");

  payload[length] = 0;
  Serial.println((char*)payload);

  JsonObject& cmdData = jsonReceiveBuffer.parseObject((char*)payload);

  if (0 == strcmp(topic, MQTT_TOPIC_DISPLAY)) {
    if (cmdData.success()) {
      /*
       * Valid message was received
       */
       FAR = cmdData.get<int>("FAR");
       jsonReceiveBuffer.clear();
       Serial.print("Far dist changed");
    } else {
      Serial.println("Received invalid JSON data");
    }
  } else if (0 == strcmp(topic, MQTT_TOPIC_INTERVAL)) {
    if (cmdData.success()) {
      /*
       * Valid message was received
       */
       ReportingInterval = cmdData.get<int32_t>("Interval");
       Serial.print("Reporting Interval has been changed:");
       Serial.println(ReportingInterval);
       jsonReceiveBuffer.clear();
    } else {
      Serial.println("Unknown command received");
    }
  }
}

void setup()   {
//  SerialBT.begin("ESP32 connecting..."); //Bluetooth device name
//  Serial.println("The device started, now you can pair it with bluetooth!");
  
  Serial.begin(115200); // Starts the serial communication
  Serial.setTimeout(2000);
  while (!Serial) { }

  /*
   * Start WiFi connection
   */
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, pass);
   while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
   }
   Serial.println();
   Serial.println("WiFi Connected");

   /*
    * Connect to MQTT = IBM Watson IoT Platform
    */
    while(!mqtt.connected()) {
      if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
        Serial.println("MQTT Connected1");
        mqtt.subscribe(MQTT_TOPIC_DISPLAY);
        mqtt.subscribe(MQTT_TOPIC_INTERVAL);  
      } else {
        Serial.println("MQTT Failed to connect! ... retrying");
        delay(500);
     }
    }
  
  pinMode(trigPin1, OUTPUT); //Sets the trigPin as an Output
  pinMode(echoPin1, INPUT); //Sets the echoPin as an Input

  pinMode(trigPin2, OUTPUT); //Sets the trigPin as an Output
  pinMode(echoPin2, INPUT); //Sets the echoPin as an Input

  pinMode(trigPin3, OUTPUT); //Sets the trigPin as an Output
  pinMode(echoPin3, INPUT); //Sets the echoPin as an Input

  pinMode(Left_Motor, OUTPUT);// Sets motor pins as outputs
  pinMode(Right_Motor, OUTPUT);

  pinMode(inputPin, INPUT);  //Declare PIR sensor (Front) as input
  pinMode(inputPin2, INPUT); //Declare PIR sensor (Back) as input
  
  Serial.print("I am here");
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  display.clearDisplay();
}


void loop() {
  mqtt.loop();
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Connected2");
      // Should verify the certificates here - like in the startup function
      mqtt.subscribe(MQTT_TOPIC_DISPLAY);
      mqtt.subscribe(MQTT_TOPIC_INTERVAL);
      mqtt.loop();
    } else {
      Serial.println("MQTT Failed to connect!");
      delay(5000);
    }
  }
  
  display.clearDisplay();

  //PIR Front and PIR Back
  val = digitalRead(inputPin);
  val2 = digitalRead(inputPin2);
  
  if (val == HIGH) {
      oledText("IR FRONT!", 12, 8, 1, true);
      Serial.println("Motion detected!");
  }

  if (val2 == HIGH) {
      oledText("IR BACK!", 12, 24, 1, true);
      Serial.println("Motion2 detected!");
  }
  
  if (val == LOW) {
      oledText("", 12, 8, 1, true);
      Serial.println("Motion ended!");
      val = digitalRead(inputPin);
  }
  
  if (val2 == LOW) {
      oledText("", 12, 24, 1, true);
      Serial.println("Motion2 ended!");
      val2 = digitalRead(inputPin2);
  }

  //delay(50);
  
  //Sensor 1
  //Clears the trigPin
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);


  //Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);

  //Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin1, HIGH);

  distance = sonar1.ping_cm();
  t = distance;

  if (t < CENTER)
  {
     //Left Line
    display.drawLine(1, 16, 1, 64, WHITE);
    display.drawLine(2, 16, 2, 64, WHITE);
    display.drawLine(3, 16, 3, 64, WHITE);
    display.drawLine(4, 16, 4, 64, WHITE);

    digitalWrite(Left_Motor, HIGH);
    delay(500);
    digitalWrite(Left_Motor, LOW);
  }

  //Sensor 2
  //Clears the trigPin
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);


  //Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  //Reads the echoPin, returns the sound wave travel time in microseconds
  duration2 = pulseIn(echoPin2, HIGH);

  distance2 = sonar2.ping_cm();
  t2 = distance2;

  if (t2 < CENTER)
  {
    //Horizontal Line
    display.drawLine(12, 1, 116, 1, WHITE);
    display.drawLine(12, 2, 116, 2, WHITE);
    display.drawLine(12, 3, 116, 3, WHITE);
    display.drawLine(12, 4, 116, 4, WHITE);

    digitalWrite(Left_Motor, HIGH);
    digitalWrite(Right_Motor, HIGH);
    delay(500);
    digitalWrite(Left_Motor, LOW);
    digitalWrite(Right_Motor, LOW);
    

  }

  //Sensor 3
  // Clears the trigPin
  digitalWrite(trigPin3, LOW);
  delayMicroseconds(2);


  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin3, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin3, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration3 = pulseIn(echoPin3, HIGH);

  distance3 = sonar3.ping_cm();
  t3 = distance3;

  if (t3 < CENTER)
  {
    //Right Line
    display.drawLine(125, 16, 125, 64, WHITE);
    display.drawLine(126, 16, 126, 64, WHITE);
    display.drawLine(127, 16, 127, 64, WHITE);
    display.drawLine(128, 16, 128, 64, WHITE);

    digitalWrite(Right_Motor, HIGH);
    delay(500);
    digitalWrite(Right_Motor, LOW);
  }

  display.display();

//   Prints the distance on the Serial Monitor in JSON format
  status["Left"] = t;
  status["Center"] = t2;
  status["Right"] = t3;
  payload.printTo(msg, 250);
  Serial.println(msg);

  if (!mqtt.publish(MQTT_TOPIC, msg)) {
      Serial.println("MQTT Publish failed");
  }

  // Pause - but keep polling MQTT for incoming messages
  for (int32_t i = 0; i < ReportingInterval; i++) {
    mqtt.loop();
    delay(1000);
    Serial.print("ReportingInterval :");
    Serial.print(ReportingInterval);
    Serial.println();
  }
  
  delay(300);
}

void oledText(String text, int x, int y, int size, boolean d) {
  display.setTextSize(size);
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(text);
  if (d) {
    display.display();
  }

  delay(1);
  Serial.println();
  Serial.println();
}
