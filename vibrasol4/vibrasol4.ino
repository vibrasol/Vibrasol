/*
 VibraSol code for Arduino  - 
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
 Last revision v.4 of Feb 12th 2014
 Logs data to eeprom & debug keys by Michael Fritschi, Tiago Docilio
 */


#include <EEPROM.h>

int sensorPin   = A0;      // select the input pin for the potentiometer
int motorPin    = 13;     // select 13 for UNO board or 2 for NANO board

#define LIMIT  3         
int p = 0;                              // variable to store the value coming from the sensor
int th_correctposture = 105;             // controls min pressure to activate motor. It is sensor/shoe dependent
int foot_on_air = 20;             

float sampling_t = 300;     // samplign rate millis
int limit = LIMIT;        // warn user if overpronated for longer that sampling_t x limit

boolean badposture = false;  // temp variable
int warncount = 0;        // number warnings to user
int badcount = 0;         // temporary overpronation counter
int cyclebadcount = 0;    // total number of samples where p < th
int elapsed = 0;          // number samples elapsed
int log_addr = 0;         // address index of last log
int OVERHEAD = 4;
int EPROM_SIZE = 1023;   // 1024 BYTES in ARDUINO UNO        
int LOG_RATE =1;      // logs number of bad posture every LOG_RATE minutes;

char whatsup;
boolean debug_flag = false;

//  EPROM MAP by BYTE
//     0     1         2      3     4    5  6  7  8  9 10 11 .... EPROM_SIZE_1023 
//   ( address )     (user id )    (stats log : 1 Byte per every 15 minutes   )

void writeW(int where, int x) {
  int b1 = x % 255;
  int b0 = x /255;
  EEPROM.write(where, b0);
  EEPROM.write(where + 1, b1);
}

int readW( int from) {
  return  255 * EEPROM.read(from) + EEPROM.read(from+1);
  
}

void setup() {
  pinMode(motorPin, OUTPUT); 
  Serial.begin(115200);
  vibrate(2,300,65);
  // writeW(2,1); // UNCOMMENT THIS TO RESET LOGS ONLY
  // writeW(0,4); // UNCOMMENT THIS DO THIS TO SET UID

  log_addr = readW(0);
  Serial.print(   "number of logs in EEPROM = " );
  Serial.println(  log_addr - OVERHEAD    );
  
  Serial.print(   "user_id = " );
  Serial.println(  readW(2)    );
}

void vibrate(int n,int b, int c) {
  for(int i=0;i<n;i++){
    digitalWrite(motorPin, HIGH);
    delay(b);
    digitalWrite(motorPin, LOW);
    delay(c);
    warncount ++;
    
  }
}

void logit(){
 
    if ( log_addr < EPROM_SIZE  ) {
       if (cyclebadcount > 255) {
          cyclebadcount = 255;
       }
       log_addr ++;
       writeW(0,log_addr);
       delay(10);
       EEPROM.write(log_addr, cyclebadcount);
       delay(10);
       if (debug_flag) {
         Serial.print("logging : ");
         Serial.print(cyclebadcount);
         Serial.print(" at ");
         Serial.println(log_addr);
       }
       cyclebadcount = 0;

    }else{
       if (debug_flag)
         Serial.println("MEM FULL(!) CANNOT LOG DATA");
         vibrate(10,165,165);
         exit(-1);
    }

}

void sample() {
  delay(sampling_t);
  listen();
  elapsed ++;
  p = analogRead(sensorPin);
  if (debug_flag) {
    Serial.print( "log_adr : "); 
    Serial.print( log_addr ); 
    Serial.print( " elapsed : "); 
    Serial.print( elapsed ); 
    Serial.print( " pressure : ");  
    Serial.println( p );
   
  }

  if ( elapsed*(sampling_t/100)  > (LOG_RATE*60)){ //*10) ){ // compared in 10ths of seconds. int range is 32k check saturation!
    
    logit();
    elapsed = 0;
  }
  // --- END EEPROM -------------

}

void send_data_usart(){
  
  int i,j;
  
  Serial.println();

  Serial.print("log_addr :");
  Serial.print("\t");
  Serial.println(readW(0));

  Serial.print("User ID:");
  Serial.print("\t");
  Serial.println(readW(2));

  for (int i=OVERHEAD; i<=log_addr; i++){
    Serial.print(i-3,DEC);
    Serial.print("\t");
    Serial.println(EEPROM.read(i),DEC);
  }
  Serial.println();
}

void listen(){

  whatsup = Serial.read();
  
  switch(whatsup) {
    case 'D':
      debug_flag = true;
      break;
    case 'd':
      debug_flag = false;
      break;
    case 'h':
      send_data_usart();
      break;
    case '0':
      writeW(0,OVERHEAD);
      Serial.println("reset done");
      break;
  }
  

}

void loop() {
  
  sample();
 
  if (p < th_correctposture && !(p < foot_on_air)) {
     badposture = true;
     badcount ++;

  }else{
     badposture = false;
     badcount = 0;

  }
  
  if (badcount > limit) {
    cyclebadcount ++;
    badcount = 0;
    vibrate(3,65,65);
    if (debug_flag){
      Serial.println( "Warning user ");
    }

  }


}

