#include "cert.h"

// set version
String FirmwareVer = {
  "1.0"
};

#define URL_fw_Version "https://raw.githubusercontent.com/Joker-PCL/ESP32_RS232_WeightCount/main/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/Joker-PCL/ESP32_RS232_WeightCount/main/fw.bin"

unsigned long previousMillis = 0;  // will store last time LED was updated
unsigned long previousMillis_2 = 0;
const long interval = 60000;  // update and send to cloud timer
const long mini_interval = 1000; // delay timer

struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button_boot = {
  0,
  0,
  false
};

/*void IRAM_ATTR isr(void* arg) {
    Button* s = static_cast<Button*>(arg);
    s->numberKeyPresses += 1;
    s->pressed = true;
}*/

void sendToCloud() {
  static bool flag = false;
  struct tm timeinfo;

  // Get current time
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  String asString(timeStringBuff);
  asString.replace(" ", "-");
  Serial.print("Time:");
  Serial.println(asString);

  // This will send the request to the server
  String url = host + GOOGLE_SCRIPT_ID + "/exec?";
  url += "machineID=" + String(machineID);
  url += "&";
  url += "amount=" + String(count);

  Serial.print("POST data to spreadsheet:");
  Serial.println(url);

  HTTPClient http;
  http.begin(url.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  url = "";

  //getting response from google sheet
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Payload: " + payload);
    count = 0;  // Reset count
  } else {
    Serial.println("Failed to connect to the server");
  }

  http.end();
}

void IRAM_ATTR isr() {
  button_boot.numberKeyPresses += 1;
  button_boot.pressed = true;
}

void firmwareUpdate(String version) {
  vTaskDelete(Task1);
  lcd.clear();
  textEnd("UPDATE FIRMWERE", 2, 0);
  textEnd("VERSION " + version, 4, 1);
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);
  textEnd("SUCCESS", 6, 3);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure* client = new WiFiClientSecure;

  if (client) {
    client->setCACert(rootCACertificate);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
    HTTPClient https;

    if (https.begin(*client, fwurl)) {  // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK)  // if version received
      {
        payload = https.getString();  // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }

  if (httpCode == HTTP_CODE_OK)  // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return false;
    } else {
      Serial.println(payload);
      Serial.println("New firmware detected");
      firmwareUpdate(payload);
    }
  }
  return false;
}

void repeatedCall() {
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    FirmwareVersionCheck();

    if (count > 0) {
      sendToCloud();
    }
  }

  if ((currentMillis - previousMillis_2) >= mini_interval) {
    previousMillis_2 = currentMillis;
    Serial.print("Check fw version in:");
    Serial.print((interval - (currentMillis - previousMillis)) / 1000);
    Serial.println("sec.");
    Serial.print("Active fw version:");
    Serial.println(FirmwareVer);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Wi-Fi Strength: ");
      // Get RSSI value
      int rssi = WiFi.RSSI();
      if (rssi >= -50) {
        Serial.println("Excellent");
      } else if (rssi >= -70) {
        Serial.println("Good");
      } else {
        Serial.println("Weak");
      }
    }
  }
}

void autoUpdate(void* val) {
  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);

  wifiMulti.addAP(ssid1, password);
  wifiMulti.addAP(ssid2, password);
  wifiMulti.addAP(ssid3, password);
  wifiMulti.addAP(ssid4, password);
  wifiMulti.addAP(ssid5, password);

  Serial.println("Waiting for WiFi");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("SSID: " + WiFi.SSID());
  Serial.println("IP address:");
  Serial.println(WiFi.localIP());

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(1000);

  for (;;) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);

    if (button_boot.pressed) {  //to connect wifi via Android esp touch app
      Serial.println("Firmware update Starting..");
      firmwareUpdate("Reset");
      button_boot.pressed = false;
    }

    if (wifiMulti.run() == WL_CONNECTED) {
      repeatedCall();
    }
  }
}
