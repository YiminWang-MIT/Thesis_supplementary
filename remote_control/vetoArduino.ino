#include <EEPROM.h>

//internal position stored in unite of steps
//output and input doubles in unit of mm 
static int UpStepCount=0;
static int DownStepCount=0;
static double conversion=126.0;//steps for 1mm (full steps)
void setup() {
  Serial.begin(9600); // set the baud rate
  // board one, upper motor
  pinMode(2, INPUT);//outer limit switch
  pinMode(3, INPUT);//inner limit switch
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  // board two, lower motor
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, INPUT);//outer limit
  pinMode(13, INPUT);//inner limit
  
  digitalWrite(4, LOW);//STEP
  digitalWrite(5, LOW);//DIR
  //The driver board can be see to move in 1/8 1/4 1/2 1 of one full step by MS1 and MS2
  //Here I set it in full step because it is quicker to move and accurate enough for the veto <0.01mm accuracy
  digitalWrite(6, LOW);//MS1 
  digitalWrite(7, LOW);//MS2
  digitalWrite(8, HIGH);//STEP
  digitalWrite(9, HIGH);//DIR
  digitalWrite(10, LOW);//MS1
  digitalWrite(11, LOW);//MS2
  //Serial.println("Ready"); // print "Ready" once
  //Store the internal position in memory that remains after power is off
  //~10K writing for lifetime, be careful not to put these lines in any loop
  UpStepCount=EEPROMReadShort(0);
  DownStepCount=EEPROMReadShort(2);

}

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

void loop() {
  String s;
  if(Serial.available()){ // only send data back if data has been sent
    s = Serial.readStringUntil('\n'); // read the incoming commands
    if (s=="read0"){
      Serial.print("pos ");
      Serial.println(UpStepCount/conversion);//convert internal steps to position in mm
    }
    else if (s=="read1"){
      Serial.print("pos");
      Serial.println(DownStepCount/conversion);
    }
    else{
      //command format
      //position in mm; space; bit for arm, 0 is upper, 1 is lower
      int len = s.length();
      int arm = s[len-1]-'0';
      s=s.substring(0,len-2);

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
          digitalWrite(9, HIGH);
        else
          digitalWrite(9, LOW);

      //avoid PMT touching check
  
      for (int i=0; i<abs(nstep); i++){
        if (arm==0 && digitalRead(2)==HIGH){//too far, move back 1 mm
          digitalWrite(5, LOW);
          for (int j=0; j<conversion; j++){
            UpStepCount++;
            digitalWrite(4, HIGH);
            delay(1);
            digitalWrite(4, LOW);
            delay(1);
          }
          UpStepCount=0;
          EEPROMWriteShort(0, UpStepCount);//update the position in memory
          break;
        }
        if (arm==0 && digitalRead(3)==HIGH){// too close, move back 1mm
          digitalWrite(5, HIGH);
          for (int j=0; j<conversion; j++){
            UpStepCount--;
            digitalWrite(4, HIGH);
            delay(1);
            digitalWrite(4, LOW);
            delay(1);
          }
          EEPROMWriteShort(0, UpStepCount);//update the position in memory
          break;
        }
        if (arm==1 && digitalRead(13)==HIGH){// too close, move back 1mm
          digitalWrite(9, HIGH);
          for (int j=0; j<conversion; j++){
            DownStepCount--;
            digitalWrite(8, HIGH);
            delay(1);
            digitalWrite(8, LOW);
            delay(1);
          }
          EEPROMWriteShort(2, DownStepCount);
          break;
        }
        if (arm==1 && digitalRead(12)==HIGH){//too far, move back 1 mm
          digitalWrite(9, LOW);
          for (int j=0; j<conversion; j++){
            DownStepCount++;
            digitalWrite(8, HIGH);
            delay(1);
            digitalWrite(8, LOW);
            delay(1);
          }
          DownStepCount=0;
          EEPROMWriteShort(2, DownStepCount);
          break;
        }

        //no comflict, just move the arm and update the internal number
        if (arm==0){
          UpStepCount+=stepnum;
          digitalWrite(4, HIGH);
          delay(1);
          digitalWrite(4, LOW);
          delay(1);
        } else {
          DownStepCount+=stepnum;
          digitalWrite(8, HIGH);
          delay(1);
          digitalWrite(8, LOW);
          delay(1);
        }
      }
      //after complete the movement
      //update the position in memory
      EEPROMWriteShort(0, UpStepCount);
      EEPROMWriteShort(2, DownStepCount);
      
      //Serial.println(UpStepCount);
      //Serial.println(DownStepCount);
    }
  }
  //delay(1); // delay for 1/10 of a second
}
