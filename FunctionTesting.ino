

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
  
  if (inCount < 6) {
    return;
    }     //return if less than 4 bytes are avaialable
  
  if (inCount > 6) { 
    Serial.println("Input Buffer Overflow in function receiveRPiData()");
    // flush the input buffer
    while (Serial.available()) {
      Serial.read();
    }
    return;
  }
  
  if (inCount == 6){    // read and store the buffr contents
      for (int i=0; i < 6; i++){
      RPirecBlock[i]=Serial.read();
      }
  }
      
  if ((RPirecBlock[0] == 72) && (RPirecBlock[2] == 77) && (RPirecBlock[4] == 83)) {
    // DATA block received verified. Adjust time
    UTC_hours   = RPirecBlock[1];
    UTC_minutes = RPirecBlock[3];
    UTC_seconds = RPirecBlock[5];      
  }
}



void getTempsF(void){

  sensors.requestTemperatures();            // required before .getTempX()
  for (int i=0; i<9; i++){                  // read all 8 temp sensors. Order of reading is fixed by DallasTemperature so be aware
  float temp = sensors.getTempF(probeAddr[i]);
  greenHouseTemperatures[i] = (int) temp;   // convert from float to int and store to greenHouseTemperature[]
  }

}

void controlHouseVent(void){
  int houseTemp   = greenHouseTemperatures[0];
  if (houseTemp >= houseVentOnTemp){
    digitalWrite(heatPin1, OFF);
    digitalWrite(heatPin2, OFF);
    digitalWrite(ventPin, ON);
    ventON = true;
  }
  if (houseTemp <= houseVentOffTemp){
    digitalWrite(ventPin, OFF);
    ventON = false;
  }
}

void controlHouseHeater(void){
  int houseTemp   = greenHouseTemperatures[0];
  if (houseTemp <= houseHeatOnTemp){
    digitalWrite(ventPin, OFF);
    digitalWrite(heatPin1, ON);
    digitalWrite(heatPin2, ON);
    heaterON = true;
  }
  if (houseTemp >= houseHeatOffTemp){
    digitalWrite(heatPin1, OFF);
    digitalWrite(heatPin2, OFF);
    heaterON = false;
  }
}


void waterPots(void){
                                   
        // check if UTC time matches  preset watering time in waterSchedule[0,1]
    if (waterON == false){              // waterON = false if watering hasn't already been turned on
      if ((UTC_hours == waterSchedule[0]) && (UTC_minutes == waterSchedule[1])){
        waterON = true;                 // waterON = time to water. Doesn't mean that watering is active
        WATER_int = 60000;             // set 1 minute to water plants if schedule waterSchedule[0,1]
      }
    }
        // check if UTC time matches preset watering time in waterSchedule[2,3]
    if (waterON == false){
      if ((UTC_hours == waterSchedule[2]) && (UTC_minutes == waterSchedule[3])){
        waterON = true;
        WATER_int = 120000;              // set 2 minute to water plants if schedule waterSchedule[2,3]
      }
    }
    
    if ((waterON == true) && (wateringON == false)){        //waterON if inside watering window and wateringON is not active
        wateringON = true;                                  // wateringON is true when  watering is active
          digitalWrite(pot1pin, ON);                        // turn watering valves ON
          digitalWrite(pot2pin, ON);
          digitalWrite(pot3pin, ON);
          digitalWrite(pot4pin, ON);
          
          
        WATER_lastRead_millis = millis();                   // init the wateringON timer start value
      }

    if ((waterON == true) && (wateringON == true)){         // if already watering (valves ON)
      
      bool endwatering = timer_lapsed(WATER);               // endwatering true when watering time has expired
      if (endwatering == true){
        waterON = false;                                    // reset the waterON to false
        wateringON = false;                                 // turn off watering
          digitalWrite(pot1pin, OFF);
          digitalWrite(pot2pin, OFF);
          digitalWrite(pot3pin, OFF);
          digitalWrite(pot4pin, OFF);
          
      }
    }
}

void printData(void){
  Serial.print("TIME:  ");
  Serial.print(UTC_hours);
  Serial.print(':');
  if (UTC_minutes < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
  Serial.println(UTC_minutes);

  if (wateringON == true){
    Serial.println("Watering is ON");
  }
  else Serial.println("Watering is OFF");

  if (ventON == true){
    Serial.println("Venting  is ON");  
  }
  else Serial.println("Venting  is OFF");

  if (heaterON == true) {
    Serial.print("Heater   is ON..");
    }
  else Serial.println("Heater   is OFF..");  
    
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
