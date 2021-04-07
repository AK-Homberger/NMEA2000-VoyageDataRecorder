# NMEA2000 Voyage Data Recorder / Logger

This is the develoment space for a NMEA 2000 Voyage Data Recorder based on an ESP32 with CAN bus and SD card.
The logger is using a SN65HVD230 CAN bus transceiver. 

For the logger, I use the pins GPIO4 for CAN RX and GPIO2 for CAN TX. This is because GPIO5 is used for SD card interface.

![SD card pins](https://github.com/AK-Homberger/NMEA2000-VoyageDataRecorder/blob/master/ESP32-SD.png)

Current features:

- Convert NMEA2000 to NMEA0183 (or alternatively to Seasmart/Actisense/Canview)
- Store NMEA0183 messages data permanently to SD card (.log)
- Optional storage as Seasmart (.ssm) CANViewer (.can) or Actisense (.ebl) data
- Optional mutiplexing serial data from port 2 to NMEA0183 (e.g. for AIS messages). See WifiGateway for circuit for RS232 level conversion.


The logger is designed to be used with the OpenCPN Voyage Data Recorder Plugin (with .log files).

The .ebl and .log files can be read with the Actisense EBL Reader application (https://www.actisense.com/product/nmea-reader-and-ebl-reader/). The .can format can be read with the Yacht Devices CAN Log Viewer (https://www.yachtd.com/products/can_view.html). For Seasmart format (.ssm) I have not found a reader so far.

Configuration can be done with a config.txt file in SD root directory. Just add keywords (NMEA, Actisense, YDcan or Seasmart) to enable logging format. No config file means NMEA logging as default.

The code is based on the NMEA 2000 / NMEA0183 libraries from Timo Lappalainen (https://github.com/ttlappalainen). Download and install the following libraries from GitHub link above:

- NMEA2000
- NMEA2000_esp32
- NMEA0183

# Remove the 120 ohm resistor from the transceiver
For unknown reasons, many CAN bus transceivers for the ESP32 have a 120 Ohm resistor built into them. The resistor does not belong to the devices at the ends of the stub lines, but to the ends of the backbone cable.

Whether the transceiver contains a 120 ohm resistor can be determined either by looking at the [circuit diagram](https://github.com/AK-Homberger/NMEA2000-Workshop/blob/main/Docs/SN65HVD230%20CAN%20Board_SCH.pdf) or by measuring with the multimeter.

A knife tip is suitable for removing the SMD resistor. Use it to scratch or pry off the resistance. With the transceiver shown here, place the tip of the knife in front of the resistor (between the chip and the resistor) and support the knife against the chip. Then lever the tip against the resistance and use it to scratch / loosen / break the resistance. Whichever happens first.

![Transceiver](https://github.com/AK-Homberger/NMEA2000WifiGateway-with-ESP32/blob/master/CAN-Transceiver.jpg)

It then looks like the picture. Then measure to be on the safe side. Without a 120 ohm resistor, the multimeter shows approx. 75 kOhm.

# Updates
Version 0.6: 15.12.2019 - Added CAN pin definition

Version 0.5: 20.08.2019 - Added support for Yacht Devices CAN Log Viewer format (.can)

Version 0.4: 18.08.2019 - Added configuration via config.txt file on SD card

Version 0.3: 17.08.2019 - Allow logging of NMEA0183 (.log), Seasmart (.ssm) and Actisense format (.ebl).

Version 0.2: 16.08.2019 - Create directories per day, files per hour. Synchronising ESP32 time with NMEA2000 time

Version 0.1: 16.08.2019 - Initial version


