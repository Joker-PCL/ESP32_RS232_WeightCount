#include <WiFi.h>
#include <WiFiMulti.h>

//  multitask
TaskHandle_t Task0;
TaskHandle_t Task1;


WiFiMulti wifiMulti;

const char* ssid1 = "pcl_plant1";
const char* ssid2 = "pcl_plant2";
const char* ssid3 = "pcl_plant3";
const char* ssid4 = "pcl_plant4";
const char* ssid5 = "weight_table";
const char* password = "plant172839";

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// tank label
uint8_t tank_wheelA[8] = { 0x1F, 0x1F, 0x1B, 0x11, 0x11, 0x1B, 0x1F, 0x1F };
uint8_t tank_wheelB[8] = { 0x1F, 0x1F, 0x0E, 0x11, 0x11, 0x0E, 0x1F, 0x1F };
uint8_t tank_wheel_L[8] = { 0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07 };
uint8_t tank_wheel_R[8] = { 0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1C };
uint8_t tank_head_L[8] = { 0x00, 0x01, 0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F };
uint8_t tank_head_R[8] = { 0x00, 0x10, 0x1C, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F };
uint8_t tank_gun[8] = { 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00 };
uint8_t bullet_mini[8] = { 0x00, 0x00, 0x0D, 0x1E, 0x1E, 0x0D, 0x00, 0x00 };

// แสดงผลแบบเรียงอักษร
void textEnd(String text, int cols, int rows) {
  for (int i = 0; i < text.length(); i++) {
    lcd.setCursor(cols + i, rows);
    lcd.print(text[i]);
    delay(100);
  }
}

// ลบอักษรหน้าจอแบบกำหนด แถว ตำแหน่ง จำนวน
void clearScreen(int row) {
  for (int i = 0; i < 20; i++) {
    lcd.setCursor(i, row);
    lcd.print(" ");
    // delay(50);
  }
}