#include "SetTimerAction.h"
#include "TimerAction.h"
#include "DataStorage.h"
#include "BackLightAction.h"
#include "Keyboard.h"
#include "MenuAction.h"
#include "Menu.h"

#include <PCD8544.h>
#include <EEPROM.h>
#include <StandardCplusplus.h>
#include <list>

String cureMask = "Cure mask";
String cureSolder = "Cure solder";
String hardeningMask = "Hard. mask";
String hardeningSolder = "Hard. solder";
String options = "Options";
String backLight = "Back light";

static const byte lcd_rst = 4;
static const byte lcd_ce = 3;
static const byte lcd_dc = 2;
static const byte lcd_din = 7;
static const byte lcd_clk = 8;
static const byte lcd_bl = 12;

// The dimensions of the LCD (in pixels)...
static const byte lcd_width = 84;
static const byte lcs_height = 48;

static const byte btn_up = 5;
static const byte btn_down = 9;
static const byte btn_right = 11;
static const byte btn_left = 6;
static const byte btn_enter = 10;

static const byte led_on = A3;

static PCD8544 lcd = PCD8544(lcd_clk, lcd_din, lcd_dc, lcd_rst, lcd_ce);

void setup() {
    Serial.begin(115200);

    pinMode(lcd_bl, OUTPUT);
    pinMode(btn_up, INPUT);
    pinMode(btn_down, INPUT);
    pinMode(btn_right, INPUT);
    pinMode(btn_left, INPUT);
    pinMode(btn_enter, INPUT);
    pinMode(led_on, OUTPUT);

    digitalWrite(lcd_bl, HIGH);
    analogWrite(led_on, 0);

    lcd.begin(lcd_width, lcs_height);
    printInfo();
}

void loop() {
    Serial.println("!!! START !!!");
    DataStorage storage = DataStorage(5);
    Keyboard keyboard = Keyboard(btn_up, btn_down, btn_left, btn_right, btn_enter);

    BackLightAction backLightAction = BackLightAction(lcd_bl, &storage, &lcd, &keyboard);
    SetTimerAction setCureMaskTimerAction = SetTimerAction(&cureMask, &storage, &lcd, &keyboard);
    SetTimerAction setCureSolderTimerAction = SetTimerAction(&cureSolder, &storage, &lcd, &keyboard);
    SetTimerAction setHardeningMaskTimerAction = SetTimerAction(&hardeningMask, &storage, &lcd, &keyboard);
    SetTimerAction setHardeningSolderTimerAction = SetTimerAction(&hardeningSolder, &storage, &lcd, &keyboard);

    std::list<Menu> optionsMenu;

    optionsMenu.push_back(Menu(&backLight, &backLightAction));
    optionsMenu.push_back(Menu(&cureMask, &setCureMaskTimerAction));
    optionsMenu.push_back(Menu(&cureSolder, &setCureSolderTimerAction));
    optionsMenu.push_back(Menu(&hardeningMask, &setHardeningMaskTimerAction));
    optionsMenu.push_back(Menu(&hardeningSolder, &setHardeningSolderTimerAction));

    MenuAction optionsAction = MenuAction(optionsMenu, &lcd, &keyboard, true);

    TimerAction timerCureMaskAction = TimerAction(led_on, &lcd, &keyboard, &setCureMaskTimerAction, &backLightAction);
    TimerAction timerCureSolderAction = TimerAction(led_on, &lcd, &keyboard, &setCureSolderTimerAction, &backLightAction);
    TimerAction timerHardeningMaskAction = TimerAction(led_on, &lcd, &keyboard, &setHardeningMaskTimerAction, &backLightAction);
    TimerAction timerHardeningSolderAction = TimerAction(led_on, &lcd, &keyboard, &setHardeningSolderTimerAction, &backLightAction);

    std::list<Menu> menu;
    menu.push_back(Menu(&options, &optionsAction));
    menu.push_back(Menu(&cureMask, &timerCureMaskAction));
    menu.push_back(Menu(&cureSolder, &timerCureSolderAction));
    menu.push_back(Menu(&hardeningMask, &timerHardeningMaskAction));
    menu.push_back(Menu(&hardeningSolder, &timerHardeningSolderAction));

    MenuAction menuAction = MenuAction(menu, &lcd, &keyboard, false);
    menuAction.execute();
}

void printInfo()
{
    digitalWrite(lcd_bl, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("UV box driver");
    lcd.setCursor(0, 2);
    lcd.println("by Sh4DoV");
    lcd.setCursor(0, 4);
    lcd.print("v1.1");
    delay(2000);
    lcd.clear();
    digitalWrite(lcd_bl, HIGH);
}
