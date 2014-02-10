/*
 VibraSol code for Arduino - 
 Purpose: Hallux Valgus treatment/prevention by using your own muscles without orthotics.
 Disclaimer: Use at your own risk. Consult a podiatrist to calibrate variable "th_correctposture"
             To claibrate you can use the com port to monitor sensor output. 
 The circuit:
 -----------------------------------
 * 1.  3.3V Output - Resistive pressure sensor (placed in x position) - A0 input
 * 2.  A0 inptut - 75K Ohm (purple red orange ) resistor - GND
 * 3.  Output 13 - Vibrator motor (flat coin type, in contact with skin, 5V) - GND
 
     _ _
   /     \                     (b) 
  |       |                    |
  |       |                    |
  |       |                  | | |
  |      |                   |(a)|
  (x)   |                    | | |
  |     |                   /  | |
  |     |             _  -     | | 
   \_ _/            (________(x)__)  
                      
 LEFT FOOTPRINT      LEG SIDE-VIEW

 (a) Strap Arduino (and vibrator) here
 (b) 4-pin usb cable to Nokia battery pack which is in a side pocket
 (x) Optimal placement of the pressure resisitve sensor to detect over-pronation. win
     Place on the opposite side (outer) if using right foot.
     One sensor on one leg is enough to give biofeedback if you mirror both legs.

 -----------------------------------
 Created by Jose Berengueres, Feb. 2014
 Pat. Pend. This code is in the public domain.
 Last revision v.3 of Feb 10th 2014
 To do : to log elapsed, totalbadcount in csv format every 10,000 elapsed ticks

 */

#include <SimpleTimer.h>

int sensorPin   = A0;      // select the input pin for the potentiometer
int motorPin    = 13;     // select the pin for the LED

#define LIMIT  30         
int p = 0;                              // variable to store the value coming from the sensor
int th_correctposture = 105;             // controls min pressure to activate motor. It is sensor/shoe dependent
int foot_on_air = 20;             

int sampling_t = 300;     // samplign rate millis
int limit = LIMIT;        // warn user if overpronated for longer that sampling_t x limit

boolean badposture = false;
int warncount = 0;        // number warnings to user
int badcount = 0;         // temporary overpronation counter
int totalbadcount = 0;    // total number of samples where p < th
int elapsed = 0;          // number samples elapsed

void setup() {
  pinMode(motorPin, OUTPUT); 
  Serial.begin(9600); 
}

void vibrate() {
  for(int i=0;i<3;i++){
    digitalWrite(motorPin, HIGH);
    delay(65);
    digitalWrite(motorPin, LOW);
    delay(65);
    warncount ++;
    
  }
}

void sample() {
  delay(sampling_t);
  elapsed ++;
  p = analogRead(sensorPin);
  Serial.print( "pressure: ");  
  Serial.println( p );

}

void loop() {
  
  sample();

  if (p < th_correctposture && !p < foot_on_air) {
     badposture = true;
     badcount ++;
     totalbadcount ++;

  }else{
     badposture = false;
     badcount = 0;

  }
  
  if (badcount > limit) {
    vibrate();
    Serial.println( "Warning user ");  
    badcount = 0;

  }


}
