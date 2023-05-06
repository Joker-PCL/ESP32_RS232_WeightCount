#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// RS232 PINS
#define RXD2 16
#define TXD2 17

// INPUT AND OUTPUT
const int SENSOR = 12;
const int LED_RED = 25;
const int LED_GREEN = 26;
const int BUZZER1 = 32;
const int BUZZER2 = 4;
const int AUTOPRINT = 33;

const int btn_exit = 34;
const int btn_down = 35;
const int btn_up = 36;
const int btn_menu = 39;

int selectmenu = 1;
bool settingMode = false;

String readString;
int Master; // set master
int currentWeight;

uint8_t tank_wheelA[8] = {0x1F, 0x1F, 0x1B, 0x11, 0x11, 0x1B, 0x1F, 0x1F};
uint8_t tank_wheelB[8] = {0x1F, 0x1F, 0x0E, 0x11, 0x11, 0x0E, 0x1F, 0x1F};
uint8_t tank_wheel_L[8] = {0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07};
uint8_t tank_wheel_R[8] = {0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C};
uint8_t tank_head_L[8] = {0x00, 0x01, 0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F};
uint8_t tank_head_R[8] = {0x00, 0x10, 0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F};
uint8_t tank_gun[8] = {0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00};
uint8_t bullet_mini[8] = {0x00, 0x00, 0x0D, 0x1E, 0x1E, 0x0D, 0x00, 0x00};

unsigned int currentTime = 0; // time stamp millis()
unsigned int Total;           // total

int currentState = 0;          // relay autoprint state
bool autoprtint_state = false; // on-off autoprint

int total_address = 0;        // total address EEPROM
int sensor_address = 10;      // sensorType address EEPROM
int sensorDelay_address = 20; // sensorDelay address EEPROM

unsigned long pressTime_menu = 0;
unsigned long pressTime_countReset = 0;
unsigned int autoPrint_delay = 2500; // sensorDelay ms.
int sensor_type = 0;                 // sensorType 0=nc,1=nc

void setup()
{
    // initialize the LCD
    lcd.begin();
    // Turn on the blacklight and print a message.
    lcd.backlight();
    lcd.createChar(0, tank_wheelA);
    lcd.createChar(1, tank_wheelB);
    lcd.createChar(2, tank_wheel_L);
    lcd.createChar(3, tank_wheel_R);
    lcd.createChar(4, tank_head_L);
    lcd.createChar(5, tank_head_R);
    lcd.createChar(6, tank_gun);
    lcd.createChar(7, bullet_mini);
    lcd.home();

    // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

    EEPROM.begin(100);
    Total = EEPROM.readUInt(total_address);
    sensor_type = EEPROM.readUInt(sensor_address);
    autoPrint_delay = EEPROM.readUInt(sensorDelay_address);

    if (sensor_type > 1)
        sensor_type = 1;
    else
        sensor_type = sensor_type;

    if (autoPrint_delay > 7000)
        autoPrint_delay = 7000;
    else
        autoPrint_delay = autoPrint_delay;

    Serial.println("Total: " + String(Total));
    Serial.println("sensorType: " + String(sensor_type));
    Serial.println("sensorDelay: " + String(autoPrint_delay));

    pinMode(btn_menu, INPUT_PULLUP);
    pinMode(btn_down, INPUT_PULLUP);
    pinMode(btn_up, INPUT_PULLUP);
    pinMode(btn_exit, INPUT_PULLUP);

    pinMode(SENSOR, INPUT_PULLUP);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(BUZZER1, OUTPUT);
    pinMode(BUZZER2, OUTPUT);
    pinMode(AUTOPRINT, OUTPUT);

    const int test_output[5] = {BUZZER1, BUZZER2, LED_RED, LED_GREEN, AUTOPRINT};
    for (int i = 0; i < 5; i++)
    {
        digitalWrite(test_output[i], HIGH);
        delay(200);
        digitalWrite(test_output[i], LOW);
    };

    lcd.setCursor(5, 0);
    lcd.print("LOADING...");
    onLoad();
    textEnd("          ", 5, 0);
    textEnd("POLIPHARM CO,LTD", 2, 0);
    delay(500);
    clearScreen(0);
}

void loop()
{
    if (!Master)
    {
        autoprtint_state = false;
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("SET PRIMARY VALUE");
        lcd.setCursor(1, 3);
        lcd.print("PRESS PRINT BUTTON");

        Serial.print("SetMaster: ");
        lcd.setCursor(0, 2);
        lcd.print("PRIMARY : ");
        lcd.blink();
        Master = readSerial();
        lcd.noBlink();
        Serial.println(String(Master) + " PCS");
        clearScreen(0);
    }
    else
    {
        autoprtint_state = true;
        lcd.setCursor(4, 0);
        lcd.print("<< READY >>");
        lcd.setCursor(0, 1);
        lcd.print("TOTAL: " + String(Total) + " PCS");
        lcd.setCursor(0, 2);
        lcd.print("PRIMARY : " + String(Master) + " PCS");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("WEIGHING: ");
        lcd.blink();

        currentWeight = readSerial();

        clearScreen(0);
        lcd.setCursor(7, 0);
        lcd.print("Wait...");
        lcd.noBlink();
        lcd.setCursor(0, 3);
        lcd.print("WEIGHING: " + String(currentWeight) + " PCS");
        Total++;
        EEPROM.writeUInt(total_address, Total);
        EEPROM.commit();
        delay(500);

        if (currentWeight == 0)
        {
            Restart();
        }
        else if (currentWeight == Master)
        {
            Serial.println("Passed: " + String(currentWeight) + " PCS");
            lcd.setCursor(0, 3);
            lcd.print("                    ");
            lcd.setCursor(7, 3);
            lcd.print("PASSED");
            digitalWrite(LED_GREEN, HIGH);
            delay(500);
            digitalWrite(LED_GREEN, LOW);
        }
        else
        {
            Serial.println("Fail: " + String(currentWeight) + " PCS");
            lcd.setCursor(0, 3);
            lcd.print("                    ");
            lcd.setCursor(4, 3);
            lcd.print("<< FAILED >>");
            alert();
        }
    }
}

// read SerialPort RS232
int readSerial()
{
    Serial.println("ReadSerialPort>>");
    int btn_down_currentstate = 0;
    int btn_down_previousstate = 0;

    while (readString.length() == 0)
    {
        // check autoprint sensor
        if (autoprtint_state == true)
        {
            if (digitalRead(SENSOR) == sensor_type)
            {
                autoPrint();
            }
            else
            {
                currentTime = millis();
                digitalWrite(AUTOPRINT, LOW); // off autoprint relay
                currentState = 0;
            }
        }

        //  check Serial Data cache
        if (Serial2.available() > 0)
            readString = Serial2.readString();

        // check setting mode
        if (!digitalRead(btn_menu))
        { // ถ้าสวิตช์ถูกกด
            if (pressTime_menu == 0)
            {
                pressTime_menu = millis(); // บันทึกเวลาเริ่มต้นการกดค้าง
            }
            if ((millis() - pressTime_menu) > 2000)
            { // ตรวจสอบเวลาการกดค้าง
                Serial.println("SettingMode>>");
                readString = "";
                setting();
                pressTime_menu = 0; // รีเซ็ตเวลาเริ่มต้นการกดค้าง
            }
        }
        else
        {
            pressTime_menu = 0; // รีเซ็ตเวลาเริ่มต้นการกดค้าง
        }

        if (Master)
        {
            bool total_update = false;

            lcd.setCursor(10, 3);
            lcd.blink();

            // total -1
            btn_down_currentstate = digitalRead(btn_down);
            if (btn_down_currentstate == 0 && btn_down_previousstate == 1 && Total > 0)
            {
                Total -= 1;
                EEPROM.writeUInt(total_address, Total);
                EEPROM.commit();
                Serial.println("TOTAL: " + String(Total) + " PCS");
                digitalWrite(BUZZER2, HIGH);
                delay(100);
                digitalWrite(BUZZER2, LOW);
                total_update = true;
            }

            btn_down_previousstate = btn_down_currentstate;

            // Reset Couter
            if (!digitalRead(btn_up) && Total > 0)
            {
                if (pressTime_countReset == 0)
                {
                    pressTime_countReset = millis(); // บันทึกเวลาเริ่มต้นการกดค้าง
                }
                if ((millis() - pressTime_countReset) > 2000)
                { // ตรวจสอบเวลาการกดค้าง
                    Serial.println("Reset Counter>>");
                    Total = 0;
                    EEPROM.writeUInt(total_address, Total);
                    EEPROM.commit();
                    digitalWrite(BUZZER2, HIGH);
                    delay(100);
                    digitalWrite(BUZZER2, LOW);
                    pressTime_countReset = 0; // รีเซ็ตเวลาเริ่มต้นการกดค้าง
                    total_update = true;
                }
            }
            else
            {
                pressTime_countReset = 0; // รีเซ็ตเวลาเริ่มต้นการกดค้าง
            }

            // update screen
            if (total_update)
            {
                String Total_SCR = "TOTAL: " + String(Total) + " PCS";
                for (int i = 0; i < 20 - Total_SCR.length(); i++)
                    Total_SCR += " ";

                lcd.setCursor(0, 1);
                lcd.print(Total_SCR);
                total_update = false;
            }
        }
    }

    if (readString.length() > 0)
    {
        digitalWrite(AUTOPRINT, LOW); // off autoprint relay
        digitalWrite(BUZZER2, HIGH);
        delay(100);
        digitalWrite(BUZZER2, LOW);
        readString.replace("+", "");
        readString.replace("pcs", "");
        int PCS = readString.toInt();
        readString = "";

        return PCS;
    }
}

void onLoad()
{
    for (int i = 0; i < 16; i++)
    {
        lcd.setCursor(i + 1, 2);
        lcd.write(4);
        lcd.write(5);
        lcd.write(6);
        lcd.write(6);

        lcd.setCursor(i, 2);
        lcd.print(" ");
        lcd.setCursor(i, 3);
        lcd.write(2);
        if (i % 2 == 0)
        {
            lcd.write(0);
            lcd.write(0);
        }
        else
        {
            lcd.write(1);
            lcd.write(1);
        }

        lcd.write(3);
        lcd.setCursor(i - 1, 3);
        lcd.print(" ");
        delay(300);
    }

    lcd.setCursor(18, 2);
    lcd.print("  ");
    lcd.setCursor(14, 2);
    lcd.write(6);
    lcd.write(6);

    for (int i = 13; i >= 0; i--)
    {
        lcd.setCursor(i, 2);
        lcd.write(7);
        delay(200);
        if (i != 13)
        {
            lcd.setCursor(i, 2);
            lcd.print("  ");
        }
    }
}

void setting()
{
    String sensor_typeDisplay;
    settingMode = true;
    selectmenu = 1;

    if (sensor_type == 0)
    {
        sensor_typeDisplay = "no";
    }
    else
    {
        sensor_typeDisplay = "nc";
    }

    lcd.clear();
    lcd.noBlink();
    lcd.setCursor(6, 0);
    lcd.print("SETTING");
    lcd.setCursor(0, 1);
    lcd.print("1.sensor: " + String(sensor_typeDisplay));
    lcd.setCursor(0, 2);
    lcd.print("2.delay : " + String(autoPrint_delay) + " ms.");
    lcd.setCursor(4, 3);
    lcd.print("save,-,+,next");
    delay(1500);
    lcd.setCursor(18, selectmenu);
    lcd.print("<<");

    while (settingMode)
    {
        if (!digitalRead(btn_exit))
        {
            EEPROM.writeUInt(sensor_address, sensor_type);
            EEPROM.writeUInt(sensorDelay_address, autoPrint_delay);
            EEPROM.commit();
            lcd.clear();
            lcd.setCursor(5, 0);
            lcd.print("Saving...");
            delay(500);
            lcd.clear();
            settingMode = false;
            loop(); // กลับหน้าหลัก
        }
        else if (!digitalRead(btn_down))
        {
            if (selectmenu == 1)
            {
                lcd.setCursor(10, 1);
                lcd.print("nc");
                sensor_type = 1;
            }
            else if (selectmenu == 0 && autoPrint_delay > 1000)
            {
                autoPrint_delay -= 500;
                lcd.setCursor(10, 2);
                lcd.print(String(autoPrint_delay) + " ms.");
            }
            delay(500);
        }
        else if (!digitalRead(btn_up))
        {
            if (selectmenu == 1)
            {
                lcd.setCursor(10, 1);
                lcd.print("no");
                sensor_type = 0;
            }
            else if (selectmenu == 0 && autoPrint_delay < 7000)
            {
                autoPrint_delay += 500;
                lcd.setCursor(10, 2);
                lcd.print(String(autoPrint_delay) + " ms.");
            }
            delay(500);
        }
        else if (!digitalRead(btn_menu))
        {
            selectmenu += 1;
            if (selectmenu == 1)
            {
                lcd.setCursor(18, selectmenu);
                lcd.print("<<");
                lcd.setCursor(18, selectmenu + 1);
                lcd.print("  ");
            }
            else if (selectmenu == 2)
            {
                lcd.setCursor(18, selectmenu);
                lcd.print("<<");
                lcd.setCursor(18, selectmenu - 1);
                lcd.print("  ");
                selectmenu = 0;
            }

            delay(500);
        }
    }
}

// autoprint
void autoPrint()
{
    if (millis() - currentTime >= autoPrint_delay && currentState == 0)
    {
        Serial.println("AUTOPRINT _ON_");
        currentTime = millis();
        digitalWrite(AUTOPRINT, HIGH);
        currentState = 1;
    }
}

void alert()
{
    for (int i = 0; i < 10; i++)
    {
        digitalWrite(LED_RED, HIGH);
        digitalWrite(BUZZER2, HIGH);
        lcd.noBacklight();
        delay(100);
        lcd.backlight();
        digitalWrite(LED_RED, LOW);
        digitalWrite(BUZZER2, LOW);
        delay(100);
    }
}

// แสดงผลแบบเรียงอักษร
void textEnd(String text, int cols, int rows)
{
    for (int i = 0; i < text.length(); i++)
    {
        lcd.setCursor(cols + i, rows);
        lcd.print(text[i]);
        delay(100);
    }
}

// ลบอักษรหน้าจอแบบกำหนด แถว ตำแหน่ง จำนวน
void clearScreen(int row)
{
    for (int i = 0; i < 20; i++)
    {
        lcd.setCursor(i, row);
        lcd.print(" ");
        // delay(50);
    }
}

void Restart()
{
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("ESP RESTART");
    textEnd("DEV. BY", 7, 1);
    textEnd("NATTAPON PONDONKO", 2, 2);

    delay(1000);
    ESP.restart();
}