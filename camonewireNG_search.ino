#define CONFIG_CRC16_ENABLED  //assume eeprom uses CRC16 scheme
#define PARASITE_POWER true   //enable parasite power

#include <CamOneWireNg_CurrentPlatform.h>
#include <Adafruit_DotStar.h> //library to control main led on board

//definitions needed to control on board led
#define NUMPIXELS 1
#define DATAPIN   7
#define CLOCKPIN  8
Adafruit_DotStar strip = Adafruit_DotStar( NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

//required to use Serial.* commands with M0
#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

/*
 * 1-wire bus pin number.
 */
#ifndef OW_PIN
# define OW_PIN         4
#endif

/* eeprom function commands */
#define WRITE_SCRATCHPAD    0x0F
#define COPY_SCRATCHPAD     0x55
#define READ_SCRATCHPAD     0xAA
#define READ_MEMORY         0xF0
#define SKIP_ROM            0xCC

bool print = 0; //global flag that set how many times to run a print loop
bool debug = 0; //flag that decides whether to show debugging messages

#if !CONFIG_SEARCH_ENABLED
# error "CONFIG_SEARCH_ENABLED is required"
#endif

#if !CONFIG_CRC16_ENABLED
# error "CONFIG_CRC16_ENABLED is required"
#endif

#if (CONFIG_MAX_SEARCH_FILTERS < 1)
# error "CONFIG_MAX_SEARCH_FILTERS >= 1 is required"
#endif

static OneWireNg *ow = NULL;

//print eeprom's UID
static void printId(const OneWireNg::Id& id)
{
  //will print eeprom uid in MSB order (received from bus in LSB)
  //verified that OneWireNg::Id stores bytes in correct format (ex: 0x5C -> 5 is MSB, C is LSB)
    for (int i = sizeof(OneWireNg::Id) - 1; i >= 0; i--)
    {
        if (i < (int)(sizeof(OneWireNg::Id) - 1)) Serial.print(':');
        if (id[i] < 0x10) Serial.print('0');
        Serial.print(id[i], HEX);
    }
    Serial.println();
    Serial.println();
}

//memAddr -> which memory location to read from
//
//pages   -> # of pages to read after memAddr
void memRead(uint8_t memAddr, int pages) {
    int size = (pages*32) + 4; //size of buffer must at least be 2 + 32 + 2 bytes (36 total)

    if(pages > 1)
    {
      //add two extra bytes to buffer size per extra page of memory to be read (2 + 32 + 2 + ADDITIONAL 32 + THIS 2 EXTRA BYTES)
      for(int i = 1; i < pages; i++)
      {
        size += 2;
      }
    }

    uint8_t data[size];
    memset(data, 0, sizeof(data));  

    if (ow->reset() == OneWireNg::EC_SUCCESS)
    {
      ow->writeByte(SKIP_ROM); //skip ROM
      ow->writeByte(READ_MEMORY); //Read Memory
      ow->writeByte(memAddr); //Read from user specified memory page/offset

      // Read each byte at memory address
      for(int i = 0; i < sizeof(data); i++)
      {
        data[i] = ow->readByte();
      }

      Serial.println("-----------------------------------------------------------------------");
      Serial.print("Data from Read Memory starting at memory address ");
      Serial.print(memAddr, HEX);
      Serial.println(":");
      Serial.println("-----------------------------------------------------------------------");

        int index = 0;

        // Print first 2 bytes (header)
        for (int i = 0; i < 2; i++, index++)
        {
            if (data[index] < 0x10) Serial.print("0");
            Serial.print(data[index], HEX);
            Serial.print(" ");
        }

        Serial.println();
        Serial.println();

        // Print each page's 32 data bytes and 2-byte footer
        for (int page = 0; page < pages; page++)
        {
            Serial.print("Page #");
            Serial.print(page);
            Serial.println(":");

            // 32 data bytes
            for (int i = 0; i < 32; i++, index++)
            {
                if (data[index] < 0x10) Serial.print("0");
                Serial.print(data[index], HEX);
                Serial.print(" ");
                if ((i + 1) % 8 == 0) Serial.println();
            }

            Serial.println();

            // 2 footer bytes
            for (int i = 0; i < 2; i++, index++)
            {
                if (data[index] < 0x10) Serial.print("0");
                Serial.print(data[index], HEX);
                Serial.print(" ");
            }

            Serial.println();
            Serial.println();
        }
    }
}

//scrAddr -> which scratchpad location to read from
//
//len     -> length of page at location
void scratchRead(uint8_t scrAddr, int len) {
    int size = len; //user specifies size of scratchpad section

    uint8_t data[size];
    memset(data, 0, sizeof(data));  

    //if (ow->reset() == OneWireNg::EC_SUCCESS)
    //{
      //ow->writeByte(SKIP_ROM); //skip ROM
      ow->writeByte(READ_SCRATCHPAD); //Read Scratchpad
      ow->writeByte(scrAddr); //Read from user specified memory page/offset

      for(int i = 0; i < sizeof(data); i++)
      {
        data[i] = ow->readByte();
      }

      Serial.println("------------------------------------------");
      Serial.print("Data from Read Scratchpad at page ");
      Serial.print(scrAddr, HEX);
      Serial.println(":");
      Serial.println("------------------------------------------");
      Serial.print(data[0], HEX);
      for(int i = 1; i < sizeof(data); i++)
      {
        Serial.print(":");
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
      }
      Serial.println("");
      Serial.println("");
    //}
}

void setup()
{
  //turn off on-board DOTstar LED
  strip.begin();
  strip.show();

  while(!Serial){ /* spin */ } //needed to allow board to enumerate USB connection before trying to print to monitor in void setup

  Serial.begin(115200);
}

void loop()
{
  OneWireNg::Id id;             //stores serial number of one wire device
  uint8_t data[32] = {0};       //buffer that stores data recieved from 0xAA 0xE0 commands
  uint8_t scratchOffset = 0x20; //offset where to read from scratchpad
  uint8_t currPage = 0x00;      //used for reading scratchpad contents, will inc by 0x20 every loop until reaching 0xE0

  bool found = 0; //stores result of search bus function
  bool resetPresence = 0; //stores result of bus reset function

  ow = new OneWireNg_CurrentPlatform(OW_PIN, false); //declare new one wire object
  
  ow->powerBus(true);  //turn on parasite power for the bus
  delay(500);         //wait 500 ms for capacitor to charge
  ow->powerBus(false); //turn off parasite power for the bus

  if(debug)
  {
    resetPresence = ow->reset();

    if(resetPresence == OneWireNg::EC_SUCCESS)
    {
      Serial.println("Device detected on the bus!");
    }
    else
    {
      Serial.println("Reset pulse did not detect device on the bus.");
    }
  }
  
  //search for devices connected to the bus
  ow->searchReset();
  found = ow->search(id);

  if(debug)
  {
    if(found == OneWireNg::EC_SUCCESS)
    {
      Serial.println("A device was found with the search function!");
    }
    else
    {
      Serial.println("A device was not found with the search function.");
    }

    //print the ID of the eeprom returned from the search function
    printId(id); 
  }

  if(!print)
  {
    Serial.println("-------------");
    Serial.println("EEPROM UID");
    Serial.println("-------------");
    printId(id); //print id of eeprom
    ow->reset();

    ow->addressSingle(id);
    delayMicroseconds(1500); //wait in uS
    memRead(0x00, 4); //read memory starting at location 0x00, read 4 pages of memory

    //read scratchpad contents to serial monitor, starting at 0x00, then incrementing up to 0x20, all the way to 0xE0, 0xE0 is only 8 bytes long
    //scratchpad sections probably used as follows
    //
    //0x00:
    //0x20:
    //0x40:
    //0x60:
    //0x80: temp section of scratchpad for data you want to write to memory
    //0xA0:
    //0xE0: probably stores material type on the spool
    for(int i = 0; i < 7; i++)
    {
      ow->reset();
      ow->addressSingle(id);
      delayMicroseconds(1500); //wait in uS
      if((currPage == 0xE0) || (currPage == 0x00) || (currPage == 0xA0))
      {
        scratchRead(currPage, 8); //read scratchpad section, 8 bytes long 
      }
      else
      {
        scratchRead(currPage, 32); //read scratchpad section, 32 bytes long
      }

      currPage += 0x20; //add 0x20 to current page number
      if(currPage == 0xC0)
      {
        currPage += 0x20; //if current page is 0xC0, skip it and increment to 0xE0, 0xC0 is all FF's
      }
    }

    currPage = 0x00; //reset current scratchpad page int

    /*
    //want to overwrite data in memory page 0x02 as all 00's
    //maybe this will reset the counter for amount of filament that has been printed from the spool
    //first have to set temp section (0x80) of scratchpad to all 00's
    ow->reset(); //reset bus
    ow->addressSingle(id); //address the eeprom
    delayMicroseconds(1500); //wait in uS

    //write data to temp area of scratchpad first (0x80)
    Serial.println("Trying to write data to scratchpad 0x80 now");
    ow->writeByte(0x0F); //write scratchpad command
    ow->writeByte(0x80); //offset in scratchpad where data is to be written

    //do I need to calc and write crc-16/inverted crc-16/crc-8 to bus before writing to scratchpad???
    //based on wavefrom captured from printer, there is no crc sent by the master before the master writes data to the scratchpad location 0x80
    //there are 2 bytes sent by the slave before and after the master's data is sent though

    //give slave time to write 2 bytes to bus
    //printer waveform shows around 8200 uS between the above commands being sent and the master starting data transmission
    //the slave sends 2 bytes in that timeframe too
    //round up to 8500 uS for my program
    delayMicroseconds(8500);
    for(int i = 0; i < 32; i++)
    {
      ow->writeByte(0x00); //write 00 to the scratchpad temp section as the data up to 32 bytes long
    }
    //only around a 900 uS delay before the slave sends another 2 bytes after master transmits data
    //round up to 1000 uS for my program
    delayMicroseconds(1000); //give slave time to write 2 more bytes to bus
    Serial.println("Data should be written into scratchpad offset 0x80 now");
    ow->reset(); //reset the bus before checking scratchpad temp area

    //verify that scratchpad area 0x80 has all 00's
    ow->addressSingle(id);
    delayMicroseconds(1500); //wait in uS

    //read scratchpad command followed by offset
    scratchRead(0x80, 32); //32 bytes long
    */
    ow->reset(); //reset the bus before moving on
    
    print = 1;
  }

  delay(5000); //20 sec delay to give time for serial monitor reading
}