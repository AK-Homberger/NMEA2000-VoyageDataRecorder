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
// Version 0.5, 19.08.2019, AK-Homberger

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
#include <N2kMsg.h>
#include "N2kDataToNMEA0183.h"

#define MaxActisenseMsgBuf 500
#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 500

#define ENABLE_DEBUG_LOG 1 // Debug log, set to 1 to enable forward on USB-Serial

#define EBL_Escape 0x10
#define EBL_StartOfText 0x02
#define EBL_EndOfText 0x03


#define MsgTypeN2k 0x93


#define LOG_TYPE_NMEA 1
#define LOG_TYPE_ACTISENSE 2
#define LOG_TYPE_SEASMART 3
#define LOG_TYPE_YDCAN 4


int LED_BUILTIN = 1;
bool LedState = false;

bool SendNMEA0183Conversion = true; // Do we send/log NMEA2000 -> NMEA0183 conversion
bool SendSeaSmart = false; // Do we send/log NMEA2000 messages in SeaSmart format
bool SendActisense = false; // Do we send/log NMEA2000 messages in Actisense .ebl format
bool SendYD_Can = false; // Do we send/log NMEA2000 messages Yacht Devices .can format


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
long MyTime = 0;
long MyLine=0;

//*****************************************************************************
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
  } else {
    Serial.println("SD card attached");
    ReadConfigFile();
  }

  NMEA2000.AttachMsgHandler(&tN2kDataToNMEA0183); // NMEA 2000 -> NMEA 0183 conversion
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg); // Also send all NMEA2000 messages in SeaSmart and Actisense format
  tN2kDataToNMEA0183.SetSendNMEA0183MessageCallback(SendNMEA0183Message);

  NMEA2000.Open();
}

//*****************************************************************************
void ReadConfigFile() {

  char buf[80] = "";
  int i = 0;
  const char delim[] = " \n,";
  char *rest = NULL;
  char *token;

  File file = SD.open("/config.txt");
  if (!file) return;  // No config file

  SendNMEA0183Conversion = false;  // Config file available set all options to false
  SendActisense = false;
  SendSeaSmart = false;
  SendYD_Can = false;

  while (file.available() && (i < sizeof(buf) - 1) ) {
    buf[i] = file.read();
    i++;
  }
  buf[i] = '\0';

  file.close();

  for (token = strtok_r(buf, delim, &rest); token != NULL; token = strtok_r(NULL, delim, &rest)) {

    if ((int)token[strlen(token) - 1] == 13) token[strlen(token) - 1] = 0; // Remove Line Feed character from token

    if (strcasecmp(token, "NMEA") == 0) SendNMEA0183Conversion = true;
    if (strcasecmp(token, "Actisense") == 0) SendActisense = true;
    if (strcasecmp(token, "Seasmart") == 0) SendSeaSmart = true;
    if (strcasecmp(token, "YDCAN") == 0) SendYD_Can = true;
  }

#if ENABLE_DEBUG_LOG == 1

  Serial.print("NMEA="); Serial.println(SendNMEA0183Conversion);
  Serial.print("Actisense="); Serial.println(SendActisense);
  Serial.print("Seasmart="); Serial.println(SendSeaSmart);
  Serial.print("YDCan="); Serial.println(SendYD_Can);

#endif
}


//*****************************************************************************
void MyAddByteEscapedToBuf(unsigned char byteToAdd, uint16_t &idx, unsigned char *buf, uint16_t &byteSum)
{
  buf[idx++] = byteToAdd;
  byteSum += byteToAdd;

  if (byteToAdd == EBL_Escape) {
    buf[idx++] = EBL_Escape;
  }
}

//*****************************************************************************
size_t N2kToActisense(const tN2kMsg &msg, unsigned char *ActisenseMsgBuf, size_t size) {

  uint16_t msgIdx = 0;
  uint16_t byteSum = 0;
  uint8_t CheckSum =0;

  unsigned long _PGN = msg.PGN;
  uint32_t _MsgTime = 1; msg.MsgTime;
  unsigned int DataLen = msg.DataLen;
  
  if (size < MaxActisenseMsgBuf) return (0);
  if (DataLen+11 > MaxActisenseMsgBuf) return (0);

  ActisenseMsgBuf[msgIdx++] = EBL_Escape;
  ActisenseMsgBuf[msgIdx++] = EBL_StartOfText;
  
  MyAddByteEscapedToBuf(MsgTypeN2k, msgIdx, ActisenseMsgBuf, byteSum);
  
  MyAddByteEscapedToBuf(DataLen + 11, msgIdx, ActisenseMsgBuf, byteSum); //length does not include escaped chars
  
  MyAddByteEscapedToBuf(msg.Priority, msgIdx, ActisenseMsgBuf, byteSum);
  
  MyAddByteEscapedToBuf(_PGN & 0xff, msgIdx, ActisenseMsgBuf, byteSum); _PGN >>= 8;
  MyAddByteEscapedToBuf(_PGN & 0xff, msgIdx, ActisenseMsgBuf, byteSum); _PGN >>= 8;
  MyAddByteEscapedToBuf(_PGN & 0xff, msgIdx, ActisenseMsgBuf, byteSum);
  
  MyAddByteEscapedToBuf(msg.Destination, msgIdx, ActisenseMsgBuf, byteSum);
  MyAddByteEscapedToBuf(msg.Source, msgIdx, ActisenseMsgBuf, byteSum);

  MyAddByteEscapedToBuf(_MsgTime & 0xff, msgIdx, ActisenseMsgBuf, byteSum); _MsgTime >>= 8;
  MyAddByteEscapedToBuf(_MsgTime & 0xff, msgIdx, ActisenseMsgBuf, byteSum); _MsgTime >>= 8;
  MyAddByteEscapedToBuf(_MsgTime & 0xff, msgIdx, ActisenseMsgBuf, byteSum); _MsgTime >>= 8;
  MyAddByteEscapedToBuf(_MsgTime & 0xff, msgIdx, ActisenseMsgBuf, byteSum);
    
  MyAddByteEscapedToBuf(DataLen, msgIdx, ActisenseMsgBuf, byteSum);

  for (int i = 0; i < DataLen; i++) MyAddByteEscapedToBuf(msg.Data[i], msgIdx, ActisenseMsgBuf, byteSum);
  byteSum %= 256;

  CheckSum = (uint8_t)((byteSum == 0) ? 0 : (256 - byteSum));
  ActisenseMsgBuf[msgIdx++] = CheckSum;
  if (CheckSum == EBL_Escape) ActisenseMsgBuf[msgIdx++] = CheckSum;

  ActisenseMsgBuf[msgIdx++] = EBL_Escape;
  ActisenseMsgBuf[msgIdx++] = EBL_EndOfText;

  return (msgIdx);
}


//*****************************************************************************
void CreateYD_SeviceMsg(unsigned char *MsgBuf) {
  int i = 0;
  const char ServiceMsg[] = "YDVR v05";
  uint16_t Word = 0;

  Word = Word |= ((7) & 7) << 11; // Set data length-1 bits 12 - 14 - Service Messege is 8 byte
  Word = Word |= ((millis() / 1000 / 60 ) & 1023); // Set minutes bits 1 - 10

  MsgBuf[0] = Word & 0xff;
  MsgBuf[1] = Word >> 8;

  Word = millis() % 60000;

  MsgBuf[2] = Word & 0xff;
  MsgBuf[3] = Word >> 8;

  MsgBuf[4] = 0xff;
  MsgBuf[5] = 0xff;
  MsgBuf[4] = 0xff;
  MsgBuf[7] = 0xff;

  for (i = 0; i < 8; i++) MsgBuf[8 + i] = ServiceMsg[i];

}

//*****************************************************************************
void N2kToYD_Can(const tN2kMsg &msg, unsigned char *MsgBuf) {
  int i = 0;

  uint16_t Word = 0;
  uint32_t Dword = 0;

  Word = Word |= ((msg.DataLen - 1) & 7) << 11; // Set data length bits 12 - 14
  Word = Word |= ((millis() / 1000 / 60 ) & 1023); // Set minutes bits 1 - 10

  MsgBuf[0] = Word & 0xff;
  MsgBuf[1] = Word >> 8;

  Word = millis() % 60000;

  MsgBuf[2] = Word & 0xff;
  MsgBuf[3] = Word >> 8;

  MsgBuf[4] = msg.Source;

  Dword = msg.PGN;

  MsgBuf[5] = Dword & 0xff;
  MsgBuf[6] = (Dword >> 8) & 0xff;
  MsgBuf[7] = (Dword >> 16) & 0xff;

  for (i = 0; i < 8; i++) MsgBuf[8 + i] = 0xff;
  for (i = 0; i < msg.DataLen; i++) MsgBuf[8 + i] = msg.Data[i];
}


//*****************************************************************************
// Test if file exists
bool TestFileExists(char * FileName) {
  File dataFile;

  dataFile = SD.open(FileName, FILE_READ);

  if (!dataFile) return (false);
  dataFile.close();
  return (true);

}


//*****************************************************************************
// Write Buffer to SD Card

void WriteSD(const char * message, int len, int logtype) {
  char FileName[80] = "/NoValidDate.log";
  char DirName[15] =  "/";
  File dataFile;
  unsigned char ServiceMessage[20];

  if (!SDavailable) return;  // No SD card, no logging!!!

  if (MyTime != 0) {
    time_t rawtime = MyTime; // Create time from NMEA 2000 time (UTC)
    struct tm  ts;
    ts = *localtime(&rawtime);

    strftime(DirName, sizeof(DirName), "/%Y-%m-%d", &ts); // Create directory name from date

    if ( logtype == LOG_TYPE_NMEA ) strftime(FileName, sizeof(FileName), "/%Y-%m-%d/%Y-%m-%d-%H.log", &ts); // Create Filname: Dir + Date + Hour
    if ( logtype == LOG_TYPE_SEASMART ) strftime(FileName, sizeof(FileName), "/%Y-%m-%d/%Y-%m-%d-%H.ssm", &ts); // Create Filname: Dir + Date + Hour
    if ( logtype == LOG_TYPE_ACTISENSE ) strftime(FileName, sizeof(FileName), "/%Y-%m-%d/%Y-%m-%d-%H.ebl", &ts); // Create Filname: Dir + Date + Hour
    if ( logtype == LOG_TYPE_YDCAN ) strftime(FileName, sizeof(FileName), "/%Y-%m-%d/%Y-%m-%d-%H.can", &ts); // Create Filname: Dir + Date + Hour
  } else MyLine=1;

  SD.mkdir(DirName);

  if (logtype == LOG_TYPE_YDCAN && TestFileExists(FileName) == false) {  // New file, create service message for .can, must be first entry in log
    CreateYD_SeviceMsg(ServiceMessage);
    dataFile = SD.open(FileName, FILE_WRITE);
    dataFile.write(ServiceMessage, 16);
    dataFile.close();
  }

  digitalWrite(LED_BUILTIN, LedState);
  LedState = !LedState;

#if ENABLE_DEBUG_LOG == 1
  Serial.print(FileName);
  Serial.print(" : ");
  Serial.print(MyLine++);
  Serial.print(" : ");  
  Serial.print(" Len: ");
  Serial.print(len);
  Serial.print(" : ");
  
  if (logtype == LOG_TYPE_SEASMART || logtype == LOG_TYPE_NMEA) {
    Serial.println(message);
  }
  else {
    int i = 0;
    for (i = 0; i < len; i++) {
      Serial.printf("%02x ", (unsigned int)message[i]);
    }
    Serial.println();
  }
#endif

  dataFile = SD.open(FileName, FILE_APPEND);

  // if the file is available, write to it:
  if (dataFile) {
    if (logtype == LOG_TYPE_ACTISENSE || logtype == LOG_TYPE_YDCAN) dataFile.write((unsigned char*)message, len);
    else dataFile.println(message);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("Error opening log file");
  }
}


//*****************************************************************************
//NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) {
  int size = 0;

  char buf[MAX_NMEA2000_MESSAGE_SEASMART_SIZE]; //Actisense max size is smaller

  if (SendSeaSmart) {
    if ( N2kToSeasmart(N2kMsg, millis(), buf, MAX_NMEA2000_MESSAGE_SEASMART_SIZE) > 0 ) WriteSD(buf, strlen(buf), LOG_TYPE_SEASMART);
  }

  if (SendActisense) {
    size = N2kToActisense(N2kMsg, (unsigned char *) buf, MaxActisenseMsgBuf);
    if ( size == 0 ) return;
    WriteSD(buf, size, LOG_TYPE_ACTISENSE);
  }

  if (SendYD_Can) {
    N2kToYD_Can(N2kMsg, (unsigned char *) buf);
    WriteSD(buf, 16, LOG_TYPE_YDCAN);
  }

}


//*****************************************************************************
void SendNMEA0183Message(const tNMEA0183Msg & NMEA0183Msg) {
  if ( !SendNMEA0183Conversion ) return;

  char buf[MAX_NMEA0183_MESSAGE_SIZE];
  if ( !NMEA0183Msg.GetMessage(buf, MAX_NMEA0183_MESSAGE_SIZE) ) return;
  WriteSD(buf, strlen(buf), LOG_TYPE_NMEA);
}



//*****************************************************************************
void loop() {
  bool TimeSet = false;

  if (NMEA0183.GetMessage(NMEA0183Msg)) {  // Get AIS NMEA sentences from serial2
    SendNMEA0183Message(NMEA0183Msg);      // Send to SD Card
  }

  NMEA2000.ParseMessages();

  MyTime = tN2kDataToNMEA0183.Update();

  if (MyTime != 0 && !TimeSet) { // Set system time from valid NMEA2000 time (only once)
    struct timeval tv;
    tv.tv_sec = MyTime;
    settimeofday(&tv, NULL);
    TimeSet = true;
  }

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }
}

