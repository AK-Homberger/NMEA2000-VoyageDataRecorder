/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// ESP32 - NMEA2000 to NMEA0182 Multiplexer and SD Card Voyage Data Recorder
// Version 0.2, 16.08.2019, AK-Homberger

#include <Arduino.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <Seasmart.h>
#include <memory>
#include <N2kMessages.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>
#include <sys/time.h>
#include "N2kDataToNMEA0183.h"

#define ENABLE_DEBUG_LOG 1 // Debug log, set to 1 to enable forward on USB-Serial

int LED_BUILTIN = 1;
bool LedState=false;

bool SendNMEA0183Conversion = true; // Do we send/log NMEA2000 -> NMEA0183 conversion
bool SendSeaSmart = false; // Do we send/log NMEA2000 messages in SeaSmart format

long timezone = 1; 
byte daysavetime = 1;

tN2kDataToNMEA0183 tN2kDataToNMEA0183(&NMEA2000, 0);

// Forward declarations
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg);
void SendNMEA0183Message(const tNMEA0183Msg &NMEA0183Msg);

// Serial port 2 config (GPIO 16)
const int baudrate = 38400;
const int rs_config = SERIAL_8N1;

// Buffer config

#define MAX_NMEA0183_MESSAGE_SIZE 150 // For AIS
char buff[MAX_NMEA0183_MESSAGE_SIZE];

// NMEA message for AIS receiving and multiplexing
tNMEA0183Msg NMEA0183Msg;
tNMEA0183 NMEA0183;


void debug_log(char* str) {
#if ENABLE_DEBUG_LOG == 1
  Serial.println(str);
#endif
}

bool SDavailable = false;
long MyTime=0;


void setup() {

  pinMode (LED_BUILTIN, OUTPUT);
  
  // Init USB serial port
  Serial.begin(115200);

  // Init AIS serial port 2
  Serial2.begin(baudrate, rs_config);
  NMEA0183.Begin(&Serial2, 3, baudrate);

  // Start and Check SD Card

  SDavailable = SD.begin();
  if (!SDavailable) Serial.println("Card Mount Failed");
  else if (SDavailable && (SD.cardType() == CARD_NONE)) {
    Serial.println("No SD card attached");
    SDavailable = false;
  } else Serial.println("SD card attached");

  NMEA2000.AttachMsgHandler(&tN2kDataToNMEA0183); // NMEA 2000 -> NMEA 0183 conversion
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg); // Also send all NMEA2000 messages in SeaSmart format
  tN2kDataToNMEA0183.SetSendNMEA0183MessageCallback(SendNMEA0183Message);

  NMEA2000.Open();
}



// Write Buffer to SD Card

void WriteSD(const char * message) {
  char FileName[80] = "/NoValidDate.log";
  char DirName[15] =  "/";
  File dataFile;

  if (!SDavailable) return;  // No SD card, no logging!!!
  
  if (MyTime!=0) {
    time_t rawtime = MyTime; // Create Time from NMEA 2000 time
    struct tm  ts;
    ts = *localtime(&rawtime);
    strftime(DirName, sizeof(DirName), "/%Y-%m-%d", &ts); // Create directory name from date
    strftime(FileName, sizeof(FileName), "/%Y-%m-%d/%Y-%m-%d-%H.log", &ts); // Create Filname: Dir + Date + Hour runtime
  }    

  SD.mkdir(DirName);
  
  digitalWrite(LED_BUILTIN, LedState);
  LedState=!LedState;

  #if ENABLE_DEBUG_LOG == 1
    Serial.print(FileName);
    Serial.print(": ");
    Serial.println(message);
  #endif
   
  dataFile = SD.open(FileName, FILE_APPEND);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(message);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("Error opening log file");
  }
}


#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 500
//*****************************************************************************
//NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg & N2kMsg) {

  if ( !SendSeaSmart ) return;

  char buf[MAX_NMEA2000_MESSAGE_SEASMART_SIZE];
  if ( N2kToSeasmart(N2kMsg, millis(), buf, MAX_NMEA2000_MESSAGE_SEASMART_SIZE) == 0 ) return;
  WriteSD(buf);
}


//*****************************************************************************
void SendNMEA0183Message(const tNMEA0183Msg & NMEA0183Msg) {
  if ( !SendNMEA0183Conversion ) return;

  char buf[MAX_NMEA0183_MESSAGE_SIZE];
  if ( !NMEA0183Msg.GetMessage(buf, MAX_NMEA0183_MESSAGE_SIZE) ) return;
  WriteSD(buf);
}


void loop() {
  bool TimeSet=false;
  
  if (NMEA0183.GetMessage(NMEA0183Msg)) {  // Get AIS NMEA sentences from serial2
    SendNMEA0183Message(NMEA0183Msg);      // Send to SD Card
  }
  
  NMEA2000.ParseMessages();
  
  MyTime=tN2kDataToNMEA0183.Update();

  if(MyTime!=0 && !TimeSet){
    struct timeval tv;
    tv.tv_sec = MyTime;
    settimeofday(&tv, NULL);
    TimeSet=true;
  }
  
  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }
}

