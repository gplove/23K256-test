/*----------------------------------------------------------------*
 | 23K256 external memory IC using SPI library                    |
 | Use logic level shifting for I/O pins of IC - NOT 5V TOLERANT! |
 | Vcc is 3.3V, can be taken from Arduino                         |
 | Use 0.1uF capacitor between Vcc and GND close to IC            |
 | Use 10k pullup resistors to Vcc on IC pins 1 and 7             |
 | Arduino SPI pins - CS=10, MOSI=11, MISO=12, SCK=13             |
 *----------------------------------------------------------------*/
#include <SPI.h>
const int CS = 10; // must be specified for Nano Every - SS appears not to work as for Nano

/* Instruction register bit settings */
const byte RAM_RDSR = 0x05; // Read the STATUS register
const byte RAM_WRSR = 0x01; // Write to the STATUS register
const byte RAM_READ = 0x03; // Read data from memory
const byte RAM_RITE = 0x02; // Write data to memory

/* Status register bit settings */
const byte MODE_BYTE = 0x00; // Byte mode (default) - handles just one byte
const byte MODE_SEQL = 0x40; // Sequential mode - 32K bytes accessible
const byte MODE_PAGE = 0x80; // Page mode - 1024 pages of 32 bytes
const byte MODE_HOLD = 0x01; // Disables hold mode if set (AND with bits 7&6)
const byte MODE_RESD = 0xC0; // Reserved (not used)

/* Global Variables */
byte send_seql[] = "This example treats the whole chip as a single accessible array of 32768 bytes.";     // data to write in sequential mode
byte send_page[] = "Graham is a genius! 32 bytes....";                                                    // data to write in page mode (max. 32 bytes per page)
byte send_user[] = "This is a test of writing more than 32 bytes to a user defined page size";            // data to write in user defined page test
byte read_seql[sizeof(send_seql)], read_page[sizeof(send_page)], read_byte, read_user[sizeof(send_user)]; // to hold data returned from functions
byte dummy = 0x00;                                            // dummy value sent to initiate clock, ignored by chip unless sent as valid data
byte status_read;                                             // status register
int i,j;                                                      // counters
int pagelength=512;                                           // user defined page length, change as required (64,128,256,512,1024,2048,4096,8192,16384)
int numberofpages=32768/pagelength;                           // calculated number of user defined pages

/* Set up variables, chip select pin and start the serial monitor and SPI */
void setup() {
  short addr;                             // variable to hold the address
  byte dataR;                             // variable to hold the data value read
  byte dataS;                             // variable to hold the data value sent
  digitalWrite(CS, HIGH);                 // I think this is the right way to ensure CS isn't accidentally enabled
  pinMode(CS, OUTPUT);                    // set up chip select pin
  randomSeed(analogRead(0));              // use unconnected A0 to generate random seed
  Serial.begin(57600);                    // set communication speed for the serial monitor
  SPI.begin();                            // start SPI protocol - default values match SRAM: MSBFIRST and SPI_MODE0

  /* Clear entire memory */
  Serial.print("'Blanking' entire memory....");
  WriteStatus(MODE_BYTE);                 // set to send/receive single byte of data
  ReadStatus();                           // call function to confirm mode
  for (addr = 0; addr < 32767; addr++){   // loop through entire chip - max for 'short' variable
    RAM_WRITEByte(addr, dummy);}          // write 'blanking' data = dummy variable
  RAM_WRITEByte(32767, dummy);            // blank last address
  Serial.print("...done");

  /* Read sample bytes from memory */
  Serial.print("\nReading five random addresses:\n");
  WriteStatus(MODE_BYTE);                 // set to send/receive single byte of data
  for (i = 0; i < 5; i++){                // loop through 5 addresses
    int randaddr = random(4096, 32767);   // select random addresses
    dataR = RAM_READByte(randaddr);       // Read a byte of data at the memory location
    Serial.print("\tAddress ");           // lengthy print option needed as sprintf does not work with SPI for Nano Every
    Serial.print(randaddr, HEX);
    Serial.print(" contains: ");
    Serial.println(dataR);}

  /* Write a single byte to memory */
  Serial.print("\nWriting in BYTE mode....");
  WriteStatus(MODE_BYTE);                 // set to send/receive single byte of data
  ReadStatus();                           // call function to confirm mode
  dataS = 2;                              // initialize the data, eg start with 2
  for (addr = 0; addr < 5; addr++){       // Write 5 individual bytes to memory eg 0 to 4
    RAM_WRITEByte(addr, dataS);           // write a byte of data to that address
    dataS *= 2;}                          // increment the data eg double each time

  /* Read a single byte from memory */
  Serial.print("\nReading in BYTE mode:\t");
  WriteStatus(MODE_BYTE);                 // set to send/receive single byte of data
  for (addr = 0; addr < 5; addr++){       // loop through 5 addresses, matching above!
    dataR = RAM_READByte(addr);           // Read a byte of data from that address
    Serial.print(dataR);                  // print the data values
    if (addr < 4) Serial.print(", ");}    // separate with a comma

  /* Write pages of data */
  Serial.print("\n\nWriting in PAGE mode....");
  WriteStatus(MODE_PAGE);                 // set to send/receive page data
  ReadStatus();                           // call function to confirm mode
  int startpage=random(0,1017);           // set start page
  for (j=0; j<5; j++){                    // loop through 5 page addresses
    addr=(startpage+j)*32;                // each page MUST be 32 bytes after previous page
    RAM_WRITEPage(addr, send_page);}      // write page array elements

  /* Read pages of data */
  Serial.println("\nReading in PAGE mode:");
  WriteStatus(MODE_PAGE);                 // set to send/receive page data
  for (j=0; j<6; j++){                    // loop through 6 pages to check subsequent pages are not overwritten
    addr=(startpage+j)*32;                // each page MUST be 32 bytes after previous page
    RAM_READPage(addr);                   // read page array elements
    Serial.print("\tPage# ");             // lengthy print option needed as sprintf does not work with SPI for Nano Every
    Serial.print(startpage+j);
    Serial.print(" contains: ");
    for(i=0;i<32;i++){
      Serial.print((char)read_page[i]);}
  Serial.print("\n");}
  
  /* Write a sequence of bytes */
  Serial.print("\nWriting in SEQUENTIAL mode....");
  WriteStatus(MODE_SEQL);                           // set to send/receive multiple bytes of data
  ReadStatus();                                     // call function to confirm mode
  addr = 0xA0;                                      // set a start address eg 0xA0
  RAM_WRITESeq(addr, send_seql, sizeof(send_seql)); // Write to memory starting at address set using the array of characters defined above

  /* Read a sequence of bytes */
  Serial.print("\nReading in SEQUENTIAL mode: \n");
  WriteStatus(MODE_SEQL);                          // set to send/receive multiple bytes of data
  RAM_READSeq(addr, sizeof(read_seql));            // Read from memory into array starting at addr set above
  Serial.print("\tAddress ");                      // lengthy print option needed as sprintf does not work with SPI for Nano Every
  Serial.print(addr, HEX);
  Serial.print(" contains: ");
  for(i=0;i<sizeof(read_seql);i++){
    Serial.print((char)read_seql[i]);}
  Serial.println();

  /* Write data to user defined pages */
  Serial.print("\nWriting to user defined pages... ");
  Serial.print(numberofpages);
  Serial.print(" pages each of ");
  Serial.print(pagelength);
  Serial.print(" bytes...");
  WriteStatus(MODE_SEQL);                             // set to send/receive multiple bytes of data
  ReadStatus();                                       // call function to confirm mode
  startpage = random(0,numberofpages-7);              // set a start page (0 to numberofpages-1)
  for (j=0; j<5; j++){                                // loop through 5 user defined page addresses
    addr=(startpage+j)*pagelength;                    // each page address MUST be 'pagelength' bytes after previous address
  RAM_WRITEUser(addr, send_user, sizeof(send_user));} // Write to memory starting at address set using the array of characters defined above

  /* Read data from user defined pages */
  Serial.print("\nReading from user defined pages: \n");
  WriteStatus(MODE_SEQL);                             // set to send/receive multiple bytes of data
  for (j=0; j<6; j++){                                // loop through 6 pages to check no overwriting
    addr=(startpage+j)*pagelength;                    // each page address MUST be 'pagelength' bytes after previous address
  RAM_READUser(addr, sizeof(read_user));              // Read from memory into array starting at addr set above
  Serial.print("\tPage# ");                           // lengthy print option needed as sprintf does not work with SPI for Nano Every
  Serial.print(startpage+j);
  Serial.print(" contains: ");
  for(i=0;i<sizeof(read_user);i++){
    Serial.print((char)read_user[i]);}
  Serial.println();}
  
  /* End of setup */
}   // This is the end of setup - do not delete/hide

/* Status register functions */
void WriteStatus(byte mode){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_WRSR);                   // instruction to write to status register
  SPI.transfer(mode);                       // set mode
  digitalWrite(CS, HIGH);}                  // close comms with device

void ReadStatus(){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_RDSR);                   // instruction to read the status register
  status_read = SPI.transfer(dummy);        // set variable to current status
  digitalWrite(CS, HIGH);                   // close comms with device
  Serial.print(" Mode is ");
  Serial.print(status_read, HEX);}

/* Byte transfer functions */
void RAM_WRITEByte(short addr, byte data_byte){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_RITE);                   // send write command
  SPI.transfer16(addr);                     // use this instead of two lines below - also works with Nano Every
  //  SPI.transfer((byte)(addr >> 8));      // send high byte of address
  //  SPI.transfer((byte)addr);             // send low byte of address
  SPI.transfer(data_byte);                  // Write the data to the memory location
  digitalWrite(CS, HIGH);}                  // close comms with device

byte RAM_READByte(short addr){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_READ);                   // send read command
  SPI.transfer16(addr);                     // send address
  read_byte = SPI.transfer(dummy);          // Read the byte at that address
  digitalWrite(CS, HIGH);                   // close comms with device
  return read_byte;}                        // send data back

/* Sequential transfer functions */
void RAM_WRITESeq(short addr, byte *send_seql, int dsize){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_RITE);                   // send write command
  SPI.transfer16(addr);                     // send address
  for (i = 0; i < dsize; i++) {
    SPI.transfer(send_seql[i]);}            // transfer the array of data
  digitalWrite(CS, HIGH);}                  // close comms with device

void RAM_READSeq(short addr, int dsize){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_READ);                   // send read command
  SPI.transfer16(addr);                     // send address
  for (i = 0; i < dsize; i++){              // loop counter
    read_seql[i] = SPI.transfer(dummy);}    // Read the data byte and update array
  digitalWrite(CS, HIGH);}                  // close comms with device

/* Page transfer functions */
void RAM_WRITEPage(short addr, byte *dataS){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_RITE);                   // send write command
  SPI.transfer16(addr);                     // send address
  for (i = 0; i < 32; i++){                 // loop counter (max page size is 32 bytes)
    SPI.transfer(dataS[i]);}                // Write the array data bytes
  digitalWrite(CS, HIGH);}                  // close comms with device

void RAM_READPage(short addr){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_READ);                   // send read command
  SPI.transfer16(addr);                     // send address
  for (i = 0; i < 32; i++){                 // loop counter
    read_page[i] = SPI.transfer(dummy);}    // Read the data byte and update array
  digitalWrite(CS, HIGH);}                  // close comms with device

/* User page transfer functions */
void RAM_WRITEUser(short addr, byte *send_user, int dsize){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_RITE);                   // send write command
  SPI.transfer16(addr);                     // send address
  for (i = 0; i < dsize; i++) {             // loop counter
    SPI.transfer(send_user[i]);}            // transfer an array of data
  digitalWrite(CS, HIGH);}                  // close comms with device

void RAM_READUser(short addr, int dsize){
  digitalWrite(CS, LOW);                    // open comms with device
  SPI.transfer(RAM_READ);                   // send read command
  SPI.transfer16(addr);                     // send address
  for (i = 0; i < dsize; i++){              // loop counter
    read_user[i] = SPI.transfer(dummy);}    // Read the data byte and update array
  digitalWrite(CS, HIGH);}                  // close comms with device

/**/
void loop() {}                              // nothing to do in the loop
