
//************************************************************************************************************
// This program is forked from GreenHouseTesting. ALL references to WiFi removed. Function is the same
// Instead of using NTP to get time, the time will be send from RPi via serial

//************************************************************************************************************
//  CURRENT WORK

// Moved printData() to (timer_lapsed(PROBE) == true)
// 
// Change watering to 3 zone PLUS a flush

// 
//*************************************************************************************************************
// PINS>
// 3,4    Heater (parallel circuit)
// 5      Vent
// A0-A4  Water pots 1-5
// 13     RUN LED
//*************************************************************************************************************

//************************************************************************************************************
// DESIRED function

//  Each Pot to have its own cycle
//  waterPots() should adjust watering based on tent and external temperatures. Hotter = more water ??
//  need to add an override watering switch to manually water/soak

//************************************************************************************************************
// IMPORTANT NOTES

// SAMD5x can sink/sourse 8mA
// Using analog pins A0-A4 for zone watering

//************************************************************************************************************


#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2                 // OneWire connected to pin2 on Uno
#define TEMPERATURE_PRECISION 12       //Set precision of the 18B20's

#define zone1WaterPin   A0                   
#define zone2WaterPin   A1
#define zone3WaterPin   A2
#define zone4WaterPin   A3
#define flushpin        A4                  // not used
#define ventPin         5
#define heatPin1        3
#define heatPin2        4
#define LEDpin          13

#define OFF HIGH                          // Active LOW inputs on the external relay board
#define ON  LOW                           // Active LOW inputs on the external relay board

// for the timer and time
#define PROBE 0x01
unsigned long PROBE_int = 60000;            // 1 minute read temp interval
#define PRINT 0x02
unsigned long PRINT_int = 10000;            // NOT USED. printData() is currently inside of timer_lapsed(PROBE)
#define WATER 0x03
unsigned long WATER_int = 120000;           // 2 minute watering DURATION timer. Water ON for 2 minutes
#define RUNNING   0x04
unsigned long RUNNING_int   = 250;          // LED toggle only indicates prgram running

// init the timers                       
unsigned long PROBE_lastRead_millis;
unsigned long PRINT_lastRead_millis;
unsigned long WATER_lastRead_millis;
unsigned long RUNNING_lastRead_millis;

// Variables to hold current time from decodeTime()
int UTC_hours   = 25;                    // init to 25 to prevent actions until a valid NTP time is received. 25 is invalid time.
int UTC_minutes = 65;                    // same as above
int UTC_seconds = 0;                     //


int waterSchedule[]   {6,0,18,55};       // 24 hour clock. {H,M,H,M.....}
bool wateringON     = false;             // true if watering is active... i.e. water valves are open
bool waterON        = false;             // true if it is time to water
bool heaterON       = false;
bool ventON         = false;

bool  crcFAIL = false;                  // CRC used in serial communication with RPi.  NOT USED currently


const int houseHeatOnTemp   = 60;
const int houseHeatOffTemp  = 63;
const int houseVentOnTemp   = 80;
const int houseVentOffTemp  = 77;
const int houseWARNTemp     = 90;

// memory for communication from RPi
uint8_t   RPirecBlock[16];                              // Data block received from RPi. 15 bytes for data, one byte for CRC
uint8_t   RPirecCRC;                                    // CRC included with received data block (from RPi)
uint8_t   calcCRC;                                      // calculated CRC8 of the data received from RPi

// Temperature sensors
OneWire oneWire(ONE_WIRE_BUS);            // create OneWire instance on pin2
DallasTemperature sensors(&oneWire);      // pass onewire instance to Dallas

// These are the ID's of the 5-probe temperature probe assembly (pre-made assembly  fixed addresses) and 4 extra probes
DeviceAddress probe1 = { 0x28, 0xFF, 0xC3, 0x0D, 0x81, 0x14, 0x02, 0x1B };    // for Greenhouse Temp
DeviceAddress probe2 = { 0x28, 0xFF, 0x7D, 0x65, 0x81, 0x14, 0x02, 0x7A };    // pot1 Temp
DeviceAddress probe3 = { 0x28, 0xFF, 0xD2, 0x5C, 0x30, 0x17, 0x04, 0x62 };    // pot2
DeviceAddress probe4 = { 0x28, 0xFF, 0xDB, 0x57, 0x81, 0x14, 0x02, 0x9A };    // pot3 Temp
DeviceAddress probe5 = { 0x28, 0xFF, 0x7B, 0xF6, 0x80, 0x14, 0x02, 0x24 };    // pot4
// 4 additional temperature probes
DeviceAddress probe6 = { 0x28, 0xAA, 0x4B, 0x76, 0x53, 0x14, 0x01, 0xA3 };    // purge water temp
DeviceAddress probe7 = { 0x28, 0xAA, 0x66, 0x74, 0x53, 0x14, 0x01, 0xB0 };    // greenhouse temp
DeviceAddress probe8 = { 0x28, 0xAA, 0xEA, 0x79, 0x53, 0x14, 0x01, 0xC7 };    // outside air temp
DeviceAddress probe9 = { 0x28, 0xAA, 0x28, 0x7B, 0x53, 0x14, 0x01, 0x61 };    // extra probe

uint8_t*    probeAddr[]  {probe1, probe2, probe3, probe4, probe5, probe6, probe7, probe8, probe9};     // indexes to each of the 8 temp probes
bool        probeAvail[] {probe1, probe2, probe3, probe4, probe5, probe6, probe7, probe8, probe9};     // available YES NO
int         greenHouseTemperatures [9];  // values from each of the 9 indexed probes (probe1...etc)
int         greenhouseMaxTemp[5] = {0,0,0,0,0};             // init to 0
int         greenhouseMinTemp[5] = {100,100,100,100,100};   // init to 100

void setup() {

  // Open serial communications
  Serial.begin(9600);

  pinMode(zone1WaterPin, OUTPUT);                   // pin A0 as GPIO OUTPUT
  digitalWrite(zone1WaterPin, OFF);
  pinMode(zone2WaterPin, OUTPUT);                   // pin A1
  digitalWrite(zone2WaterPin, OFF);
  pinMode(zone3WaterPin, OUTPUT);                   // pin A2
  digitalWrite(zone3WaterPin, OFF);
  pinMode(zone4WaterPin, OUTPUT);                   // pin A3
  digitalWrite(zone4WaterPin, OFF);
  pinMode(flushpin, OUTPUT);                   // pin A4
  digitalWrite(flushpin, OFF);
  pinMode(ventPin, OUTPUT);                    // pin 6
  digitalWrite(ventPin, OFF);
  pinMode(heatPin1, OUTPUT);                   // pin 3
  digitalWrite(heatPin1, OFF);
  pinMode(heatPin2, OUTPUT);                   // pin 4
  digitalWrite(heatPin2, OFF);
  pinMode(LEDpin, OUTPUT);                     // pin 13
  digitalWrite(LEDpin, LOW);

  sensors.begin();    // Start Dallas 18B20 on oneWire


  // start the timers. Used to schedule function calls at interval
  // should be a better way to init the timers
  PROBE_lastRead_millis       = millis();
  PRINT_lastRead_millis       = millis();
  WATER_lastRead_millis       = millis();
  RUNNING_lastRead_millis     = millis();

  // Start first temperature conversion
  getTempsF();        // first call to get temperatures. Using this instead of delay(1000) since the getTempsF() takes ~ 1 sec.
}

void loop() {

  // TOGGLE the run state LED
  if (timer_lapsed(RUNNING) == true){
    digitalWrite(LEDpin, !digitalRead(LEDpin));
  }

  receiveRPiData();
  
  // check to see if it's time to water
  waterPots();

  // get probe temps every PROBE_int
  if (timer_lapsed(PROBE) == true) {  // read temps every PROBE_int
    getTempsF();                      // This function take a LOT of time.. 830ms if using all 9 sensors!
    recordMinMax();                   // Record the daily Min and Max temps. Resets at midnight
    printData();                      // print the TIME and temps
  }

  // Control the venting
  controlHouseVent();

  // Control the heating
  controlHouseHeater();               //  Heat if too cold
 
  if (timer_lapsed(PRINT) == true) {  // print Time and Temp data to Serial
    //printData();
  }

}
