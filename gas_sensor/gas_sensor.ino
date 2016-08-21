#include <PCD8544.h>
#include <EEPROM.h>

static const byte lcd_light = 2;
static const byte lcd_clk = 4;
static const byte lcd_din = 5;
static const byte lcd_dc = 6;
static const byte lcd_ce = 7;
static const byte lcd_rst = 8;

static const byte btn_up = A0;
static const byte btn_down = A1;
static const byte btn_right = A2;
static const byte btn_enter = A3;
static const byte btn_left = A4;

static const byte piezo = A5;

static const byte sensor = A6;
static const byte sensor_on = 3;

// The dimensions of the LCD (in pixels)...
static const byte lcd_width = 84;
static const byte lcs_height = 48;

static PCD8544 lcd = PCD8544(lcd_clk, lcd_din, lcd_dc, lcd_rst, lcd_ce);


void setup() {
    Serial.begin(115200);

    setupPins();
    setDefautValues();

    lcd.begin(lcd_width, lcs_height);

    welcomeScreen();
}

void setDefautValues() {
    digitalWrite(lcd_light, LOW);
    digitalWrite(sensor_on, HIGH);
    digitalWrite(piezo, LOW);
}

void setupPins() {
    pinMode(lcd_light, OUTPUT);

    pinMode(btn_up, INPUT);
    pinMode(btn_down, INPUT);
    pinMode(btn_right, INPUT);
    pinMode(btn_left, INPUT);
    pinMode(btn_enter, INPUT);

    pinMode(sensor_on, OUTPUT);
    pinMode(sensor, INPUT);

    pinMode(piezo, OUTPUT);
}

void welcomeScreen(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("Gas sensor");
    lcd.setCursor(0, 2);
    lcd.print("v1.0");
    lcd.setCursor(0, 4);
    lcd.println("by Sh4DoV");

    bool on = true;
    int val = 1024;
    for (int i = 0; i < 8; i++){
        if (on)
        {
            analogWrite(piezo, val);
        }
        else{
            analogWrite(piezo, 0);
        }
        delay(100);
        on = !on;
    }
    analogWrite(piezo, 0);
}

int calibrationTime = 60*3;
int calibrationOffset = 0;
int calibrationMinTime = calibrationTime - 30;

void loop() {
    int value = analogRead(sensor);

    lcd.clear();
    if (calibrationTime > 0){
        lcd.setCursor(0, 0);
        lcd.println("Kalibracja...");
        calibrationTime--;

        if (calibrationOffset == 0)
        {
            calibrationOffset = value;
        }
        else{
            calibrationOffset = (calibrationOffset + value) / 2;
        }

        if (value < 70 && calibrationTime < calibrationMinTime)
        {
            calibrationTime = 0;
        }

        lcd.setCursor(0, 1);
        lcd.print("Offset: ");
        lcd.println(calibrationOffset);
    }
    else{
        lcd.setCursor(0, 0);
        lcd.print("Offset: ");
        lcd.println(calibrationOffset);

        lcd.setCursor(0, 1);
        lcd.print("Raw: ");
        lcd.println(value);

        value = value - calibrationOffset;
        if (value < 0){
            calibrationOffset += value;
        }
    }
    
    lcd.setCursor(0, 3);
    lcd.print("Wartosc: ");
    lcd.println(value);
    
    int piezoTime = (value / 20) * 20;
    lcd.setCursor(0, 5);
    lcd.print("Alarm: ");
    lcd.print(piezoTime);

    int delayTime = 1000;
    
    if (calibrationTime == 0 && piezoTime > 0){
        digitalWrite(piezo, HIGH);
        delay(value);
        digitalWrite(piezo, LOW);
        delayTime -= value;
        delayTime = delayTime < 0 ? 0 : delayTime;
    }

    delay(delayTime);  
}
