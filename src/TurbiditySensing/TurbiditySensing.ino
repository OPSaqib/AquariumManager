// init variables
#define turbidity_sensor A1

int turbidity_value;

void setup() {
  // setup serial monitor
  Serial.begin(9600);
 
}

void loop() {

  int sensorValue = analogRead(turbidity_sensor);
  float voltage = sensorValue * (5.0 / 1024.0);
 
  // print turbidity sensor results

  Serial.println ("Sensor Output (V):");
  Serial.println (voltage);
  Serial.println();
  delay(1000);

  // or store in a variable
  turbidity_value = (int) voltage;
}
