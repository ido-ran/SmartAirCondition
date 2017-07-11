/*
 * Smart Air Condition Control.
 * 
 * Hardware Wiring:
 * IR Transmiter - Data to D3
 * 
 * Using IRmote library from https://github.com/markszabo/IRremoteESP8266/
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>

#include <EEPROM.h>
PROGMEM const int LAST_ETAG_ADDRESS = 0;

#define USE_SERIAL Serial

const char server[] = "ran-smart-home.appspot.com"; // IP Adres (or name) of server to dump data to
unsigned long last_etag = 0;
unsigned int fail_count = 0;
IRsend irsend(4);
uint16_t uSendBuff[131];

// Raw IR buffers pre-configured with A/C commands (see ac_ir_analysis.py Python script)
// stored in Flash! (it's too much for to 2KB of SRAM we have..)
PROGMEM const uint16_t  COOL_23[132] = {131, 7900,4200, 1700,650, 600,1750, 650,1700, 550,1800, 550,1750, 600,1750, 650,1750, 600,1700, 1700,650, 550,1800, 650,1700, 550,1750, 1750,600, 1750,600, 600,1750, 600,1750, 550,1800, 550,1800, 550,1750, 600,1750, 1750,650, 1650,650, 600,1750, 650,1700, 550,1800, 550,1750, 600,1750, 600,1800, 550,1750, 600,1750, 550,1800, 550,1800, 550,1800, 550,1750, 600,1800, 550,1750, 600,1750, 550,1800, 550,1800, 550,1800, 600,1700, 600,1800, 550,1750, 600,1750, 1700,650, 1700,650, 550,1800, 550,1800, 550,1750, 600,1750, 600,1750, 550,1800, 550,1800, 550,1800, 550,1750, 600,1800, 1750,550, 1700,650, 600,1750, 1700,650, 550,1800, 600,1700, 600,1800, 550,1750, 1600};  // UNKNOWN D6A905FB
PROGMEM const uint16_t  COOL_25[132] = {131, 7850,4250, 1750,650, 550,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1700, 650,1750, 1700,650, 600,1700, 600,1750, 600,1750, 1750,650, 550,1750, 600,1750, 600,1750, 600,1750, 1700,650, 600,1700, 600,1750, 1750,650, 1700,650, 550,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1700, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1700, 600,1800, 600,1750, 600,1700, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 1750,650, 1700,600, 600,1700, 650,1700, 600,1750, 600,1750, 600,1700, 650,1750, 600,1750, 600,1750, 600,1750, 600,1750, 1750,600, 1700,650, 600,1700, 1750,650, 550,1750, 600,1750, 600,1700, 650,1750, 1600};  // UNKNOWN 3B7F5B7B
PROGMEM const uint16_t  HEAT_30[132] = {131, 7850,4250, 1750,650, 550,1750, 600,1750, 600,1700, 650,1750, 600,1750, 600,1700, 600,1750, 600,1750, 1750,600, 600,1750, 600,1750, 600,1750, 600,1750, 1700,650, 550,1800, 550,1750, 600,1750, 1750,600, 1750,650, 1700,650, 1700,650, 550,1750, 600,1750, 550,1750, 650,1750, 600,1700, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 550,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 1750,550, 1750,600, 650,1700, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 600,1700, 650,1750, 600,1750, 550,1750, 1750,650, 550,1750, 600,1750, 1750,600, 1750,650, 550,1750, 600,1750, 600,1750, 1600};  // UNKNOWN 30BAF5AF
PROGMEM const uint16_t  OFF[132] = {131, 7850,4250, 1750,600, 600,1750, 600,1750, 600,1750, 600,1750, 600,1700, 600,1750, 600,1750, 1750,600, 600,1750, 600,1750, 600,1750, 600,1750, 600,1750, 1750,600, 550,1800, 600,1750, 1700,650, 1700,600, 1750,600, 600,1750, 1750,600, 600,1750, 600,1750, 600,1750, 600,1700, 600,1750, 600,1750, 600,1750, 600,1750, 600,1700, 650,1750, 600,1750, 600,1700, 600,1700, 650,1700, 650,1750, 600,1750, 600,1700, 650,1750, 600,1750, 600,1750, 600,1750, 550,1750, 650,1750, 600,1650, 1800,650, 1700,600, 600,1750, 600,1750, 600,1700, 650,1750, 600,1750, 600,1750, 600,1700, 600,1800, 550,1750, 1750,600, 600,1750, 600,1750, 600,1750, 1700,650, 600,1750, 550,1700, 1700};  // UNKNOWN 821ABB3
PROGMEM const uint16_t  HEAT_26[132] = {131, 7850, 4250, 1700, 650, 600, 1700, 600, 1750, 600, 1750, 650, 1700, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 1750, 600, 550, 1750, 650, 1750, 1700, 650, 550, 1750, 600, 1750, 600, 1750, 600, 1700, 600, 1750, 1750, 650, 600, 1700, 1750, 650, 1700, 650, 550, 1800, 550, 1750, 600, 1700, 650, 1750, 600, 1750, 600, 1700, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1700, 650, 1750, 550, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1700, 600, 1750, 1750, 650, 1700, 650, 550, 1750, 600, 1750, 600, 1750, 600, 1700, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 1750, 600, 1700, 650, 1700, 650, 550, 1750, 600, 1750, 600, 1750, 600, 1750, 1600};

const uint16_t * getAcSendBuff(unsigned int index) {
  if ( index == 1 ) { return COOL_23; /* D6A905FB */ }
  if ( index == 2 ) { return COOL_25; /* 3B7F5B7B */ }
  if ( index == 3 ) { return HEAT_30; }
  if ( index == 4 ) { return HEAT_26; }
  if ( index == 0 ) { return OFF; }
  return 0;
}

void sendIRCommand(unsigned int command_index)
{
  const uint16_t *pfSendBuff = getAcSendBuff(command_index);
  if (pfSendBuff == 0)
  {
    Serial.print( F("unsupported IR command - ") );
    Serial.println(command_index);
  }
  else
  {
    Serial.print( F("Sending IR command - ") );
    Serial.println(command_index);
    
    uint16_t rawlen = 0;
    rawlen = pgm_read_word_near(pfSendBuff);
    memcpy_P(uSendBuff, pfSendBuff+1, rawlen * sizeof(uint16_t));
    irsend.sendRaw(uSendBuff, rawlen, 38);
    //irsend.sendRaw((uint16_t*) pfSendBuff, 131, 38);
  }
}

//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void EEPROMWritelong(int address, long value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  
  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

//This function will return a 4 byte (32bit) long from the eeprom
//at the specified address to address + 3.
long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  
  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}


ESP8266WiFiMulti WiFiMulti;

void setup() {

  irsend.begin();
  
  USE_SERIAL.begin(115200);
 // USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for(uint8_t t = 4; t > 0; t--) {
      USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
      USE_SERIAL.flush();
      delay(1000);
  }

  WiFiMulti.addAP("Avital-Mamad", "avitallavie");

}

void loop() {

  // wait for WiFi connection
  if((WiFiMulti.run() == WL_CONNECTED)) {

      HTTPClient http;

      USE_SERIAL.print("[HTTP] begin...\n");
      // configure traged server and url
      //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
      http.begin("http://ran-smart-home.appspot.com/api"); //HTTP

      USE_SERIAL.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if(httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

          // file found at server
          if(httpCode == HTTP_CODE_OK) {
              String payload = http.getString();
              USE_SERIAL.println(payload);

              char buffer[10];
              payload.toCharArray(buffer, sizeof(buffer));
              unsigned long etag = strtoul(buffer + 2, NULL, 10);
              Serial.print( F("curr etag ") );
              Serial.print(etag);
              Serial.print( F("  last etag ") );
              Serial.print(last_etag);
              Serial.println();
              if (etag == last_etag)
              {
                Serial.println( F("same etag, ignoring...") );
              }
              else
              {
                last_etag = etag;
                EEPROMWritelong(LAST_ETAG_ADDRESS, last_etag);
                unsigned int command_index = buffer[0] - '0';
                sendIRCommand(command_index);          
              }
          }
      } else {
          USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
  }

  delay(10000);
}

