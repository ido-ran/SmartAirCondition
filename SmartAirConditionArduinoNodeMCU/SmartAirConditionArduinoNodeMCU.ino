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

/**
 * Store the etag of the last executed command.
 * This is the actual command that was sent using the IR LED.
 */
unsigned long last_executed_etag = 0;
uint16_t ir_command_length = 0;
uint16_t ir_command[200];

void parseIntFromString(String commandStr, int cmdLength, int &startIndex, int &value)
{
  value = 0;
  while (startIndex < cmdLength)
  {
    char c = commandStr.charAt(startIndex++);
    //USE_SERIAL.printf("char at %d is %c\n", startIndex, c);
    
    if (c >= '0' && c <= '9')
    {
      value *= 10;
      value += c - '0';
    } else
    {
      // reach non-digit character
      break;
    }
  }
}

void readIRCommand(String commandStr, int startIndex)
{
  // first parse the command length
  int currIndex = startIndex;
  int value = 0;
  int commandLength = commandStr.length();
  parseIntFromString(commandStr, commandLength, currIndex, value);
  ir_command_length = value;

  Serial.print( F("read command length - ") );
  Serial.println(value);

  // then parse mark and spaces
  int arrIndex = 0;
  while (currIndex < commandLength)
  {
    parseIntFromString(commandStr, commandLength, currIndex, value);
    ir_command[arrIndex++] = value;

    Serial.print(value);
    Serial.print( F(", ") );
  }
  Serial.println();
}

void sendIRCommand()
{
  USE_SERIAL.print("[IR] Sending IR command...\n");
  irsend.sendRaw(ir_command, ir_command_length, 38);
  //irsend.sendRaw((uint16_t*) pfSendBuff, 131, 38);
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

  EEPROM.commit();
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

  EEPROM.begin(512);
  
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
      String serverAddress = "http://ran-smart-home.appspot.com/api?last_ececuted_etag=";
      serverAddress += last_executed_etag;
      
      http.begin(serverAddress); //HTTP

      USE_SERIAL.print("[HTTP] GET ");
      USE_SERIAL.print(serverAddress);
      USE_SERIAL.print("...\n");
      
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

              int newLineIndex = 0;
              int etag = 0;
              parseIntFromString(payload, payload.length(), newLineIndex, etag);

              if (newLineIndex != -1) {
                Serial.print( F("curr etag ") );
                Serial.print(etag);
                Serial.print( F("  last etag ") );
                Serial.print(last_etag);
                Serial.println();

                if (last_etag == 0)
                {
                  // last_etag is zero on first read (need to check why eeprom is not storing it or reading it)
                  Serial.println( F("first time, not sending command just storing etag") );
                  last_etag = etag;
                  EEPROMWritelong(LAST_ETAG_ADDRESS, last_etag);
                }
                else if (etag == last_etag)
                {
                  Serial.println( F("same etag, ignoring...") );
                }
                else
                {
                  last_executed_etag = last_etag = etag;
                  EEPROMWritelong(LAST_ETAG_ADDRESS, last_etag);

                  readIRCommand(payload, newLineIndex + 1 /* +1 for /n */);
                  sendIRCommand();          
                }
              }
          }
      } else {
          USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
  }

  delay(10000);
}

