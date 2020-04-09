

// Check the presence of 9 probes from setup()
void checkSensorPresence(void){
  for (int i=0; i<9; i++){
    if (sensors.isConnected(probeAddr[i])){
      probeAvail[i] = true;
    }
    else probeAvail[i] = false;
    }
}

// Read data block from RPi and verify CRC8
// If the CRC8 fails set crcFAIL to TRUE

void receiveRPiData(void){
  int inCount = Serial.available();
  if (inCount < 4) { return;}     //return if less than 4 bytes are avaialable
  if (inCount > 4) { Serial.write(" Input Buffer Overflow in function receiveData()"); Serial.println("");
    // must deal with buffer overrun
  }
  if (inCount == 4){    // read and store the buffr contents
      for (int i=0; i < 4; i++){
      RPirecBlock[i]=Serial.read();
      calcCRC = crc8(RPirecBlock, 3);
      }
 
  if ((RPirecBlock[3]) != calcCRC){     // check CRC8
    Serial.write("CRC FAILED in function receiveData()");
    Serial.println("");
    crcFAIL = true;
    // request data to be resent
    }
  }
}

void decodeRPiData(){
  // create decode strategy
  }


void getTempsF(void){

  sensors.requestTemperatures();            // required before .getTempX()
  for (int i=0; i<9; i++){                  // read all 8 temp sensors. Order of reading is fixed by DallasTemperature so be aware
  float temp = sensors.getTempF(probeAddr[i]);
  greenHouseTemperatures[i] = (int) temp;   // convert from float to int and store to greenHouseTemperature[]
  }

}

void controlHouseVent(void){
  int houseTemp   = greenHouseTemperatures[6];
  if (houseTemp >= houseVentOnTemp){
    digitalWrite(heatPin1, OFF);
    digitalWrite(heatPin2, OFF);
    digitalWrite(ventPin, ON);
  }
  if (houseTemp <= houseVentOffTemp){
    digitalWrite(ventPin, OFF);
  }
}

void controlHouseHeater(void){
  int houseTemp   = greenHouseTemperatures[6];
  if (houseTemp <= houseHeatOnTemp){
    digitalWrite(ventPin, OFF);
    digitalWrite(heatPin1, ON);
    digitalWrite(heatPin2, ON);
  }
  if (houseTemp >= houseHeatOffTemp){
    digitalWrite(heatPin1, OFF);
    digitalWrite(heatPin2, OFF);
  }
}


void waterPots(void){

        
        // Check if NTP returned a valid time
    if (UTC_hours >= 24) {return;}  // 24 is an invalid UTC value. UTC_hours was init to 25 so nothing is done until a valid time received
                                    // so no action until after the first successful getNTPtime()
                                   
        // check if UTC time matches first preset watering time
    if (waterON == false){          // check schedule if the waterON = false. If true then moving on..........
      if ((UTC_hours == waterSchedule[firstWatering]) && (UTC_minutes == 0)){
        waterON = true;
        WATER_int = 120000;             // first watering 6am is for 2 minutes
      }
    }
        // check if UTC time matches second preset watering time
    if (waterON == false){
      if ((UTC_hours == waterSchedule[secondWatering]) && (UTC_minutes == 0)){
        waterON = true;
        WATER_int = 60000;              // second watering 2pm is for 1 minute
      }
    }
    
    if ((waterON == true) && (wateringON == false)){        //waterON if inside watering window 
        wateringON = true;                                  // wateringON is true is watering is active
          digitalWrite(pot1pin, ON);
          digitalWrite(pot2pin, ON);
          digitalWrite(pot3pin, ON);
          digitalWrite(pot4pin, ON);
          digitalWrite(pot5pin, ON);
          
        WATER_lastRead_millis = millis();                   // init the wateringON timer start value
      }

    if ((waterON == true) && (wateringON == true)){
      
      bool endwatering = timer_lapsed(WATER);               // endwatering true is the watering time has expired
      if (endwatering == true){
        waterON = false;                                    // reset the waterON to false
        wateringON = false;                                 // 
          digitalWrite(pot1pin, OFF);
          digitalWrite(pot2pin, OFF);
          digitalWrite(pot3pin, OFF);
          digitalWrite(pot4pin, OFF);
          digitalWrite(pot5pin, OFF);
      }
    }
}

void printData(void){
  Serial.print("UTC time is:  ");
    if ((UTC_hours >= 24) || (UTC_minutes >= 60)){
      Serial.println("  INVALID TIME STAMP:");
    }
  Serial.print(UTC_hours);
  Serial.print(':');
  if (UTC_minutes < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
  Serial.println(UTC_minutes);

  if (wateringON == true){
    Serial.println("Watering is ACTIVE");
  }
    else Serial.println("Watering in INACTIVE");
  
    
  // print the temperatures
  for (int i=0; i<9; i++){
    Serial.print(greenHouseTemperatures[i]);
    Serial.print("  ");
  }
    Serial.println();
    Serial.println();
}

// These are the system timers. Using millis() function. Each timer has a NAME_int to define the length of time in ms.
// if current time - start time >= whatever_int then {}
// 
bool timer_lapsed(uint8_t PID){                             // timer. used for short interval scheduling 1sec- a few minutes

  if (PID == PROBE){
    if ((millis() - PROBE_lastRead_millis) >= PROBE_int){    // set to 10 sec
      PROBE_lastRead_millis = millis();
      return true;
    }
    else {return false;}
    }
  if (PID == WATER){
    if ((millis() - WATER_lastRead_millis) >= WATER_int){   // set to 2minutes
      WATER_lastRead_millis = millis();
      return true;
    }
    else {return false;}
    }
  if (PID == PRINT){
    if ((millis() - PRINT_lastRead_millis) >= PRINT_int){    // set to 10 sec
      PRINT_lastRead_millis = millis();
      return true;
    }
    else {return false;}
    }

  if (PID == RUNNING){
    if ((millis() - RUNNING_lastRead_millis) >= RUNNING_int){        // set to .5 sec
      RUNNING_lastRead_millis = millis();
      return true;
    }
    else {return false;}
    }
}

// CRC8 function for Arduino (c++)
uint8_t crc8( uint8_t *addr, uint8_t len) {
      uint8_t crc=0;
      for (uint8_t i=0; i<len;i++) {
         uint8_t inbyte = addr[i];
         for (uint8_t j=0;j<8;j++) {
             uint8_t mix = (crc ^ inbyte) & 0x01;
             crc >>= 1;
             if (mix) 
                crc ^= 0x8C;
         inbyte >>= 1;
      }
    }
   return crc;
}
