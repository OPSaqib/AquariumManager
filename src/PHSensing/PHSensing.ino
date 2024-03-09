// definition of variables
#define SensorPin A0          // the pH meter Analog output is connected with A0 pin of arduino
unsigned long int avgValue;  // store average value of PH sensor feedback
float b;
int buf[10];
int temp;

int ph;
 
void setup() {
  pinMode(13,OUTPUT);  

  // setup serial monitor
  Serial.begin(9600);  
  Serial.println("Ready");
}

void loop() {
  
  for(int i = 0; i < 10; i++) {     // get 10 sample value from the sensor for smooth the value
    buf[i]=analogRead(SensorPin);
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

  // or store it in a variable
  ph = (int) phValue;
 
}
