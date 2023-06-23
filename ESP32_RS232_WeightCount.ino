#include <Wire.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "esp_task_wdt.h"
#include "GlobalSetup.h"
#include "SendToCloud_AutoUpdate.h"

// RS232 PINS
#define RXD2 16
#define TXD2 17

void setup() {
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

  esp_task_wdt_init(60, true);  // 60 วินาทีสำหรับ wdt

  EEPROM.begin(100);
  EEPROM.writeUInt(machineID_address, machineID);
  EEPROM.writeUInt(total_address, 0);
  EEPROM.commit();
  machineID = EEPROM.readUInt(machineID_address);
  Total = EEPROM.readUInt(total_address);

  Serial.println("machineID: " + String(machineID));
  Serial.println("Total: " + String(Total));

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

  const int test_output[5] = { BUZZER1, BUZZER2, LED_RED, LED_GREEN, AUTOPRINT };
  for (int i = 0; i < 5; i++) {
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

  // esp32 auto update setup
  pinMode(button_boot.PIN, INPUT);
  attachInterrupt(button_boot.PIN, isr, RISING);
  pinMode(LED_BUILTIN, OUTPUT);

  // start program
  xTaskCreatePinnedToCore(autoUpdate, "Task0", 100000, NULL, 10, &Task0, 0);
  xTaskCreatePinnedToCore(mainLoop, "Task1", 30000, NULL, 9, &Task1, 1);
}

void loop() {
}

void mainLoop(void *val) {
  for (;;) {
    if (!Master) {
      autoprtint = false;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("SET PRIMARY VALUE");
      lcd.setCursor(1, 3);
      lcd.print("PRESS PRINT BUTTON");
      
      lcd.setCursor(0, 2);
      lcd.print("PRIMARY : ");
      lcd.blink();
      Master = readSerial();
      lcd.noBlink();
      Serial.println("PRIMARY:" + String(Master) + " PCS");
      clearScreen(0);
    } else {
      autoprtint = true;
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
      count++;
      EEPROM.writeUInt(total_address, Total);
      EEPROM.commit();
      delay(500);

      if (currentWeight == Master) {
        Serial.println("Passed: " + String(currentWeight) + " PCS");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        lcd.setCursor(7, 3);
        lcd.print("PASSED");
        digitalWrite(LED_GREEN, HIGH);
        delay(500);
        digitalWrite(LED_GREEN, LOW);
      } else {
        Serial.println("Fail: " + String(currentWeight) + " PCS");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        lcd.setCursor(4, 3);
        lcd.print("<< FAILED >>");
        alert();
      }
    }
  }
}

// Read serial port RS232
int readSerial() {
  Serial.println("ReadSerialPort>>");
  Serial2.flush();
  int btn_down_currentstate = 0;
  int btn_down_previousstate = 0;
  int PCS = 0;

  while (PCS <= 0) {
    //  check Serial Data cache
    if (Serial2.available() > 0)
      readString = Serial2.readString();

    if (Master) {
      // check autoprint sensor
      if (autoprtint == true) {
        if (digitalRead(SENSOR) == sensor_type) {
          autoPrint();
        } else {
          currentTime = millis();
          digitalWrite(AUTOPRINT, LOW);  // off autoprint relay
          autoprtint_state = 0;
        }
      }

      bool total_update = false;

      lcd.setCursor(10, 3);
      lcd.blink();

      // total -1
      btn_down_currentstate = digitalRead(btn_down);
      if (btn_down_currentstate == 0 && btn_down_previousstate == 1 && Total > 0) {
        Total--;
        EEPROM.writeUInt(total_address, Total);
        EEPROM.commit();
        Serial.println("TOTAL: " + String(Total) + " PCS");
        digitalWrite(BUZZER2, HIGH);
        delay(100);
        digitalWrite(BUZZER2, LOW);
        total_update = true;

        if(count > 0)
          count--;
      }

      btn_down_previousstate = btn_down_currentstate;

      // Reset Couter
      if (!digitalRead(btn_up) && Total > 0) {
        if (pressTime_countReset == 0) {
          pressTime_countReset = millis();  // บันทึกเวลาเริ่มต้นการกดค้าง
        }
        if ((millis() - pressTime_countReset) > 2000) {  // ตรวจสอบเวลาการกดค้าง
          Serial.println("Reset Counter>>");
          Total = 0;
          EEPROM.writeUInt(total_address, Total);
          EEPROM.commit();
          digitalWrite(BUZZER2, HIGH);
          delay(100);
          digitalWrite(BUZZER2, LOW);
          pressTime_countReset = 0;  // รีเซ็ตเวลาเริ่มต้นการกดค้าง
          total_update = true;
        }
      } else {
        pressTime_countReset = 0;  // รีเซ็ตเวลาเริ่มต้นการกดค้าง
      }

      // update screen
      if (total_update) {
        String Total_SCR = "TOTAL: " + String(Total) + " PCS";
        for (int i = 0; i < 20 - Total_SCR.length(); i++)
          Total_SCR += " ";

        lcd.setCursor(0, 1);
        lcd.print(Total_SCR);
        total_update = false;
      }
    }


    if (readString.length() > 0) {
      digitalWrite(AUTOPRINT, LOW);  // off autoprint relay
      digitalWrite(BUZZER2, HIGH);
      delay(100);
      digitalWrite(BUZZER2, LOW);
      readString.replace("+", "");
      readString.replace("pcs", "");
      PCS = readString.toInt();
      readString = "";

      if (PCS > 0)
        return PCS;
    }
  }
}

void onLoad() {
  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i + 1, 2);
    lcd.write(4);
    lcd.write(5);
    lcd.write(6);
    lcd.write(6);

    lcd.setCursor(i, 2);
    lcd.print(" ");
    lcd.setCursor(i, 3);
    lcd.write(2);
    if (i % 2 == 0) {
      lcd.write(0);
      lcd.write(0);
    } else {
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

  for (int i = 13; i >= 0; i--) {
    lcd.setCursor(i, 2);
    lcd.write(7);
    delay(200);
    if (i != 13) {
      lcd.setCursor(i, 2);
      lcd.print("  ");
    }
  }
}

// autoprint
void autoPrint() {
  if(Master <= 3)
    autoPrint_delay = 600;
  else if (Master <= 5)
    autoPrint_delay = 800;
  else if (Master <= 10)
    autoPrint_delay = 1000;
  else if (Master <= 15)
    autoPrint_delay = 1200;
  else if (Master <= 20)
    autoPrint_delay = 1400;
  else if (Master <= 30)
    autoPrint_delay = 1600;
  else if (Master <= 40)
    autoPrint_delay = 1800;
  else if (Master <= 50)
    autoPrint_delay = 2000;
  else
    autoPrint_delay = autoPrint_delay;
  
  if (millis() - currentTime >= autoPrint_delay && autoprtint_state == 0) {
    Serial.println("AUTOPRINT _ON_");
    currentTime = millis();
    digitalWrite(AUTOPRINT, HIGH);
    autoprtint_state = 1;
  }
}

void alert() {
  for (int i = 0; i < 10; i++) {
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
