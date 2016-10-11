/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
int outputvalue = 255;
int counter = 0;
int output_d = -1;
int algorithm = 0;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
}

// the loop routine runs over and over again forever:
void loop() {
  outputvalue = outputvalue + output_d;
  analogWrite(3, outputvalue);
  delay(5);
  
  if( outputvalue == 0 ){
    outputvalue = 255;
    delay(300);
  }
}

