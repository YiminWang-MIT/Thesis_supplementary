#include <EEPROM.h>
//internal position stored in unit of steps (short is easier than float)
//output and input doubles in unit of mm 

// motor 0 should be the upper motor, motor 1 should be the lower motor
static int UpStepCount=0;
static int DownStepCount=0;
static float conversion=76.92;//steps for 1mm (1 step = 0.0005 inch = 0.013 mm)


//Executed once when power is on
void setup() {

  Serial.begin(9600); // set the baud rate
  
  //white CW/CCW, red Enable, green STEP

  //Motor 0
  pinMode(5, OUTPUT);//CW/CCW
  pinMode(6, OUTPUT);//Enable
  pinMode(7, OUTPUT);//STEP pulse
  
  //Motor 1
  pinMode(10, OUTPUT);//CW/CCW
  pinMode(11, OUTPUT);//Enable
  pinMode(12, OUTPUT);//STEP pulse
 
  //CW moves the tungsten further from the beamline
  //All default setting is towards the safe side
  digitalWrite(5, LOW);//CW 
  digitalWrite(6, LOW);//Enable is high, disable by default
  digitalWrite(7, LOW);//STEP

  digitalWrite(10, LOW);//CW 
  digitalWrite(11, LOW);//Enable is high, disable by default
  digitalWrite(12, LOW);//STEP

  //Store the internal position in memory that remains after power is off
  //~10K writing for lifetime, be careful not to put these lines in any stepping loop

  UpStepCount=EEPROMReadShort(0);
  DownStepCount=EEPROMReadShort(2);
}


//To write or read a value of short from the EEPROM
//The value is for position information
void EEPROMWriteShort(int address, short value){
  byte down = (value & 0xFF);
  byte up = ((value >> 8) & 0xFF);
  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, down);
  EEPROM.write(address + 1, up);
}

int EEPROMReadShort(int address){
  byte down = EEPROM.read(address);
  byte up = EEPROM.read(address+1);
  //read the 4 bytes into the eeprom memory.
  return ((up<<8) & 0xFFFF) + ((down<<0) & 0xFF);
}


//loop after setup
void loop() {
  String s;
  // Read commands : read0/ read1
  // Move commands : xxxxxxy
  // y specifies the motor
  // xxxxxx specifies the new position in mm
  // e.g. 3.11 means move motor 1 to position at 3.1 mm

  if(Serial.available()){ // only send data back if data has been sent
    s = Serial.readStringUntil('\n'); // read the incoming commands
    if (s=="read0"){
      Serial.print("pos ");
      Serial.println(UpStepCount/conversion);//convert internal steps to position in mm
    }
    else if (s=="read1"){
      Serial.print("pos ");
      Serial.println(DownStepCount/conversion);
    }
    else if (s=="reset0"){
      Serial.println("pos0 reset");
      UpStepCount=0;
      EEPROMWriteShort(0, UpStepCount);
    }
    else if (s=="reset1"){
      Serial.println("pos1 reset");
      DownStepCount=0;
      EEPROMWriteShort(2, DownStepCount);
    }
    else{
      //command format
      //position in mm; space; bit for arm, 0 is upper, 1 is lower
      int len = s.length();
      int arm = s[len-1]-'0';
      s=s.substring(0,len-1);

      int nstep = 0;
      if (arm==0) 
        nstep=s.toFloat()*conversion-UpStepCount;
      else 
        nstep=s.toFloat()*conversion-DownStepCount;



      //negative is farther from the beam line
      int stepnum=nstep>0? 1:-1;
      if (arm==0)
        if (nstep<0)
          //farther
          digitalWrite(5, HIGH);
        else
          digitalWrite(5, LOW);
      else
        if (nstep<0)
          //farther
          digitalWrite(10, HIGH);
        else
          digitalWrite(10, LOW);

      if (arm==0)
          digitalWrite(6, HIGH);
      else 
          digitalWrite(11, HIGH);
          
      for (int i=0; i<abs(nstep); i++){
          if (arm==0){

            UpStepCount+=stepnum;
            digitalWrite(7, HIGH);
            delay(1);
            digitalWrite(7, LOW);
            delay(1);
          } else {
            DownStepCount+=stepnum;
            digitalWrite(12, HIGH);
            delay(1);
            digitalWrite(12, LOW);
            delay(1);
          }
      }
      //after complete the movement
      //disable both
      //update the position in memory

      digitalWrite(6, LOW);
      digitalWrite(11, LOW);

      EEPROMWriteShort(0, UpStepCount);
      EEPROMWriteShort(2, DownStepCount);
      
      //Serial.println(UpStepCount);
      //Serial.println(DownStepCount);
    }
  }
  //delay(1); // delay for 1/10 of a second
}
