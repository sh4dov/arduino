#include <EEPROM.h>

#define TIME_RESOLUTION 100
#define MAX_VAL 255

int vals[] = {0, 0, 0, 0};
unsigned long val = 0;

void setup()
{
  pinMode(A0, INPUT);
  Serial.begin(4800);
}

void loop()
{
  if(Serial.available())
  {
    String cmd = Serial.readString();

    if(cmd.startsWith("GeT"))
    {
      for(int i=0; i < 4; i++)
      {
        vals[i] = analogRead(A0);
        delay(50);
      }

      val = 0;
      for(int i=0; i< 4; i++){
        val += vals[i];
      }

      val = (int)(val / 4);

        Serial.println(val);      
    }
  }
}
