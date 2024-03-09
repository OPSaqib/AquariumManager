// relevant libraries
#include <OneWire.h>
#include <DallasTemperature.h>
 
// connect the yellow wire of temperature sensor to any pin on Arduino
#define temp_sensor_pin 12
 
// setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(temp_sensor_pin);
 
// pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// variable to store temperature
int temp;
 
void setup() {
  // init serial monitor
  Serial.begin(9600);
  
  // start temperature sensing
  sensors.begin();
}
 
void loop() { 
  // request temperature method called
  sensors.requestTemperatures(); 
  
  // print temperatures in the serial monitor
  Serial.print("Celsius temperature: ");
  Serial.print(sensors.getTempCByIndex(0)); 
  Serial.print(" - Fahrenheit temperature: ");
  Serial.println(sensors.getTempFByIndex(0));
  delay(1000);

  // or store it in a variable
  temp = sensors.getTempCByIndex(0);
}
