/*
 VibraSol code for Arduino  - 
 Purpose: 
 * Hallux Valgus treatment/prevention by using your own muscles without orthotics.
 * Fisio / Rehab
 * Idiopathic foot syndrome
 * Down syndrome walk teaching
 * Pronation detection (for marathon runners)
 *
 Disclaimer: Use at your own risk. Consult a podiatrist to calibrate variable "SENSOR_PRESSURE_LEVEL"
             To claibrate you can use the com port to monitor sensor output. 
 The circuit:
 -----------------------------------
 * 1.  3.3V Output - Resistive pressure sensor (placed in x position) - A0 input
 * 2.             inptut - 75K Ohm (purple red orange ) resistor - GND
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
 (x) Optimal placement of the pressure resisitve sensor to detect over-pronation. 
     Place on the opposite side (outer) if using right foot.
     One sensor on one leg is enough to give biofeedback if you mirror both legs.

 -----------------------------------
 Created by Jose Berengueres, Feb. 2014
 
 Pat. Pend. This code is in the public domain.
 Last revision v.4 of Feb 12th 2014
 Logs data to eeprom & debug keys by Michael Fritschi, Tiago Docilio
 +[mf, 19.2.14]: add settings for cheapduino (use Arduino NG/ATmega8 board settings)
 +[jb, 25.2.14]: fix bug that prevented bad posture waring while walking)
 */


#include <EEPROM.h>

#define SENSOR_PIN  0             // Sensor input pin

// Device specific definitions
#if defined(__AVR_ATmega8__)        // Settings for CheapDuino (Arduino NG/ATmega8A)
  #define EPROM_SIZE  511           // only 512bytes for cheapduino
  #define MOTOR_PIN   9             // ...where the vibration motor is connected to
#else
  #define EPROM_SIZE  1023          // 1024 BYTES in ARDUINO UNO        
  #define MOTOR_PIN   12            // 13 or 12 in some select models!!!!!!!!!!!!!!!!!!! ...where the vibration motor is connected to
#endif

// General definition/parameters
#define LIMIT                  30   // warn user if overpronated for longer that LIMIT samples sampled at 1/SAMPLE_TIME rate
#define OVERHEAD               4
#define SENSOR_PRESSURE_LEVEL  280  // controls min pressure to activate motor. It is sensor/shoe dependent jose 104
#define FOOT_ON_AIR            50   // ???  jose 20 vivobarefoot
#define SAMPLE_TIME            300  // Sampling rate [ms]
#define LOG_RATE               15   // time delay for logging bad-count [minutes]


unsigned int p = 0;                          // variable to store the value coming from the sensor
unsigned int cycle_warns = 0;             // total number of samples where p < th for more thanbadcount
unsigned int elapsed = 0;                    // number samples elapsed ina LOG_RATE period

unsigned int last_warn = 0;                  // when was the last time we warned a user
unsigned int log_addr = 0;                   // address index of last log

unsigned int p_h[LIMIT];
char whatsup;

boolean debug_flag = false;

// stats ---
unsigned int n_high = 0;
unsigned int n_low = 0;


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

// -- SETUP
void setup() {

  pinMode(MOTOR_PIN, OUTPUT); 
  Serial.begin(115200);

  log_addr = readW(0);
  Serial.print(   "number of logs in EEPROM = " );
  Serial.println(  log_addr - OVERHEAD    );
  
  Serial.print(   "user_id = " );
  Serial.println(  readW(2)    );
  hello();
  hello();

}

void hello(){
    vibrate(1,1000,1000);
}

void warn(){
  if ( elapsed - last_warn > LIMIT ) {
    vibrate(2,255,255);
    last_warn = elapsed;
    cycle_warns++;
    if (debug_flag){
      Serial.println( "Warning user ");
    }

  }
}

void malfunction(){
  vibrate(3,2000,300);
  Serial.println( "Error ");
}

void mem_full_warn(){
  vibrate(5,2000,300);
}

void vibrate(int n,int b, int c) {
  for(int i=0;i<n;i++){
    digitalWrite(MOTOR_PIN, HIGH);
    delay(b);
    digitalWrite(MOTOR_PIN, LOW);
    delay(c);
    
  }
}

void logit(){
 
    if ( log_addr < EPROM_SIZE  ) {
       if (cycle_warns > 255) {
          cycle_warns = 255;
       }
       log_addr ++;
       writeW(0,log_addr);
       delay(10);
       EEPROM.write(log_addr, cycle_warns);
       delay(10);
       if (debug_flag) {
         Serial.print("logging : ");
         Serial.print(cycle_warns);
         Serial.print(" at ");
         Serial.println(log_addr);
       }
       cycle_warns = 0;

    }else{
      if (debug_flag)
         Serial.println("MEM FULL(!) CANNOT LOG DATA");
      mem_full_warn();
    }

}

void sample() {
  delay(SAMPLE_TIME);
  listen();
  elapsed ++;
  p = analogRead(SENSOR_PIN);
  p_h[ elapsed % LIMIT ] = p;
  
  if (debug_flag) {
    Serial.print( " elapsed : "); 
    Serial.print( elapsed ); 
    //Serial.print( " p_zero : "); 
    //Serial.print( p_zero ); 
    Serial.print( " pressure : ");  
    Serial.print( p );
    //for (int i=0; i<LIMIT;i++) {    Serial.print( " :  " + p_h[i] );  }
  }
   
  if ( elapsed*(SAMPLE_TIME/100)  > (LOG_RATE*60*10) ){ // compared in 10ths of seconds. int range is 32k check saturation!
    logit();
    elapsed = 0;
    last_warn = 0;
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

  for (int i=OVERHEAD; i<log_addr; i++){
    Serial.print(i-3,DEC);
    Serial.print("\t");
    Serial.println(EEPROM.read(i),DEC);
  }
  Serial.println();
}

void listen(){

  whatsup = Serial.read();
  
  switch(whatsup) {
    
    case 'D': //Debug on
      debug_flag = true;
      break;
    case 'd': //Debug off
      debug_flag = false;
      break;
    case 'h': //Dump stored values to USART
      send_data_usart();
      break;
    case '0': //Reset EEPROM
      log_addr = OVERHEAD;      // reset log_add to start pos = OVERHEAD
      writeW( 0, OVERHEAD );   // reset first record pos  = OVERHEAD
      writeW( 2, 1 );         // default  uid = 1
      Serial.println("reset done");
      
      break;
  }
  

}

// -- MAIN LOOP

void loop() {
  
  sample();

  // -- stats  
  n_high = 0;
  n_low=0;
  for (int k=0; k<LIMIT;k++){
     if (p_h[k] > SENSOR_PRESSURE_LEVEL )  
       n_high++;
     if (p_h[k] < FOOT_ON_AIR           ) 
       n_low++;
     //if (debug_flag){ 
     //    Serial.print( ":");         
     //    Serial.print( p_h[k]);         
     //}
  }
  if (debug_flag){ 
     Serial.print( " n_high : ");
     Serial.print( n_high );
     Serial.print( " n_low : ");
     Serial.println( n_low );
     
  }

  // WARNING LOGIC --- 
  if ( n_high == 0 && (n_low < 25)  ) // out of LIMIT = 30
    warn();

}
