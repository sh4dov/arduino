#include <EEPROM.h>

#define TIME_RESOLUTION 100
#define MAX_VAL 255

int leds[] = {10};
byte ledsCount = 1;
bool ledsOn = true;
byte currentLedVal = 0;
byte expectedLedVal = 100;

unsigned long nextRead = 0;

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
  }
  else
  {
    expectedLedVal = EEPROM.read(1);
  }

  pinMode(signalPin, OUTPUT);
  pinMode(incPin, INPUT);
  pinMode(decPin, INPUT);

  for (byte i = 0; i < ledsCount; i++)
  {
    pinMode(leds[i], OUTPUT);
  }
}

void loop()
{
  handleTimer();
}

void handleTimer()
{
  if (nextRead < millis())
  {
    nextRead = millis() + TIME_RESOLUTION;
    handleEvents();
  }
}

void handleEvents()
{
  handleIncDec();
  handleLeds();
  handleSignal();
}

void handleSignal()
{
  if (signalState && signalVal < 5) // 500ms
  {
    signalVal++;
  }
  else if (signalState && signalVal >= 5)
  {
    signalState = false;
    digitalWrite(signalPin, LOW);
    signalVal = 0;
  }
  else if (!signalState && signalVal < 20) // 2s
  {
    signalVal++;
  }
  else if (!signalState && signalVal >= 20)
  {
    signalState = true;
    digitalWrite(signalPin, HIGH);
    signalVal = 0;
  }
}

void handleIncDec()
{
  bool wasChanged = false;

  if (digitalRead(incPin) == HIGH && digitalRead(decPin) == HIGH)
  {
    ledsOn = true;
    currentLedVal = 0;
  }
  else if (expectedLedVal < MAX_VAL && digitalRead(incPin) == HIGH)
  {
    wasChanged = true;
    ledsOn = true;
    expectedLedVal += 5;
  }
  else if (expectedLedVal > 0 && digitalRead(decPin) == HIGH)
  {
    wasChanged = true;
    ledsOn = true;
    expectedLedVal -= 5;
    currentLedVal = expectedLedVal > 0 ? expectedLedVal - 5 : 0;
  }

  if (wasChanged)
  {
    EEPROM.write(1, expectedLedVal);
  }
}

void handleLeds()
{
  bool wasChanged = false;

  if (ledsOn && currentLedVal < expectedLedVal)
  {
    currentLedVal += 1;
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
