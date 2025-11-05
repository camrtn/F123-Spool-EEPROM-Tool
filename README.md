# F123-Spool-EEPROM-Tool
A software tool to interact with eeprom chips from material spools used with the stratasys F123 series of printers. 

Stratasys F123 series printers require a user to purchase spools of material directly from the company. I do not know of a workaround to be able to use cheap PLA or other materials from third party sources. This tool aims to rectify that and open up F123 series printers to use material not from Stratasys directly.

Stratasys achieves this "lockdown" of material by using a custom eeprom chip on each spool they produce. This chip tracks what kind of material is on the spool and how much remains. If this eeprom could be re-programmed to show 100% material remaining (from 0%), a user could theoretically load third-party material on a spool and re-use this eeprom to convince the printer it's using an official Stratasys spool. This would allow a user to use cheaper materials in F123 series printers.

I used an Adafruit Trinket M0 to interact with the material spool eeprom chips since I had a spare and it uses +3.3V logic levels. Background research and my experimentation revealed that the custom eeprom chips use a +3.3V logic level to communicate to the printer mainboard. The tool was written using the Arduino IDE and Arduino libraries to upload a sketch to the Trinket M0. 

The Data directory contains the following:
 1. Screenshots from the waveform captured of the communication between an eeprom chip and an F370 printer mainboard.
 2. Screenshots of the outputs from the Arduino program and its interaction with the eeprom chip.
 3. HEX data dumps read from the material spool eeprom chip by the Arduino program.

The Software directory contains the following:
1. An existing Arduino one wire library modified to be able to interact with the material spool eeprom chip.
2. The Arduino sketch I wrote to interact with the eeprom chip.

Circuit.png shows how I hooked up an Adafruit Trinket M0 to interact with the eeprom chip. To hook this circuit up to an oscilloscope and view the waveform of the tool's interaction with the eeprom, connect the GND part of the probe to the M0's GND pin and the data part of the probe between the eeprom chip +3.3V pin and M0's pin 4.

--------------------------------------------------------------------------
To view where I left this project navigate to Data/Hex-Dumps/Notes.txt
