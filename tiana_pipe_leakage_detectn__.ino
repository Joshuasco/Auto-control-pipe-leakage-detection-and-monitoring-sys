//IMPORT LIBRARIES
#include <WiFi.h>    
#include <HTTPClient.h>
#include <UrlEncode.h>
//END IMPORT LIBRARIES

//SETUP WIFI SSID AND PWD
const char* ssid = "YHEMEE"; 
const char* password = "ADEMI2023"; 
//END SETUP WIFI SSID AND PWD

//SETUP WHASAPP PHONE NUMBER AND API KEY
// +international_country_code + phone number
String phoneNumber = "+2348038520698"; 
String apiKey = "9970049";
String msg;
//END SETUP WHASAPP PHONE NUMBER AND API KEY

//BEGIN PIN DECLARATION AND ASSIGNMENT
// Define the pins to which the flow sensors are connected
const int flowSensorPin1 = 16;
const int flowSensorPin2 = 17;

const int buzzerPin = 4;      // pin for a buzzer
const int relayPin = 25;       // pin for a relay to control solenoid valve and pump
const int relay2Pin = 32;       // pin for a relay2 to control solenoid valve and pump

// Variables to store flow sensor data for Sensor 1
volatile int pulseCount1 = 0;
float flowRate1 = 0.0;

// Variables to store flow sensor data for Sensor 2
volatile int pulseCount2 = 0;
float flowRate2 = 0.0;

// Variables for timing
unsigned long oldTime = 0;


// Variables for logging data timing
unsigned long oldLogTime = 0;
//unsigned long currentLogTime;

// Threshold for detecting a leakage (adjust as needed)
const float leakageThreshold = 0.27; // Adjust this value based on your setup

bool noLeakage = true;
//END PIN DECLARATION AND ASSIGNMENT


//BEGIN SEND MESSAGE FUNCTION
void sendMessage(String message){

  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);    
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Message sent successfully");
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // end HTTP POST request
  http.end();
}

//END SEND MESSAGE FUNCTION


//BEGIN PULSECOUNTER FUNCTION FOR FLOW RATE READINGS
// Function to handle interrupt from the flow sensor for Sensor 1
void IRAM_ATTR pulseCounter1() {
  pulseCount1++;
}

// Function to handle interrupt from the flow sensor for Sensor 2
void IRAM_ATTR pulseCounter2() {
  pulseCount2++;
}
//BEGIN PULSECOUNTER FUNCTION FOR FLOW RATE READINGS


//ALARM FUNCTION
void send_alarm(){
  tone(buzzerPin, 90, 500);
  delay(2500);
  noTone(buzzerPin);
  }
//END ALARM FUNCTION


 //WATER PUMP/ SOLENOID CONTROL FUNCTION
  void solenoidOff(){
  digitalWrite(relayPin, LOW);
  }
  
  void solenoidOn(){
     digitalWrite(relayPin, HIGH);
    }
    //end solenoid function
    
     void waterPumpOff(){
  digitalWrite(relay2Pin, LOW);
  }
  
  void waterPumpOn(){
     digitalWrite(relay2Pin, HIGH);
    }
    //end water pump function
  //END WATER PUMP/ SOLENOID CONTROL FUNCTION


void setup() {
  // Start serial communication
  Serial.begin(115200);

  //begin wifi setup
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  //end wifi setup

  // Set up the flow sensor pins as inputs with internal pull-up resistors
  pinMode(flowSensorPin1, INPUT_PULLUP);
  pinMode(flowSensorPin2, INPUT_PULLUP);

  // Attach interrupts to the flow sensor pins
  attachInterrupt(digitalPinToInterrupt(flowSensorPin1), pulseCounter1, FALLING);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin2), pulseCounter2, FALLING);

  // Set up the buzzer pin
  pinMode(buzzerPin, OUTPUT);

  // Set up the relay pin for solenoid valve and water pump control
  pinMode(relayPin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  
  solenoidOn(); // Initially turn on the solenoid valve 
  waterPumpOn(); // Initially turn on the  pump
}


void loop() {

  // Calculate time since the last calculation
  unsigned long currentTime = millis();
  unsigned long currentLogTime=millis();
  unsigned long elapsedTime = currentTime - oldTime;
  unsigned long elapsedLogTime = currentLogTime - oldLogTime;

  // Update flow rates every second
  if (elapsedTime >= 1000) {
    // Calculate flow rate for Sensor 1 in liters per minute
    flowRate1 = (1000.0 / elapsedTime) * pulseCount1 / 7.5;

    // Calculate flow rate for Sensor 2 in liters per minute
    flowRate2 = (1000.0 / elapsedTime) * pulseCount2 / 7.5;

    // Print flow rates and total liters on the serial monitor
    Serial.print("Flow Rate1: "); 
    Serial.print(flowRate1);
    Serial.print(" L/min\t");

    Serial.print("Flow Rate2: ");
    Serial.print(flowRate2);
    Serial.print(" L/min\t");

    // Check for leakage
    float flowRateDifference = abs(flowRate1 - flowRate2);

    Serial.print("difference = ");
    Serial.print(flowRateDifference);
    Serial.print(" L/min\t");
 
    //message to logg to whatsapp
    msg = "flow Rate1 = "+String(flowRate1)+"\n"+"flow Rate2 = "+String(flowRate2)+"\n"+"Difference = "+String(flowRateDifference)+"\n"+"Threshold = "+String(leakageThreshold)+"\n";
    
    if (flowRateDifference > leakageThreshold) {

      if(millis()>=60000){// wait for 1min for normalize flow before detecting leakage
      Serial.println("Possible leakage detected!\n");
      
      solenoidOff();   // Turn off solenoid valve
      waterPumpOff();  // Turn off pump
      send_alarm();   // Trigger alarm 
      sendMessage(msg+"leakage detected, water cutoff \n");  //send alert msg on whatsapp
      noLeakage=false;
      
      }

    } else {
      // Turn on solenoid valve
      Serial.println("No leakage detected!");
      solenoidOn();
      waterPumpOn(); 
//      Serial.print("currentLogTime = ");
//       Serial.println(currentLogTime);
//        Serial.print("elapsedLogTime = ");
//       Serial.println(elapsedLogTime);
//        Serial.print("oldLogTime = ");
//       Serial.println(oldLogTime);
      //log datas to whatsapp evry 1min
    if(elapsedLogTime >= 60000){
      Serial.print("Sending Message to Whatsapp....");
      sendMessage(msg+"No leakage Detected\n");
      oldLogTime=currentLogTime;
       Serial.println("No leakage detected! 60 secs = 1min reached");
      }
    //end log datas to whatsapp evry 30sec
    }
    
    // Reset pulse counts and update old time
    pulseCount1 = 0;
    pulseCount2 = 0;
    oldTime = currentTime;
       
  }

}
