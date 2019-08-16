# NMEA2000-VoyageDataRecorder

This is the develoment space for a NMEA 2000 Voyage Data Recorder based on an ESP32 with CAN bus and SD card.
For SD card details see examples in the ESP32 library

The logger is designed to be used with the OpenCPN Voyage Data Recorder Plugin.

Current features:

- Convert NMEA2000 to NMEA0183 (or alternatively to SeaSmart) messages
- Mutiplex serial data from port 2 to NMEA0183 (for AIS messages)
- Store NMEA0183 messages data permanently to SD card
- Optional storage as Seasmart data

Version 0.1: 16.08.2019
