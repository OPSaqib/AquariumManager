//Libraries:
#include <ArduinoBLE.h>
#include <Arduino.h>
#include <TimeLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

//Network details
//#define WIFI_SSID "Network"
//#define WIFI_PASSWORD "123456789"

//API key
//#define API_KEY "AIzaSyA0xp_iiZUN9OxgY8n41yhlNHKT2fnexQ0"

//Database URL
//#define DATABASE_URL "https://aquariummanager-846c8-default-rtdb.europe-west1.firebasedatabase.app/" 

//Define Firebase Data object and for auth and config
//FirebaseData fbdo;
//FirebaseAuth auth;
//FirebaseConfig config;

//For Firebase
//unsigned long sendDataPrevMillis = 0;
//int count = 0;
//bool signupOK = false;

//Pins Init:
#define temperature_pin D2 

//PWM Pin For Heater:
int PWM_pin = 3;

//More vars and setup for temperature:
OneWire oneWire(temperature_pin); // setup a oneWire instance to communicate with any OneWire devices

DallasTemperature sensors(&oneWire); // pass our oneWire reference to Dallas Temperature sensor

//Other global vars:
bool y = true;

float temperature;

//PID variables:
int kp = 9.1;   
int ki = 0.3;   
int kd = 1.8;
int PID_p = 0;    
int PID_i = 0;    
int PID_d = 0;

//More variables (for) PID:
//Variables
float set_temperature = 10;  //what should be the temperature of the water...
float PID_error = 0;
float previous_error = 0;
float elapsedTime;
float Time;
float timePrev;
int PID_value = 0;

void setup() {

  //Begin serial:
  Serial.begin(9600);

  //Connect to wifi:
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  //Serial.println("connecting to wifi");

  //while (WiFi.status() != WL_CONNECTED) {
    //Serial.print(".");
    //delay(300);
  //}

  //Assign the API key
  //config.api_key = API_KEY;

  //Assign the database URL
  //config.database_url = DATABASE_URL;

  //Sign up to Firebase
  //if (Firebase.signUp(&config, &auth, "", "")){
    //Serial.println("ok");
    //signupOK = true;
  //} else {
    //Serial.println("error signup firebase");
  //}

  //Callback function for Firebase token generation
  //config.token_status_callback = tokenStatusCallback;
  
  //Begin Firebase!
  //Firebase.begin(&config, &auth);
  //Firebase.reconnectWiFi(true);

  //Serial.println();
  //Serial.print("Connected with IP: ");
  //Serial.println(WiFi.localIP());
  //Serial.println();

  //Start temperature sensing for temperature:
  sensors.begin();

  //Set pins for PH:
  pinMode(13,OUTPUT);

  Time = millis();
}
 
void loop() {

  float temp = checkTemperature();

  if (temp != -127) {
    temperature = checkTemperature();

    Serial.println(temperature);

    monitorTemperature();
  }

  //SENDING FIREBASE DATA TO FIRESTORE
  //if (Firebase.ready() && signupOK) {
    
    //send data, REPLACE Sensor/temp_data WITH DIRECTORY OF WHERE TO STORE DATA
    //if (Firebase.RTDB.setFloat(&fbdo, "Sensor/temp_data", temperature)) {  //REPLACE temperature WITH ANY VARIABLE WE WANT TO SEND
      //Serial.println("PASSED");
      //Serial.println("PATH: " + fbdo.dataPath());
      //Serial.println("TYPE: " + fbdo.dataType());
    //} else {
      //Serial.println("FAILED");
      //Serial.println("REASON: " + fbdo.errorReason());
    //}
  //}

}

//Get the temperature at a call to this method:
float checkTemperature() {

  // request temperature method called
  sensors.requestTemperatures();

  float returnTemp = sensors.getTempCByIndex(0);

  return returnTemp;

}

//To monitor temperature with PID:
void monitorTemperature() {

  //The error in temperature (to be altered)
  PID_error = set_temperature - temperature;

  //Calculate the P value
  PID_p = kp * PID_error;

  //Calculate the I value based on the error calculated earlier
  if(-3 < PID_error <3) {
    PID_i = PID_i + (ki * PID_error);
  }

  //For derivative we need real time to calculate speed change rate
  timePrev = Time;                            
  Time = millis();                            
  elapsedTime = (Time - timePrev) / 1000; 

  //Now we can calculate D
  PID_d = kd*((PID_error - previous_error)/elapsedTime);
  
  //Now the PID value is P + I + D
  PID_value = PID_p + PID_i + PID_d;

  //We define PWM range between 0 and 255
  if(PID_value < 0) {    
    PID_value = 0;    
  } else if (PID_value > 255) {    
    PID_value = 255;  
  }

  //Now we can write the PWM signal to the mosfet on digital pin D3
  analogWrite(PWM_pin,255-PID_value);

  previous_error = PID_error;

}
