# NMEA2000-VoyageDataRecorder

This is the develoment space for a NMEA 2000 Voyage Data Recorder based on an ESP32 with CAN bus and SD card.
The logger is using a SN65HVD230 CAN bus transceiver. See ESP32-SD.png for connection details for SD card and transceiver.

The logger is designed to be used with the OpenCPN Voyage Data Recorder Plugin.

Current features:

- Convert NMEA2000 to NMEA0183 (or alternatively to Seasmart) messages
- Store NMEA0183 messages data permanently to SD card
- Optional storage as Seasmart data
- Optional mutiplexing serial data from port 2 to NMEA0183 (e.g. for AIS messages). See WifiGateway for circuit for RS232 level conversion

Version 0.1: 16.08.2019
