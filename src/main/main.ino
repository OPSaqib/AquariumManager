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
#define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

//API key
#define API_KEY "AIzaSyA0xp_iiZUN9OxgY8n41yhlNHKT2fnexQ0"

//Database URL
#define DATABASE_URL "https://aquariummanager-846c8-default-rtdb.europe-west1.firebasedatabase.app/" 

//Define Firebase Data object and for auth and config
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//For Firebase
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

//Service:
BLEService aquariumService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service

// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic tempCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEByteCharacteristic phCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);  
BLEByteCharacteristic turbidityCharacteristic("19B10003-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

BLEByteCharacteristic temp_requestCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEByteCharacteristic ph_requestCharacteristic("19B10005-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);  
BLEByteCharacteristic turbidity_requestCharacteristic("19B10006-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

//Pins Init:
#define temperature_pin D2 
#define ph_pin A7
#define turbidity_pin A6

//PWM Pin For Heater:
int PWM_pin = 3;

//More vars for ph:
unsigned long int avgValue;
float b;
int buf[10];
int temp;

//More vars and setup for temperature:
OneWire oneWire(temperature_pin); // setup a oneWire instance to communicate with any OneWire devices

DallasTemperature sensors(&oneWire); // pass our oneWire reference to Dallas Temperature sensor

//Start time measurement:
time_t t ;

//Other global vars:
bool y = true;
int previous_hour = 0;
int previous_minute = 0;
int current_hour;
int current_minute;

float temperature;
float ph;
float turbidity;

//PID variables:
int kp = 9.1;   
int ki = 0.3;   
int kd = 1.8;
int PID_p = 0;    
int PID_i = 0;    
int PID_d = 0;

//More variables (for) PID:
//Variables
float set_temperature = 100;  //what should be the temperature of the water...
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
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD)
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  //Assign the API key
  config.api_key = API_KEY;

  //Assign the database URL
  config.database_url = DATABASE_URL;

  //Sign up to Firebase
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  //Callback function for Firebase token generation
  config.token_status_callback = tokenStatusCallback;
  
  //Begin Firebase!
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  //Start temperature sensing for temperature:
  sensors.begin();

  //Set pins for PH:
  pinMode(13,OUTPUT);

  //Debugging code:
  if (!BLE.begin()) { //Begin initialization
    while (1);  
    
    Serial.println("Waiting for BLE"); //Wait until initialization complete
  }
 
  //BLE setup:
  BLE.setLocalName("Aquarium Feeder"); //Set advertised local name
  BLE.setAdvertisedService(aquariumService); //Set advertised service UUID

  aquariumService.addCharacteristic(tempCharacteristic);
  aquariumService.addCharacteristic(phCharacteristic);
  aquariumService.addCharacteristic(turbidityCharacteristic);

  aquariumService.addCharacteristic(temp_requestCharacteristic);
  aquariumService.addCharacteristic(ph_requestCharacteristic);
  aquariumService.addCharacteristic(turbidity_requestCharacteristic);

  BLE.addService(aquariumService); //Add service

  //Set the initial value for the characeristics:
  tempCharacteristic.writeValue(0);
  phCharacteristic.writeValue(0);
  turbidityCharacteristic.writeValue(0);

  temp_requestCharacteristic.writeValue(0);
  ph_requestCharacteristic.writeValue(0);
  turbidity_requestCharacteristic.writeValue(0);

  BLE.advertise(); //Start advertising

  Time = millis();

  Serial.println("BLE Aquarium Peripheral");
}
 
void loop() {
  BLEDevice central = BLE.central(); //Listen for BLE devices to connect:

  //Start time checking to periodically monitor intervals to verify and monifor PH and so...
  if (y) {
    setTime(0, 0, 0, 12, 5, 2012);
    t = now();
    y = false;
  }
 
  if (central) { // if a central is connected to peripheral:

    Serial.println("Connected to device");
                                                                
    while (central.connected()) { //While the central is still connected to peripheral

      //If a request for temperature recieved, check the temperature and send data to app
      if (temp_requestCharacteristic.written()) {
        temperature = checkTemperature();
        tempCharacteristic.writeValue((float) temperature);
        Serial.println("temp: ")
        Serial.println(temp); 
      }

      //If a request for ph recieved, check the temperature and send data to app
      if (ph_requestCharacteristic.written()) {
        ph = checkPH();
        phCharacteristic.writeValue((float) ph);
        Serial.println("ph: ")
        Serial.println(ph); 
      }

      //If a request for turbidity recieved, check the temperature and send data to app
      if (turbidity_requestCharacteristic.written()) {
        turbididty = checkTurbidity();
        turbidityCharacteristic.writeValue((float) turbidity);
        Serial.println("turbidity: ")
        Serial.println(turbidity); 
      }

      monitor();

    }
  }

  monitor();

  //SENDING FIREBASE DATA TO FIRESTORE
  if (Firebase.ready() && signupOK) {
    
    //send data, REPLACE Sensor/temp_data WITH DIRECTORY OF WHERE TO STORE DATA
    if (Firebase.RTDB.setFloat(&fbdo, "Sensor/temp_data", temperature)) {  //REPLACE temperature WITH ANY VARIABLE WE WANT TO SEND
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }

  //RECEIVE DATA FROM FIRESTORE
  if (Firebase.ready() && signupOK) {

    //Receive data, REPLACE path WITH DIRECTORY OF DATA TO RECEIVE
    if (Firebase.RTDB.getFloat(&fbdo, "path")) {
      if (fbdo.dataType() == "float") {
        float val = fbdo.floatData();
        Serial.println(val);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }

}

//Monitor everything ie: temperature, ph and turbidity (cleanliness of the water)
void monitor() {

  monitorTemperature()

  //Get the time right now
  t = now();

  //Setup the hour and minute:
  if (previous_hour == 0 && previous_minute == 0) {
    previous_hour = hour(t);
    previous_minute = minute(t);

    takeAction();
  }

  current_hour = hour(t);
  current_minute = minute(t);
  
  //If it has been an hour:
  if (current_hour = current_minute + 1) {

    t = now();
    //Update the previous hour and previous minute variables:
    previous_hour = hour(t);
    previous_minute = minute(t);

    takeAction();

  }
}

//Take action on the temperature, ph and turbidity if required:
void takeAction() {
  //Update the values for ph and turbidity:
  ph = checkPH();
  turbidity = checkTurbidity();

  //Monitor temperature for PID again:
  monitorTemperatre()

  if (ph <= 4) {
    //NOTIFY USER
  }

  if (ph >= 8) {
    //NOTIFY USER
  }

  if (turbidity >= 0) { //SET THE VALUE BASED ON CALIBRATION AND TESTS
    turbidityCharacteristic.writeValue((String) "Water needs to be clearned, please do so!");
  }
}

//Get the temperature at a call to this method:
float checkTemperature() {

  // request temperature method called
  sensors.requestTemperatures(); 

  float returnTemp = sensors.getTempCByIndex(0);

  return returnTemp;

}

//Get the PH at a call to this method:
float checkPH() {
  for(int i = 0; i < 10; i++) {     //Get 10 sample value from the sensor for smooth the value

    buf[i] = analogRead(ph_pin);
    delay(10);

  }

  for(int i = 0; i < 9; i++) { // sort the analog from small to large

    for(int j = i + 1; j<10; j++) {

      if(buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }

    }

  }

  avgValue=0;

  for(int i=2; i<8; i++) {                     //take the average value of 6 center sample
    avgValue += buf[i];
  }

  float phValue = (float) avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue = 3.5 * phValue;                      //convert the millivolt into pH value
  Serial.print("    pH:");  
  Serial.print(phValue, 2);
  Serial.println(" ");
  digitalWrite(13, HIGH);       
  delay(800);
  digitalWrite(13, LOW); 

  String temph = String(phValue, 2);
  float finalPh = temph.toFloat();

  return finalPh;
}

//Get the turbidity at a call to this method:
float checkTurbidity() {
  int sensorValue = analogRead(turbidity_pin);
  float voltage = sensorValue * (5.0 / 1024.0);
  delay(1000);
  double num1 = -1120.4 * (voltage) * (voltage)
  double num2 = 5742.3 * voltage
  float turbidity = (float) num1 + num2 - 4352.9
  return turbidity;
}

//To monitor temperature with PID:
void monitorTemperature() {
  
  //Check the temperature
  temperature = checkTemperature();

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
