# NMEA2000-VoyageDataRecorder

This is the develoment space for a NMEA 2000 Voyage Data Recorder based on an ESP32 with CAN bus and SD card.
The logger is using a SN65HVD230 CAN bus transceiver.

The shown CAN bus ports of the ESP32 (GPIO 02/04) are valid for an AzDelivery ESP32 NODE MCU development board. Other ESP32 derivates may use different ports!

![SD card pins](https://github.com/AK-Homberger/NMEA2000-VoyageDataRecorder/blob/master/ESP32-SD.png)

Current features:

- Convert NMEA2000 to NMEA0183 (or alternatively to Seasmart/Actisense)
- Store NMEA0183 messages data permanently to SD card (.log)
- Optional storage as Seasmart (.ssm) CANViewer (.can) or Actisense (.ebl) data
- Optional mutiplexing serial data from port 2 to NMEA0183 (e.g. for AIS messages). See WifiGateway for circuit for RS232 level conversion.


The logger is designed to be used with the OpenCPN Voyage Data Recorder Plugin (with .log files).

The .ebl and .log files can be read with the Actisense EBL Reader application (https://www.actisense.com/product/nmea-reader-and-ebl-reader/). The .can format can be read with the Yacht Devices CAN Log Viewer (https://www.yachtd.com/products/can_view.html). For Seasmart format (.ssm) I have not found a reader so far.

Configuration can be done with a config.txt file in SD root directory. Just add keywords (NMEA, Actisense, DYcan or Seasmart) to enable logging format. No config file means NMEA logging as default.

The code is based on the NMEA 2000 library from Timo Lappalainen (https://github.com/ttlappalainen/NMEA2000).

# Updates
Version 0.5: 20.08.2019 - Added support for Digital Yacht format CANViewer(.can)

Version 0.4: 18.08.2019 - Added configuration via config.txt file on SD card

Version 0.3: 17.08.2019 - Allow logging of NMEA0183 (.log), Seasmart (.ssm) and Actisense format (.ebl).

Version 0.2: 16.08.2019 - Create directories per day, files per hour. Synchronising ESP32 time with NMEA2000 time

Version 0.1: 16.08.2019 - Initial version


