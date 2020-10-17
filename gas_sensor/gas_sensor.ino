#include "../libs/SetTimerAction.h"
#include "../libs/SetIntAction.h"
#include "../libs/DataStorage.h"
#include "../libs/Keyboard.h"
#include "../libs/MenuAction.h"
#include "../libs/Menu.h"
#include "CalibrationAction.h"

#include <avr/eeprom.h>
const byte DEFAULTS[] EEMEM = {180, 0, 0, 0, 30, 0, 0, 0, 70, 0, 20, 0};
const long TIMERS_DEFAULT[] EEMEM = { 3 * 60, 30 };
const int INT_DEFAULTS[] EEMEM = { 70, 20 };

#include <PCD8544.h>
#include <EEPROM.h>
#include <StandardCplusplus.h>
#include <list>

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

String start = "   Start    ";
String options = "   Opcje    ";
String calibration = " Kalibracja ";
String calibrationMaxTime = "  Max czas  ";
String calibrationMinTimeText = "  Min czas  ";
String calibrationThreshold = "    Prog    ";
String piezoFactor = "   Piezo    ";
String proobingFactor = "Probkowanie ";

void loop() {
    DataStorage storage = DataStorage(5);
    Keyboard keyboard = Keyboard(btn_up, btn_down, btn_left, btn_right, btn_enter);

    SetTimerAction setCalibrationMaxTime = SetTimerAction(&calibrationMaxTime, (unsigned long)(3 * 60), &storage, &lcd, &keyboard);
    SetTimerAction setCalibrationMinTime = SetTimerAction(&calibrationMinTimeText, (unsigned long)(30), &storage, &lcd, &keyboard);
    SetIntAction setCalibrationThreshold = SetIntAction(0, 1023, (int)70, &calibrationThreshold, &storage, &lcd, &keyboard);
    SetIntAction setPiezoFactor = SetIntAction(1, 100, (int)10, &piezoFactor, &storage, &lcd, &keyboard);
    SetIntAction setProobingFactor = SetIntAction(1, 1000, (int)100, &proobingFactor, &storage, &lcd, &keyboard);

    storage.setDefault();
    
    SensorAction sensorAction = SensorAction(sensor, piezo, &setPiezoFactor, &setProobingFactor, &lcd, &keyboard);
    CalibrationAction calibrationAction = CalibrationAction(&sensorAction, sensor_on, sensor, &setCalibrationMaxTime, &setCalibrationMinTime, &setCalibrationThreshold, &lcd, &keyboard);
    
    std::list<Menu> calibrationMenu;
    calibrationMenu.push_back(Menu(&calibrationMaxTime, &setCalibrationMaxTime));
    calibrationMenu.push_back(Menu(&calibrationMinTimeText, &setCalibrationMinTime));
    calibrationMenu.push_back(Menu(&calibrationThreshold, &setCalibrationThreshold));
    calibrationMenu.push_back(Menu(&proobingFactor, &setProobingFactor));

    MenuAction calibrationMenuAction = MenuAction(calibrationMenu, &lcd, &keyboard, true);

    std::list<Menu> optionsMenu;
    optionsMenu.push_back(Menu(&calibration, &calibrationMenuAction));
    optionsMenu.push_back(Menu(&piezoFactor, &setPiezoFactor));

    MenuAction optionsAction = MenuAction(optionsMenu, &lcd, &keyboard, true);

    std::list<Menu> menu;
    menu.push_back(Menu(&start, &calibrationAction));
    menu.push_back(Menu(&options, &optionsAction));

    MenuAction menuAction = MenuAction(menu, &lcd, &keyboard, false);
    menuAction.execute();
}

void setup() {
    Serial.begin(115200);
        
    setupPins();
    setDefautValues();

    lcd.begin(lcd_width, lcs_height);

    welcomeScreen();
}

void setDefautValues() {
    digitalWrite(lcd_light, LOW);
    digitalWrite(sensor_on, LOW);
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

void loopOld() {
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
