/*
 * UIPEthernet TcpClient example.
 *
 * UIPEthernet is a TCP/IP stack that can be used with a enc28j60 based
 * Ethernet-shield.
 *
 * UIPEthernet uses the fine uIP stack by Adam Dunkels <adam@sics.se>
 *
 *      -----------------
 *
 * This TcpClient example gets its local ip-address via dhcp and sets
 * up a tcp socket-connection to 192.168.0.1 port 5000 every 5 Seconds.
 * After sending a message it waits for a response. After receiving the
 * response the client disconnects and tries to reconnect after 5 seconds.
 *
 * Copyright (C) 2013 by Norbert Truchsess <norbert.truchsess@t-online.de>
 */

#include <UIPEthernet.h>

#include <IRremote.h>

#include <EEPROM.h>

EthernetClient client;
signed long next;

const char server[] = "ran-smart-home.appspot.com"; // IP Adres (or name) of server to dump data to

PROGMEM const int MAX_RESPONSE_LINE_SIZE = 10;
char buffer[MAX_RESPONSE_LINE_SIZE+1];

PROGMEM const int LAST_ETAG_ADDRESS = 0;

unsigned long last_etag = 0;
unsigned int fail_count = 0;

IRsend irsend;
//unsigned int uSendBuff[RAWBUF];

// Raw IR buffers pre-configured with A/C commands (see ac_ir_analysis.py Python script)
// stored in Flash! (it's too much for to 2KB of SRAM we have..)
PROGMEM const uint16_t uSendBuff_Leave_Cool_Auto_23[] = {100, 7900, 4250, 1750, 600, 600, 1750, 650, 1700, 650, 1750, 650, 1750, 650, 1700, 650, 1750, 600, 1750, 1800, 600, 600, 1750, 600, 1750, 600, 1750, 1800, 650, 1700, 650, 600, 1750, 600, 1750, 600, 1750, 1800, 600, 1750, 600, 1750, 600, 600, 1750, 1750, 650, 600, 1750, 600, 1750, 650, 1700, 600, 1750, 650, 1750, 600, 1750, 600, 1750, 650, 1750, 600, 1750, 600, 1750, 600, 1750, 650, 1750, 600, 1750, 600, 1750, 650, 1700, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 1750, 650, 1750, 650, 600, 1750, 600, 1750, 600, 1750};
PROGMEM const uint16_t uSendBuff_Leave_Cool_Auto_25[] = {100, 7900, 4250, 1800, 600, 650, 1700, 650, 1700, 650, 1700, 650, 1700, 700, 1700, 650, 1700, 650, 1700, 1800, 600, 600, 1700, 650, 1700, 650, 1700, 1800, 600, 1750, 600, 550, 1750, 650, 1700, 650, 1750, 1800, 650, 600, 1700, 650, 1700, 1800, 600, 1800, 600, 600, 1750, 650, 1700, 650, 1700, 650, 1700, 700, 1700, 650, 1750, 650, 1750, 650, 1700, 650, 1700, 650, 1750, 600, 1700, 700, 1700, 650, 1700, 600, 1750, 700, 1700, 650, 1700, 650, 1700, 650, 1700, 650, 1700, 650, 1700, 650, 1700, 700, 1700, 1800, 600, 1750, 650, 600, 1750, 650, 1750, 600, 1750};
PROGMEM const uint16_t uSendBuff_Leave_Heat_Autp_30[] = {100, 7900, 4250, 1750, 700, 600, 1750, 600, 1700, 650, 1800, 600, 1750, 600, 1750, 600, 1750, 650, 1700, 600, 1800, 1750, 650, 600, 1750, 600, 1750, 600, 1750, 650, 1750, 1700, 700, 550, 1750, 600, 1750, 600, 1800, 1700, 700, 1750, 650, 1700, 700, 1700, 650, 550, 1800, 600, 1750, 600, 1750, 650, 1750, 550, 1800, 550, 1800, 600, 1800, 550, 1800, 550, 1800, 600, 1800, 550, 1800, 600, 1750, 600, 1750, 650, 1750, 600, 1750, 600, 1750, 600, 1800, 550, 1800, 600, 1750, 600, 1800, 600, 1750, 600, 1750, 1750, 650, 1700, 650, 600, 1750, 550, 1800, 600, 1800};
PROGMEM const uint16_t uSendBuff_Toggle_Cool_Auto_0[] = {100, 7850, 4250, 1800, 600, 600, 1700, 650, 1750, 650, 1700, 650, 1700, 650, 1700, 650, 1700, 650, 1750, 1800, 600, 650, 1700, 650, 1750, 650, 1700, 1800, 600, 1800, 600, 650, 1700, 650, 1700, 650, 1700, 1800, 650, 1750, 600, 1750, 650, 600, 1700, 1800, 550, 650, 1700, 650, 1750, 650, 1700, 650, 1750, 650, 1700, 650, 1750, 650, 1700, 650, 1700, 650, 1750, 650, 1750, 650, 1700, 650, 1700, 650, 1700, 700, 1700, 650, 1750, 650, 1750, 650, 1750, 600, 1700, 650, 1750, 600, 1750, 650, 1750, 650, 1750, 650, 1750, 650, 1700, 1800, 650, 1750, 600, 600, 1750};

const uint16_t * getAcSendBuff(unsigned int index) {
  if ( index == 1 ) { return uSendBuff_Leave_Cool_Auto_23; }
  if ( index == 2 ) { return uSendBuff_Leave_Cool_Auto_25; }
  if ( index == 1000 ) { return uSendBuff_Leave_Heat_Autp_30; }
  if ( index == 0 ) { return uSendBuff_Toggle_Cool_Auto_0; }
  return 0;
}

void sendIRCommand(unsigned int command_index)
{
  const uint16_t * pfSendBuff = getAcSendBuff(command_index);
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
    //memcpy_P(uSendBuff, pfSendBuff+1, rawlen * sizeof(uint16_t));
    //irsend.sendRaw(uSendBuff, rawlen, 38);
    irsend.sendRaw((uint16_t*) pfSendBuff, rawlen, 38);
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

void setup() {

  Serial.begin(9600);

  uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
  Ethernet.begin(mac);

  Serial.print( F("localIP: ") );
  Serial.println(Ethernet.localIP());
  Serial.print( F("subnetMask: ") );
  Serial.println(Ethernet.subnetMask());
  Serial.print( F("gatewayIP: ") );
  Serial.println(Ethernet.gatewayIP());
  Serial.print( F("dnsServerIP: ") );
  Serial.println(Ethernet.dnsServerIP());

  next = 0;

  last_etag = EEPROMReadlong(LAST_ETAG_ADDRESS);
  Serial.print( F("  last etag ") );
  Serial.print(last_etag);
  Serial.println();

}

void loop() {

  if (((signed long)(millis() - next)) > 0)
    {
      next = millis() + 5000;
      Serial.println( F("Client connect") );
      // replace hostname with name of machine running tcpserver.pl
//      if (client.connect("server.local",5000))
      if (client.connect(server,80))
        {
          Serial.println( F("Client connected") );

          // Make a HTTP request:
          client.print( F("GET /api") );
          client.println( F(" HTTP/1.1") );
          client.print( F("Host: ") );
          client.println(server);
          client.println( F("Connection: close") );
          client.println();
          client.println();

          Serial.println( F("waiting for response...") );
          
          while(client.available()==0)
          {
            signed long delta = next - millis();
            Serial.print( F("time left ") );
            Serial.println(delta);
            
            if (delta < 0) {
              Serial.print( F("timeout waiting for response!") );
              break;
            }
          }

          if (client.available())
          {
            Serial.println( F("got response...") );
          }

          fail_count = 0;

          int index = 0;
          while (client.available())
          {
            if (next - millis() < 0) break;
            
            char c = client.read();
            if (c == '\n') 
            {
              index = 0;
            }
            else
            {
              if (index < MAX_RESPONSE_LINE_SIZE)
              {
                buffer[index] = c;
                index++;
              }
            }
          }
    
          buffer[index] = 0; // zero terminated string
      
          Serial.print(F("last line: "));
          Serial.println(buffer);

          if (index > 0)
          {
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
              EEPROMWritelong(LAST_ETAG_ADDRESS, last_etag);
              last_etag = etag;
              unsigned int command_index = buffer[0] - '0';
              sendIRCommand(command_index);          
            }        
          }

          //disconnect client
          Serial.println( F("Client disconnect") );
          client.stop();
        }
      else
        Serial.println( F("Client connect failed") );
        fail_count++;
    }

    if (fail_count >= 3)
    {
      Serial.println( F("soft-reset...") );

      // Delay 2 seconds and reset the Arudino
      delay(2000);
      asm volatile ( "jmp 0");
    }

}
