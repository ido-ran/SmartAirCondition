/*
 * IR AC Sender - send IR signals to A/C unit
 * An IR LED must be connected to Arduino PWM pin 3.
 * Version 1.0 September 2013
 * Copyright 2013 Itamar Ostricher
 * http://itamaro.com
 */

#include <avr/pgmspace.h>
#include <IRremote.h>

IRsend irsend;

enum States{
  Ready,
  ReadPower,
  ReadMode,
  ReadFan,
  ReadTempFirstDigit,
  ReadTempSecondDigit,
  Send,
  Error,
} state;
enum PowerCommand{
  Toggle,
  Leave,
  NoPower
} pwr;
enum ModeCommand{
  Cool,
  Heat,
  Fan,
  Dry,
  NoMode
} mode;
enum FanCommand{
  Auto,
  Low,
  Medium,
  High,
  NoFan
} fan;
int temp;

void resetState() {
  pwr=NoPower;
  mode=NoMode;
  fan=NoFan;
  temp = -1;
  state = Ready;
}

unsigned int uSendBuff[RAWBUF];

PROGMEM const uint16_t uSendBuff_Leave_Cool_Auto_23[] = {100, 7900, 4250, 1750, 600, 600, 1750, 600, 1750, 600, 1750, 550, 1750, 600, 1750, 600, 1750, 600, 1750, 1750, 650, 550, 1750, 600, 1750, 600, 1750, 600, 1700, 600, 1750, 1750, 650, 600, 1700, 600, 1750, 1750, 600, 1750, 650, 1700, 600, 600, 1700, 1800, 550, 650, 1750, 600, 1700, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 550, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 550, 1750, 600, 1750, 600, 1750, 600, 1750, 1750, 650, 1700, 600, 600, 1750, 600, 1750, 550, 1750};
PROGMEM const uint16_t uSendBuff_Leave_Cool_Auto_25[] = {100, 7900, 4250, 1800, 550, 650, 1650, 700, 1650, 700, 1650, 700, 1650, 650, 1700, 650, 1650, 700, 1650, 1850, 550, 650, 1650, 700, 1650, 700, 1650, 700, 1650, 650, 1700, 1800, 550, 700, 1650, 650, 1700, 1800, 550, 650, 1650, 700, 1650, 1850, 500, 1850, 500, 700, 1650, 700, 1650, 650, 1700, 650, 1700, 650, 1650, 700, 1650, 700, 1700, 650, 1650, 700, 1650, 650, 1700, 650, 1700, 650, 1700, 650, 1650, 700, 1700, 650, 1650, 700, 1650, 700, 1650, 650, 1700, 600, 1750, 650, 1700, 650, 1700, 650, 1700, 1800, 550, 1800, 500, 700, 1650, 700, 1700, 600, 1650};
PROGMEM const uint16_t uSendBuff_Leave_Heat_Auto_30[] = {100, 7850, 4350, 1650, 700, 500, 1800, 550, 1800, 550, 1800, 500, 1800, 550, 1850, 550, 1800, 500, 1850, 500, 1850, 1650, 700, 500, 1800, 550, 1800, 500, 1850, 550, 1800, 1650, 700, 500, 1850, 500, 1850, 500, 1800, 1700, 700, 1650, 650, 1700, 700, 1600, 700, 550, 1800, 500, 1850, 500, 1850, 500, 1850, 500, 1800, 550, 1850, 500, 1800, 550, 1800, 550, 1800, 500, 1800, 550, 1850, 500, 1850, 500, 1850, 500, 1850, 500, 1800, 550, 1800, 550, 1800, 550, 1800, 500, 1850, 550, 1800, 550, 1800, 500, 1850, 1650, 700, 1650, 700, 500, 1800, 550, 1800, 550, 1800};
PROGMEM const uint16_t uSendBuff_Toggle_Cool_Auto_0[] = {100, 7900, 4250, 1750, 650, 550, 1750, 600, 1700, 650, 1700, 650, 1700, 600, 1750, 600, 1750, 600, 1750, 1750, 650, 600, 1700, 600, 1750, 550, 1750, 600, 1800, 550, 1800, 1750, 600, 600, 1700, 600, 1750, 1750, 650, 550, 1750, 650, 1650, 1800, 650, 1650, 650, 600, 1700, 600, 1750, 650, 1700, 650, 1700, 650, 1700, 600, 1750, 650, 1700, 600, 1750, 600, 1700, 650, 1750, 550, 1750, 600, 1750, 600, 1750, 600, 1750, 600, 1750, 550, 1800, 550, 1750, 600, 1800, 550, 1800, 550, 1750, 600, 1750, 600, 1750, 600, 1750, 550, 1800, 1750, 600, 1750, 650, 550, 1750};

const uint16_t * getAcSendBuff() {
  if ( (Leave == pwr) && (Cool == mode) &&(Auto == fan) && (23 == temp) ) { return uSendBuff_Leave_Cool_Auto_23; }
  if ( (Leave == pwr) && (Cool == mode) &&(Auto == fan) && (25 == temp) ) { return uSendBuff_Leave_Cool_Auto_25; }
  if ( (Leave == pwr) && (Heat == mode) &&(Auto == fan) && (30 == temp) ) { return uSendBuff_Leave_Heat_Auto_30; }
  if ( (Toggle == pwr) && (Cool == mode) &&(Auto == fan) && (0 == temp) ) { return uSendBuff_Toggle_Cool_Auto_0; }
  return 0;
}

int sendAcCommand() {
  uint16_t rawlen;
  if ( (NoPower == pwr) || (NoMode == mode) || (NoFan == fan) || (-1 == temp) ) {
    Serial.println(F("Error in send A/C command - not all parameters set"));
    return 0;
  }
  // valid state - send it
  Serial.print(F("Sending A/C command with following parameters: Power="));
  Serial.print(pwr, DEC);
  Serial.print(F(", Mode="));
  Serial.print(mode, DEC);
  Serial.print(F(", Fan="));
  Serial.print(fan, DEC);
  Serial.print(F(", Temperature="));
  Serial.println(temp, DEC);
  const uint16_t * pfSendBuff = getAcSendBuff();
  if (0 != pfSendBuff) {
    rawlen = pgm_read_word_near(pfSendBuff);
    memcpy_P(uSendBuff, pfSendBuff+1, rawlen * sizeof(uint16_t));
    //irsend.sendRaw(uSendBuff, rawlen, 38);
    Serial.println("Success");
  } else {
    Serial.println("Unsupported Command");
  }
  return 1;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Arduino A/C-Control v1.0 - Ready");
  resetState();
}

void loop() {
  char incoming;
  int val;
  // Valid A/C commands:
  // - P#  - set power toggle value
  // - M#  - set mode value
  // - F#  - set fan value
  // - T##  - set temperature value
  // - S    - send A/C command
  // where "#" is a ASCII digit (according to enum, or specifying temperature as 2-digit number)
  while (Serial.available()) {
    incoming = Serial.read();
    switch (state) {
      
      case Ready:
        switch (incoming) {
          case 'P':
          state = ReadPower;
          break;
          case 'M':
          state = ReadMode;
          break;
          case 'F':
          state = ReadFan;
          break;
          case 'T':
          state = ReadTempFirstDigit;
          break;
          case 'S':
          if (0 != sendAcCommand()) {
            state = Ready;
          } else {
            state = Error;
          }
          break;
        }
        break;
      
      case ReadPower:
        val = (int)incoming - (int)'0';
        if ( (val >= 0) && (val < (int)NoPower)  ) {
          pwr = (PowerCommand)val;
          Serial.print(F("Power parameter set to "));
            Serial.println(pwr, DEC);
          state = Ready;
        } else {
          Serial.println(F("Invalid power value to"));
          state = Error;
        }
        break;
      
      case ReadMode:
        val = (int)incoming - (int)'0';
        if ( (val >= 0) && (val < (int)NoMode)  ) {
          mode = (ModeCommand)val;
          Serial.print(F("Mode parameter set to "));
            Serial.println(mode, DEC);
          state = Ready;
        } else {
          Serial.println(F("Invalid mode value"));
          state = Error;
        }
        break;
      
      case ReadFan:
        val = (int)incoming - (int)'0';
        if ( (val >= 0) && (val < (int)NoFan)  ) {
          fan = (FanCommand)val;
          Serial.print(F("Fan parameter set to "));
            Serial.println(fan, DEC);
          state = Ready;
        } else {
          Serial.println(F("Invalid fan value"));
          state = Error;
        }
        break;
      
      case ReadTempFirstDigit:
        val = (int)incoming - (int)'0';
        if ( (val >= 0) && (val <= 9)  ) {
          temp = 10 * val;
          state = ReadTempSecondDigit;
        } else {
          Serial.println(F("Invalid temperature digit"));
          state = Error;
        }
        break;
      
      case ReadTempSecondDigit:
        val = (int)incoming - (int)'0';
        if ( (val >= 0) && (val <= 9)  ) {
          temp += val;
          Serial.print(F("Temperature parameter set to "));
            Serial.println(temp, DEC);
          state = Ready;
        } else {
          Serial.println(F("Invalid temperature digit"));
          state = Error;
        }
        break;
    }
  }
  
  if (Error == state) {
    // clear all pending serial inputs and return to Ready state
    // (keep parameters that were set before)
    while (Serial.available()) {
      Serial.read();
    }
    state = Ready;
  }
}
