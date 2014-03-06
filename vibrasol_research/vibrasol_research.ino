/*
 VibraSol code for Arduino  - 
 * Hallux Valgus, flat foot, fallen arches neuromuscular retraining software
 * Over-Pronation detection (for marathon runners)
 *
 Disclaimer: Use at your own risk. Consult a podiatrist to calibrate variable "SENSOR_PRESSURE_LEVEL"
             To calibrate you can use the com port to monitor sensor output menu: d debug, h history, 0 for reset. 
 The circuit:
 -----------------------------------
 * 1.  3.3V Output - Resistive pressure sensor (placed in x position) - A0 input
 * 2.             inptut - 75K Ohm (purple red orange ) resistor - GND
 * 3.  Output 13 - Vibrator motor (flat coin type, in contact with skin, 5V) - GND
 
     _ _
   /    \                      (b) 
  |      |                     |
  |      |                     |
  |     |                    | | |
  |    |                     |(a)|
  (x)  |                     | | |
  |     |                   /  | |
  |     |             _  -     | | 
   \_ _/            (________(x)__)  
                      
 LEFT FOOTPRINT      LEG SIDE-VIEW

 (a) Strap Arduino (and vibrator) here
 (b) 4-pin usb cable to Nokia battery pack which is in a side pocket
 (x) Optimal placement of the pressure resisitve sensor to detect over-pronation/supination 
     Place on the opposite side (outer) if using right foot.
     One sensor on one leg is enough to give biofeedback if you mirror both legs.

 -----------------------------------
 Created by Jose Berengueres, Feb. 2014
 
 Pat. Pend. This code is in the public domain.
 Last revision v.4 of Feb 12th 2014
 Logs data to eeprom & debug keys by Michael Fritschi, Tiago Docilio
 +[mf, 19.2.14]: add settings for cheapduino (use Arduino NG/ATmega8 board settings)
 +[jb, 25.2.14]: fix bug that prevented bad posture tracking while walking)
 +[jb, 28.2.14]: Added step counter, avg, var and other cycle stats, increased logging rate to 3 minutes
  */


#include <EEPROM.h>

#define SENSOR_PIN  0             // Sensor input pin

// Device specific definitions  
#if defined(__AVR_ATmega8__)        // Settings for CheapDuino (Arduino NG/ATmega8A)
  #define EPROM_SIZE  511           // only 512bytes for cheapduino
  #define MOTOR_PIN   9             // ...where the vibration motor is connected to
#else
  #define EPROM_SIZE  1023          // 1024 BYTES and SRAM   2k bytes in ARDUINO UNO        
  #define MOTOR_PIN   12            // 13 or 12 in some select models!!!!!!!!!!!!!!!!!!! ...where the vibration motor is connected to
#endif

// General definition/parameters
#define LIMIT                  30   // warn user if overpronated for longer that LIMIT samples sampled at 1/SAMPLE_TIME rate
#define OVERHEAD               4
#define SENSOR_PRESSURE_LEVEL  90  //150  // controls min pressure to activate motor. It is sensor/shoe dependent jose 104 jose snadal 150
#define FOOT_ON_AIR            15   //50 ???  jose 20 vivobarefoot
#define SAMPLE_TIME            300  // Sampling rate [ms]
#define LOG_RATE               2.5    // time delay for logging bad-count [minutes]
#define N                      500   // ! Over 500 not enough SRAM!!   
                               // samples recordeed per cycle. Can be less that LOGRATE/SAMPLE_TIME .... 500 = 2.5 x 60s @(1/0.3) Hz 
                                     // A cycle LOG_RATE = SAMPLE TIME * N       
#define M                      8    // size of cycle write   
#define TH0                    1    // zero thereshold
#define CO                     1  // COMPRESION for sd , mean

unsigned int p = 0;                          // variable to store the value coming from the sensor
unsigned int cycle_warns = 0;             // total number of samples where p < th for more thanbadcount
unsigned int elapsed = 0;                    // number samples elapsed ina LOG_RATE period

unsigned int last_warn = 0;                  // when was the last time we warned a user
unsigned int log_addr = 0;                   // address index of last log

unsigned int p_h[N];
char whatsup;

boolean debug_flag = false;

// --- variables for cycle stats ---
unsigned int n_high = 0;
unsigned int n_low = 0;

unsigned int _cycleavg = 0.0;
unsigned int _sd = 0.0;
unsigned int _max = 0 ;
unsigned int _step = 0;
unsigned int _nhigh = 0;
unsigned int _nlow = 0;
unsigned int _cyclezeroes=0;
boolean foot_up = true;
boolean foot_down = false;

// ---  ---  ---  ---  ---  ---  --- 

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

  //send_data_usart();
  ///if (log_addr < (EPROM_SIZE -2*M) ) {
  //  send_data_usart();
  //  Serial.println( " Memory full - exiting");
  //  mem_full_warn();  
  //}

}

void hello(){
    vibrate(1,1000,1000);
}

void warn(){
  if ( elapsed - last_warn > LIMIT ) {
    //vibrate(2,255,255);
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
  vibrate(1,20000,300);
}

void vibrate(int n,int b, int c) {
  for(int i=0;i<n;i++){
    digitalWrite(MOTOR_PIN, HIGH);
    delay(b);
    digitalWrite(MOTOR_PIN, LOW);
    delay(c);
    
  }
}

void stepcounter(){
    if (foot_up &&   p >= FOOT_ON_AIR ) { foot_down = true; foot_up=false;  
    }else{ 
      if (foot_down && p < FOOT_ON_AIR ) {   
        foot_up = true; 
        foot_down =false;
        _step = _step + 1; 
        if ( _step> CO*255 ) 
          _step = CO*255;
      }
    }
}

void logit() {
  
  hello();
  if ( log_addr < (EPROM_SIZE-M)  ) {

       variance_stats();       

       EEPROM.write(log_addr, _cycleavg);
       delay(10);
       log_addr ++;       

       EEPROM.write(log_addr,  _sd);
       delay(10);
       log_addr ++;       

       if (cycle_warns > CO*255) {
          cycle_warns = CO*255;
       }
       EEPROM.write(log_addr, cycle_warns);
       delay(10);
       log_addr ++;

       EEPROM.write(log_addr, _max);
       delay(10);
       log_addr ++;

       EEPROM.write(log_addr, _step);
       delay(10);
       log_addr ++;

       EEPROM.write(log_addr, _nlow);
       delay(10);
       log_addr ++;
       EEPROM.write(log_addr, _nhigh);
       delay(10);
       log_addr ++;

       EEPROM.write(log_addr, _cyclezeroes);
       delay(10);
       log_addr ++;


       // UPDATE POS  --------     
       // check it is a multiple of M
       writeW(0,log_addr);
       delay(10);
       cycle_warns = 0;       _cycleavg =0;
       _sd =0;       _max = 0;
       _step = 0;       _nhigh = 0;
       _nlow = 0;    _cyclezeroes = 0;
       
    }else{
      mem_full_warn();
      if (debug_flag)
         Serial.println("MEM FULL(!) CANNOT LOG DATA");
    }

  }
  
void variance_stats(){
       short s = 0;
       float sd = 0;
       
       for(int j=0; j<N;j++ ){
          s = s + p_h[j];
       }
       
       float mean= s/N;
       if (mean > CO*255) {mean = CO*255; Serial.println("sd ... saturated"); }
       _cycleavg = (unsigned int)mean/CO;
  
       for(int j=0; j<N;j++){
          sd = sd + ((float)p_h[j]-mean)*((float)p_h[j]-mean);
       }
       sd = sqrt(sd /N);

       if (sd > CO*255) {sd = CO*255;  Serial.println("sd ... saturated");}
      _sd = (unsigned int)(sd/CO);
      if (debug_flag) {
             Serial.print( mean);
             Serial.println("");
             
             for(int j=0; j<N;j++){
               Serial.print( ((short)p_h[j]-mean)*((short)p_h[j]-mean));
               Serial.print("\t");
             }
             Serial.println("");

             for(int j=0; j<N;j++){
               Serial.print( p_h[j]);
               Serial.print("\t");
             }
             Serial.println("");

      }

      stepcounter();

}  

void debug_info(){
       //variance_stats();       
       if (debug_flag) {
         
         Serial.print(" avg ");
         Serial.print(_cycleavg);
         
         Serial.print(" sd ");
         Serial.print(_sd);

         Serial.print(" w ");
         Serial.print(cycle_warns);
 
         Serial.print(" max ");
         Serial.print( _max);

         Serial.print(" _step ");
         Serial.print(  _step);

         Serial.print(" _nhigh ");
         Serial.print(  _nhigh);
         
         Serial.print(" _nlow ");
         Serial.print(  _nlow);
         
         Serial.print(" down ");
         Serial.print(foot_down);
         
         Serial.print(" up ");
         Serial.print(foot_up);
         
         Serial.print(" at ");
         Serial.println(log_addr);
       }
       
}

void sample() {
  delay(SAMPLE_TIME);
  listen();
  elapsed ++;
  p =  analogRead(SENSOR_PIN);
 
  p_h[ elapsed %N] = p;
  
  if (debug_flag) {
    Serial.print( elapsed ); 
    Serial.print( " p : ");  
    Serial.print( p );
    debug_info();  
  }
   
  if ( elapsed*(SAMPLE_TIME/100)  > (LOG_RATE*60*10)  ){ // compared in 10ths of seconds. int range is 32k check saturation!
    logit();  // only logs the first ?  500 samples (150 s @ 1/3 Hz)
    elapsed = 0;
    last_warn = 0;
  }

}

void send_data_usart(){
  
  Serial.println();

  Serial.print("log_addr :");
  Serial.print("\t");
  Serial.println(readW(0));

  Serial.print("User ID:");
  Serial.print("\t");
  Serial.println(readW(2));

  Serial.println("");
  Serial.println("LIMIT \t SENSOR_PRESSURE_LEVEL \t FOOT_ON_AIR \t SAMPLE_TIME \t LOG_RATE \t N");
  Serial.print("\t");  Serial.print(LIMIT);
  Serial.print("\t");  Serial.print(SENSOR_PRESSURE_LEVEL);
  Serial.print("\t");  Serial.print(FOOT_ON_AIR);
  Serial.print("\t");  Serial.print(SAMPLE_TIME);
  Serial.print("\t");  Serial.print(LOG_RATE);
  Serial.print("\t");  Serial.println(N);
   
  int i = OVERHEAD;
  Serial.println("i\t avg\t sd\t nw\t max\t stp\t nl\t  nh\t  0s ");
  while  ( i<log_addr ){
    Serial.print(i-3,DEC);
    Serial.print("\t");
    for (int k=0; k<M ;k++ ){
      
      if (k < 2)
         Serial.print(CO*(short)EEPROM.read(i),DEC);
      else
         Serial.print(EEPROM.read(i),DEC);
      
      i++;
      Serial.print("\t");
    }
    Serial.println();
  }
  Serial.println();
}

void listen(){

  whatsup = Serial.read();
  
  switch(whatsup) {
    
    case 'D': //Debug on
      debug_flag = !debug_flag;
      break;
    case 'd': //Debug off
      debug_flag = !debug_flag;
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

  // --- WARNING LOGIC --- 
  n_high = 0;
  n_low=0;
  for (int k=0 ; k<LIMIT;k++){
    int j =(elapsed-k)%N; 
    if (p_h[j] > SENSOR_PRESSURE_LEVEL )  
       n_high++;
     if (p_h[j] < FOOT_ON_AIR           ) 
       n_low++;
  }
  if ( n_high == 0 && (n_low < 25)  )
    warn();
  
  // --- STATSD --- 
  
    if ( _max < p ) { _max = p; }
    if ( p < TH0 )  {  _cyclezeroes =_cyclezeroes+1;   }
    if ( p < FOOT_ON_AIR ) {  _nlow =_nlow + 1;   }
    if ( p > SENSOR_PRESSURE_LEVEL ) {  _nhigh = _nhigh + 1;   }
    stepcounter();

}
