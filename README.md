# F123-Spool-EEPROM-Tool
A software tool to interact with eeprom chips from material spools used with the stratasys F123 series of printers. 

I used an Adafruit Trinket M0 to interact with the eeprom chips since it has +3.3V logic levels on its pins. The eeprom chips use a +3.3V logic level to communicate to the printer mainboard. The tool was written using the Arduino IDE and Arduino libraries to upload a sketch to the Trinket M0. 

The Data directory contains the following:
 1. Screenshots from the waveform captured of the communication between an eeprom chip and an F370 printer mainboard.
 2. Screenshots of the outputs from the Arduino program I wrote to interact with the eeprom chip.
 3. Data dumps of the HEX data read from the eeprom chip by the Arduino program I wrote.

The Software directory contains the following:
1. An existing Arduino one wire library I edited to be able to interact with the eeprom chip
2. The Arduino sketch I wrote to interact with the eeprom chip.

The Setup picture shows how I hooked up an Arduino UNO to interact with the eeprom chip. It also shows how to hook this circuit up to an oscilloscope to view the waveforms of the program's interaction with the eeprom.
