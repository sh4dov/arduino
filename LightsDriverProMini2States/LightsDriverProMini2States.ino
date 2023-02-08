#include <Boards.h>
#include <Firmata.h>
#include <FirmataConstants.h>
#include <FirmataDefines.h>
#include <FirmataMarshaller.h>
#include <FirmataParser.h>

#include <EEPROM.h>

#define TIME_RESOLUTION 10
#define AUTO_OFF_MAX_VAL 1 * 60 * (1000 / TIME_RESOLUTION) // 1min
#define INITIAL_AUTO_OFF_VAL 1 * 59 * (1000 / TIME_RESOLUTION) // 59s
#define MAX_VAL 255

int leds[] = {10};
byte ledsCount = 1;
bool ledsOn = true;
byte currentLedVal = 0;
byte expectedLedVal = 100;
byte expectedDimLedValue = 20;

int detectors[] = {A0};
byte detectorsCount = 1;
bool isDetected = false;
bool wasDetected = true;
bool isMasterOn = false;

unsigned int autoOffCounter = INITIAL_AUTO_OFF_VAL;

unsigned long timeNow = 0;

int masterPin = 6;
int incPin = 7;
int decPin = 8;

int signalPin = 13;
int signalVal = 0;
bool signalState = false;

void setup()
{
  if (EEPROM.read(0) != 1)
  {
    EEPROM.write(0, 1);
    EEPROM.write(1, expectedLedVal);
    EEPROM.write(2, expectedDimLedValue);
  }
  else
  {
    expectedLedVal = EEPROM.read(1);
    expectedDimLedValue = EEPROM.read(2);
  }

  pinMode(signalPin, OUTPUT);
  pinMode(incPin, INPUT);
  pinMode(decPin, INPUT);
  pinMode(masterPin, INPUT);

  for (byte i = 0; i < ledsCount; i++)
  {
    pinMode(leds[i], OUTPUT);
  }

  for (byte i = 0; i < detectorsCount; i++)
  {
    pinMode(detectors[i], INPUT);
  }
}

void loop()
{
  handleTimer();
}

void handleTimer()
{
  if (millis() - timeNow > TIME_RESOLUTION)
  {
    timeNow = millis();
    handleEvents();
  }
}

void handleEvents()
{
  handleIncDec();
  handleAutoOnOff();
  handleLeds();
  //handleSignal();
}

void handleSignal()
{
  if (signalState && signalVal < 50) // 500ms
  {
    signalVal++;
  }
  else if (signalState && signalVal >= 50)
  {
    signalState = false;
    digitalWrite(signalPin, LOW);
    signalVal = 0;
  }
  else if (!signalState && signalVal < 200) // 2s
  {
    signalVal++;
  }
  else if (!signalState && signalVal >= 200)
  {
    signalState = true;
    digitalWrite(signalPin, HIGH);
    signalVal = 0;
  }
}

void handleIncDec()
{
  isMasterOn = digitalRead(masterPin) == HIGH;
  bool wasChanged = false;
  byte *ledVal = isMasterOn ? &expectedLedVal : &expectedDimLedValue;

  if (digitalRead(incPin) == HIGH && digitalRead(decPin) == HIGH)
  {
    ledsOn = true;
    currentLedVal = 0;
  }
  else if (*ledVal < MAX_VAL && digitalRead(incPin) == HIGH)
  {
    wasChanged = true;
    ledsOn = true;
    *ledVal += 5;
  }
  else if (*ledVal > 5 && digitalRead(decPin) == HIGH)
  {
    wasChanged = true;
    ledsOn = true;
    *ledVal -= 5;
    currentLedVal = *ledVal > 0 ? *ledVal - 5 : 0;
  }

  if (wasChanged)
  {
    autoOffCounter = INITIAL_AUTO_OFF_VAL;
    if(!isMasterOn)
    {
      wasDetected = true;
    }
    EEPROM.write(1, expectedLedVal);
    EEPROM.write(2, expectedDimLedValue);
    delay(100);
  }
}

void handleAutoOnOff()
{  
  handleDetection();
  if (ledsOn && !isDetected && !isMasterOn)
  {
    autoOffCounter++;
  }

  if (ledsOn && isDetected)
  {
    autoOffCounter = 0;
  }

  if (!ledsOn && (isDetected || isMasterOn))
  {
    ledsOn = true;
  }

  if (autoOffCounter >= AUTO_OFF_MAX_VAL || !wasDetected)
  {
    if(!isMasterOn)
    {
      ledsOn = false;      
    }
    autoOffCounter = 0;
    wasDetected = false;
    digitalWrite(signalPin, LOW);
  }
}

void handleDetection()
{
  for (byte i = 0; i < detectorsCount; i++)
  {
    if (digitalRead(detectors[i]) == HIGH)
    {
      digitalWrite(signalPin, HIGH);
      isDetected = true;
      wasDetected = true;
      return;
    }
  }
  
  isDetected = false;
}

void handleLeds()
{
  bool wasChanged = false;

  if (ledsOn && isMasterOn && currentLedVal < expectedLedVal)
  {
    currentLedVal += 1;
    wasChanged = true;
  }

  if(ledsOn && !isMasterOn && currentLedVal < expectedDimLedValue)
  {
    currentLedVal += 1;
    wasChanged = true;
  }

  if(ledsOn && !isMasterOn && currentLedVal > expectedDimLedValue)
  {
    currentLedVal -= 1;
    wasChanged = true;
  }

  if (!ledsOn && currentLedVal > 0)
  {
    currentLedVal -= 1;
    wasChanged = true;
  }

  if (wasChanged)
  {
    for (byte i = 0; i < ledsCount; i++)
    {
      analogWrite(leds[i], currentLedVal);
    }
  }
}
