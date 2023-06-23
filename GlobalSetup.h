// time sever
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;
char timeStringBuff[50];  //50 chars should be enough

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

// Google script ID and required credentials
String host = "https://script.google.com/macros/s/";
String GOOGLE_SCRIPT_ID = "AKfycbwPzTux1m1_9ESA7L0y_aIanjtEFkIZIeC8XhohXGwzAlfzjogOTG92L_W6e024Z7vO";  // change Gscript ID

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

String readString; // cache serail port
int Master;  // set master
int currentWeight; // cache weight

unsigned int currentTime = 0;  // time stamp millis()

int autoprtint_state = 0; // relay autoprint state
bool autoprtint = false;  // on-off autoprint

int machineID_address = 0;  // machine address EEPROM
unsigned long machineID = 20104;  // machine ID

int total_address = 10; // total address EEPROM
unsigned int Total; // total
unsigned int count; // cache count

unsigned long pressTime_menu = 0;
unsigned long pressTime_countReset = 0;
unsigned int autoPrint_delay = 2500;  // sensorDelay ms.
int sensor_type = 0;  // sensorType 0=nc,1=nc

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