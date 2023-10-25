#include <WiFi.h>
#include <WiFiMulti.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "esp_task_wdt.h"
#include "GlobalSetup.h"
#include "SendToCloud_AutoUpdate.h"

// RS232 PINS
#define RXD2 16
#define TXD2 17

unsigned long passed_previousTime = 0;

bool total_update = false;

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

  // ตั้งค่าสำหรับ upload program ครั้งแรก
  // EEPROM.writeUInt(machineID_address, machineID);
  // EEPROM.writeUInt(total_address, 0);
  // EEPROM.commit();

  machineID = EEPROM.readUInt(machineID_address);
  Total = EEPROM.readUInt(total_address);

  Serial.println("machineID: " + String(machineID));
  Serial.println("Total: " + String(Total));

  int numNetworks = sizeof(ssidArray) / sizeof(ssidArray[0]);
  for (int i = 0; i < numNetworks; i++) {
    wifiMulti.addAP(ssidArray[i], password);
  }

  pinMode(btn_select, INPUT_PULLUP);
  pinMode(btn_confirm, INPUT_PULLUP);

  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  const int test_output[3] = { BUZZER, LED_RED, LED_GREEN };
  for (int i = 0; i < 3; i++) {
    digitalWrite(test_output[i], HIGH);
    delay(200);
    digitalWrite(test_output[i], LOW);
  };

  digitalWrite(LED_STATUS, LOW);

  lcd.setCursor(5, 0);
  lcd.print("LOADING...");
  onLoad();
  textEnd("          ", 5, 0);
  textEnd("POLIPHARM CO,LTD", 2, 0);
  textEnd("VERSION " + String(FirmwareVer), 4, 1);
  delay(500);
  clearScreen(1);
  textEnd("ID " + String(machineID), 5, 1);
  delay(500);
  clearScreen(0);

  // esp32 auto update setup
  pinMode(button_boot.PIN, INPUT);
  attachInterrupt(button_boot.PIN, isr, RISING);

  // check Device
  if (digitalRead(btn_select) == 0) {
    digitalWrite(BUZZER, HIGH);
    delay(500);
    digitalWrite(BUZZER, LOW);
    delay(1000);
    checkDevice();
  }

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  // start program
  xTaskCreatePinnedToCore(autoUpdate, "Task0", 100000, NULL, 10, &Task0, 0);
  xTaskCreatePinnedToCore(readButton, "Task1", 5000, NULL, 9, &Task1, 0);
  xTaskCreatePinnedToCore(mainLoop, "Task2", 20000, NULL, 8, &Task2, 1);
}

void loop() {}

void readButton(void *val) {
  for (;;) {
    if (Master) {
      // total -1
      btn_confirm_currentstate = digitalRead(btn_confirm);
      if (btn_confirm_currentstate == 0 && btn_confirm_previousstate == 1 && Total > 0) {
        delay(100);
        Total -= 1;
        EEPROM.writeUInt(total_address, Total);
        EEPROM.commit();
        Serial.println("TOTAL: " + String(Total) + " PCS");
        digitalWrite(BUZZER, HIGH);
        delay(100);
        digitalWrite(BUZZER, LOW);
        total_update = true;

        if (count > 0)
          count--;
      }

      btn_confirm_previousstate = btn_confirm_currentstate;

      // Reset Couter
      if (!digitalRead(btn_select) && Total > 0) {
        delay(100);
        if (pressTime_countReset == 0) {
          pressTime_countReset = millis();  // บันทึกเวลาเริ่มต้นการกดค้าง
        }
        if ((millis() - pressTime_countReset) > 2000) {  // ตรวจสอบเวลาการกดค้าง
          Serial.println("Reset Counter>>");
          Total = 0;
          EEPROM.writeUInt(total_address, Total);
          EEPROM.commit();
          digitalWrite(BUZZER, HIGH);
          delay(100);
          digitalWrite(BUZZER, LOW);
          pressTime_countReset = 0;  // รีเซ็ตเวลาเริ่มต้นการกดค้าง
          total_update = true;
        }
      } else {
        pressTime_countReset = 0;  // รีเซ็ตเวลาเริ่มต้นการกดค้าง
      }
    }

    delay(100);
  }
}

void mainLoop(void *val) {
  for (;;) {
    if (!Master) {
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("SET PRIMARY VALUE");
      lcd.setCursor(10, 3);
      lcd.print("<CONFIRM>");
      lcd.setCursor(1, 3);
      lcd.print("<NEXT>");

      lcd.setCursor(0, 2);
      lcd.print("PRIMARY : ");
      lcd.blink();
      Master = setMaster();
    } else {
      clearScreen(0);
      lcd.noBlink();
      lcd.setCursor(4, 0);
      lcd.print("<< READY >>");
      lcd.setCursor(0, 1);
      lcd.print("TOTAL: " + String(Total) + " PCS");
      lcd.setCursor(0, 2);
      lcd.print("PRIMARY : " + String(Master) + " PCS");
      clearScreen(3);
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
      lcd.setCursor(0, 1);
      lcd.print("TOTAL: " + String(Total) + " PCS");
      EEPROM.writeUInt(total_address, Total);
      EEPROM.commit();
      delay(200);

      if (currentWeight == Master) {
        count++;
        Serial.println("Passed: " + String(currentWeight) + " PCS");
        clearScreen(3);
        lcd.setCursor(7, 3);
        lcd.print("PASSED");
        digitalWrite(LED_GREEN, HIGH);
        passed_previousTime = millis();
      } else {
        countNC++;
        Serial.println("Fail: " + String(currentWeight) + " PCS");
        clearScreen(3);
        lcd.setCursor(4, 3);
        lcd.print("<< FAILED >>");
        alert();
      }
    }
  }
}

// Set master value and check Device
int setMaster() {
  int master_selected = 0;
  unsigned long pressTime_checkDevice = 0;

  Serial.println("PRIMARY:" + String(MasterList[master_selected]) + " PCS");
  lcd.setCursor(10, 2);
  lcd.print(String(MasterList[master_selected]) + " PCS ");

  while (Master <= 0) {
    // check device
    // if (digitalRead(btn_select) == 0) {
    //   delay(100);
    //   if (pressTime_checkDevice == 0) {
    //     pressTime_checkDevice = millis();  // บันทึกเวลาเริ่มต้นการกดค้าง
    //   }
    //   if ((millis() - pressTime_checkDevice) > 2000) {  // ตรวจสอบเวลาการกดค้าง
    //     lcd.noBlink();
    //     digitalWrite(BUZZER, HIGH);
    //     delay(500);
    //     digitalWrite(BUZZER, LOW);
    //     delay(1000);
    //     pressTime_checkDevice = 0;  // รีเซ็ตเวลาเริ่มต้นการกดค้าง
    //     checkDevice();
    //   }
    // } else {
    //   pressTime_checkDevice = 0;  // รีเซ็ตเวลาเริ่มต้นการกดค้าง
    // }

    // select
    btn_select_currentstate = digitalRead(btn_select);
    if (btn_select_currentstate == 0 && btn_select_previousstate == 1) {
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      delay(100);
      master_selected++;
      if (master_selected >= master_number) {
        master_selected = 0;
      }

      Serial.println("PRIMARY:" + String(MasterList[master_selected]) + " PCS");
      lcd.setCursor(10, 2);
      lcd.print(String(MasterList[master_selected]) + " PCS  ");
    }

    btn_select_previousstate = btn_select_currentstate;

    // Confirm
    if (digitalRead(btn_confirm) == 0) {
      delay(100);
      if (pressTime_countReset == 0) {
        pressTime_countReset = millis();  // บันทึกเวลาเริ่มต้นการกดค้าง
      }
      if ((millis() - pressTime_countReset) > 2000) {  // ตรวจสอบเวลาการกดค้าง
        lcd.noBlink();
        Serial.println("<< Confirm >>");
        clearScreen(3);
        lcd.setCursor(3, 3);
        lcd.print("<< CONFIRM >>");
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(1000);
        pressTime_countReset = 0;  // รีเซ็ตเวลาเริ่มต้นการกดค้าง

        return MasterList[master_selected];
      }
    } else {
      pressTime_countReset = 0;  // รีเซ็ตเวลาเริ่มต้นการกดค้าง
    }
  }
}

void checkDevice() {
  unsigned long previousMillis1 = 0;
  unsigned long previousMillis2 = 0;
  bool ledState = false;
  String serialMonitor;
  String serialMonitor_cache;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: NULL");
  lcd.setCursor(0, 1);
  lcd.print("RSSI: NULL");
  lcd.setCursor(0, 2);
  lcd.print("Sensor: ");
  lcd.setCursor(0, 3);
  lcd.print("RS232: NULL");

  while (true) {
    if (digitalRead(SENSOR) == 1) {
      digitalWrite(LED_RED, HIGH);
      lcd.setCursor(8, 2);
      lcd.print("ON ");
    } else {
      digitalWrite(LED_RED, LOW);
      lcd.setCursor(8, 2);
      lcd.print("OFF");
    }

    if (wifiMulti.run() == WL_CONNECTED) {
      lcd.setCursor(6, 0);
      lcd.print(WiFi.SSID());
      lcd.setCursor(6, 1);
      int rssi = WiFi.RSSI();
      if (rssi >= -50) {
        lcd.print("Excellent");
      } else if (rssi >= -70) {
        lcd.print("Good     ");
      } else {
        lcd.print("Weak     ");
      }

      if (millis() - previousMillis1 >= 1000) {
        previousMillis1 = millis();  // บันทึกค่าเวลาปัจจุบัน
        if (ledState) {
          digitalWrite(LED_GREEN, HIGH);
        } else {
          digitalWrite(LED_GREEN, LOW);
        }

        ledState = !ledState;
      }
    } else {
      digitalWrite(LED_GREEN, LOW);
    }

    if (Serial2.available() > 0) {
      lcd.setCursor(7, 3);
      lcd.print("OK  ");
    } else {
      lcd.setCursor(7, 3);
      lcd.print("null");
    }

    Serial2.read();
    
    // char incomingByte;
    // static char receivedData[10];  // อาร์เรย์เก็บข้อมูลที่อ่านได้
    // static int dataIndex = 0;      // ดัชนีของอาร์เรย์ receivedData

    // //  check Serial Data cache
    // if (Serial2.available() > 0) {
    //   incomingByte = Serial2.read();             // อ่านค่าที่ส่งมาจาก RS232
    //   if (incomingByte != '\n') {                // เมื่อไม่พบตัวขึ้นบรรทัดใหม่
    //     receivedData[dataIndex] = incomingByte;  // เก็บค่าที่อ่านได้ในอาร์เรย์
    //     dataIndex++;
    //   } else {                           // เมื่อพบตัวขึ้นบรรทัดใหม่
    //     receivedData[dataIndex] = '\0';  // ตั้งค่าตัวสิ้นสุดสตริง
    //     serialMonitor = receivedData;
    //     dataIndex = 0;  // เริ่มต้นดัชนีใหม่สำหรับอาร์เรย์ receivedData
    //   }
    // }

    // if (serialMonitor.length() > 0) {
    //   serialMonitor.replace("n", "");

    //   if (serialMonitor_cache != serialMonitor) {
    //     serialMonitor.trim();
    //     lcd.setCursor(7, 3);
    //     lcd.print(serialMonitor);
    //     serialMonitor = "";
    //     Serial2.flush();
    //   }

    //   serialMonitor_cache = serialMonitor;
    // }

    if (digitalRead(btn_confirm) == 0) {
      return;
    }
  }
}

// read SerialPort RS232
String readString;
int PCS = 0;
int PCS_Cache = 0;
unsigned long PCS_TimerCheck = 0;

int Sensor_CurrentState = 0;
bool Sensor_PreviousState = true;

int readSerial() {
  Serial.println("ReadSerialPort>>");

  PCS = 0;
  PCS_Cache = 0;
  PCS_TimerCheck = 0;

  while (true) {
    if ((millis() - passed_previousTime) > 500) {
      digitalWrite(LED_GREEN, LOW);
    }

    // update screen
    if (total_update) {
      lcd.noBlink();
      String Total_SCR = "TOTAL: " + String(Total) + " PCS";
      while (Total_SCR.length() < 20) Total_SCR += " ";

      lcd.setCursor(0, 1);
      lcd.print(Total_SCR);
      total_update = false;
      lcd.setCursor(10, 3);
      lcd.blink();
    }

    Sensor_CurrentState = digitalRead(SENSOR);
    if (Sensor_CurrentState == 1) {
      char incomingByte;
      static char receivedData[10];  // อาร์เรย์เก็บข้อมูลที่อ่านได้
      static int dataIndex = 0;      // ดัชนีของอาร์เรย์ receivedData

      //  check Serial Data cache
      if (Serial2.available() > 0 && Sensor_PreviousState == true) {
        incomingByte = Serial2.read();             // อ่านค่าที่ส่งมาจาก RS232
        if (incomingByte != '\n') {                // เมื่อไม่พบตัวขึ้นบรรทัดใหม่
          receivedData[dataIndex] = incomingByte;  // เก็บค่าที่อ่านได้ในอาร์เรย์
          dataIndex++;
        } else {                           // เมื่อพบตัวขึ้นบรรทัดใหม่
          receivedData[dataIndex] = '\0';  // ตั้งค่าตัวสิ้นสุดสตริง
          readString = receivedData;
          dataIndex = 0;  // เริ่มต้นดัชนีใหม่สำหรับอาร์เรย์ receivedData
        }
      }

      // check pcs
      if (readString.length() > 0 && Sensor_PreviousState == true) {
        readString.replace("+", "");
        readString.replace("pcs", "");
        int current_pcs = readString.toInt();

        if (readString.indexOf("-") && PCS_Cache == current_pcs && current_pcs > 0) {
          if (PCS_TimerCheck == 0) {
            PCS_TimerCheck = millis();  // บันทึกเวลาเริ่มต้นการวาง
          }

          if ((millis() - PCS_TimerCheck) > 700) {  // ตรวจสอบเวลาการกดค้าง
            Sensor_PreviousState = false;           // เปลี่ยนสถานะการรับข้อมูล
            PCS_TimerCheck = 0;                     // รีเซ็ตเวลาเริ่มต้นการวาง
            readString = "";
            digitalWrite(BUZZER, HIGH);
            delay(100);
            digitalWrite(BUZZER, LOW);
            delay(100);
            Serial2.flush();
            return current_pcs;
          }
        } else {
          PCS_Cache = current_pcs;  // รีเซ็ตค่าน้ำหนักที่นำมาเปรียบเทียบ
          PCS_TimerCheck = 0;       // รีเซ็ตเวลาเริ่มต้นการวาง
        }

        readString = "";

        Serial.print("current_pcs: ");
        Serial.println(current_pcs);  // พิมพ์สตริงที่แปลงจากค่าที่อ่านได้
      }
    } else {
      readString = "";
      PCS_Cache = 0;
      PCS_TimerCheck = 0;           // รีเซ็ตเวลาเริ่มต้นการวาง
      Sensor_PreviousState = true;  // เปลี่ยนสถานะการรับข้อมูล
      Serial2.read();
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

void alert() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER, HIGH);
    lcd.noBacklight();
    delay(100);
    lcd.backlight();
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
}
