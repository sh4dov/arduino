#include <avr/sleep.h>
#include <avr/power.h>

static const byte vcc_read = A3;
static const byte options = 2;

static const byte m1_in = 3;
static const byte m2_in = A2;
static const byte m3_in = A1;
static const byte m4_in = A0;

static const byte s1a0 = A7;
static const byte s2a0 = A6;
static const byte s3a0 = A5;
static const byte s4a0 = A4;

static const byte led_red = 8;
static const byte led_m1 = 4;
static const byte led_m2 = 5;
static const byte led_m3 = 6;
static const byte led_m4 = 7;

static const int vcc_min = 990;

static const unsigned long checkInMinutes = 20L;
static const unsigned long checkDelay =  60L * checkInMinutes / 4L;
static const int pompDelay = 1000 * 10;

static const int sensorThresholds[] = { 160, 510, 510, 510 };

static const int sensorTries = 5;

byte pomps[] = { m1_in, m2_in, m3_in, m4_in };

byte sensors[] {s1a0, s2a0, s3a0, s4a0};

byte allLeds[] = { led_m1, led_m2, led_m3, led_m4, led_red };
byte motorLeds[] = { led_m1, led_m2, led_m3, led_m4 };

ISR(TIMER1_OVF_vect)
{
    
}

void setup() {
    Serial.begin(115200);

    pinMode(13, OUTPUT);
    /* Normal timer operation.*/
    TCCR1A = 0x00;

    /* Enable the timer overlow interrupt. */
    TIMSK1 = 0x01;

    pinMode(vcc_read, INPUT);
    pinMode(options, INPUT);

    pinMode(m1_in, OUTPUT);
    pinMode(m2_in, OUTPUT);
    pinMode(m3_in, OUTPUT);
    pinMode(m4_in, OUTPUT);

    pinMode(s1a0, INPUT);
    pinMode(s2a0, INPUT);
    pinMode(s3a0, INPUT);
    pinMode(s4a0, INPUT);

    pinMode(led_red, OUTPUT);
    pinMode(led_m1, OUTPUT);
    pinMode(led_m2, OUTPUT);
    pinMode(led_m3, OUTPUT);
    pinMode(led_m4, OUTPUT);

    analogWrite(m1_in, 0);
    analogWrite(m2_in, 0);
    analogWrite(m3_in, 0);
    analogWrite(m4_in, 0);

    welcome();
}

void loop() {
    checkVoltage();

    checkSensors();

    for (int i = 0; i < checkDelay; i++){
        enterSleep();
    }
}

void enterSleep()
{
    /* Clear the timer counter register.
    * You can pre-load this register with a value in order to
    * reduce the timeout period, say if you wanted to wake up
    * ever 4.0 seconds exactly.
    */
    TCNT1 = 0x0000;

    /* Configure the prescaler for 1:1024, giving us a
    * timeout of 4.09 seconds.
    */
    TCCR1B = 0x05;

    set_sleep_mode(SLEEP_MODE_IDLE);

    sleep_enable();


    /* Disable all of the unused peripherals. This will reduce power
    * consumption further and, more importantly, some of these
    * peripherals may generate interrupts that will wake our Arduino from
    * sleep!
    */
    power_adc_disable();
    power_spi_disable();
    power_timer0_disable();
    power_timer2_disable();
    power_twi_disable();

    /* Now enter sleep mode. */
    sleep_mode();

    /* The program will continue from here after the timer timeout*/
    sleep_disable(); /* First thing to do is disable sleep. */

    /* Re-enable the peripherals. */
    power_all_enable();

    TCCR1B = 0x1;
}

void checkSensors()
{
    for (byte i = 0; i < sizeof(sensors); i++)
    {
        digitalWrite(motorLeds[i], HIGH);

        int sensor, sum = 0;
        for (byte j = 0; j < sensorTries; j++)
        {
            sensor = analogRead(sensors[i]);
            sum += sensor;
            delay(100);
        }
        sensor = (int)((double)sum / (double)sensorTries);

        Serial.print("Sensor ");
        Serial.print(i + 1);
        Serial.print(" : ");
        Serial.println(sensor);
        if (sensor > sensorThresholds[i])
        {
            Serial.print("Pomp ");
            Serial.print(i + 1);
            Serial.println(" ON");
            analogWrite(pomps[i], 255);
            delay(pompDelay);
            analogWrite(pomps[i], 0);
            Serial.print("Pomp ");
            Serial.print(i + 1);
            Serial.println(" OFF");
        }
        else
        {
            delay(500);
        }

        digitalWrite(motorLeds[i], LOW);
    }
}

void checkVoltage()
{
    int vcc = analogRead(vcc_read);
    if (vcc > vcc_min)
    {
        Serial.println("VCC OK");
    }
    else{
        Serial.println("VCC Too low!");

        digitalWrite(13, HIGH);
        digitalWrite(led_red, HIGH);
        delay(1000);
        digitalWrite(13, LOW);
        digitalWrite(led_red, LOW);
        delay(1000);
        digitalWrite(13, HIGH);
        digitalWrite(led_red, HIGH);
        delay(1000);
        digitalWrite(13, LOW);
        digitalWrite(led_red, LOW);

        CLKPR = 0x80;
        CLKPR = 0x08;
                
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();

        digitalWrite(13, LOW);                 
        digitalWrite(LED_BUILTIN, LOW);
           
        SPCR = 0;

        power_adc_disable();
        power_spi_disable();
        power_timer0_disable();
        power_timer1_disable();
        power_timer2_disable();
        power_twi_disable();

        sleep_mode();
    }
}

void welcome()
{
    Serial.println("Welcome to podlewacz v0.5 by Sh4DoV");

    for (byte i = 0; i < sizeof(allLeds); i++)
    {
        digitalWrite(allLeds[i], HIGH);
        delay(200);
        digitalWrite(allLeds[i], LOW);
    }
}
