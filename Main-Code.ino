//=====================================================================================//
//                             MONITORING SUHU PH TDS                                  //
//                        ===============================                              //
//  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx   //
//       Alat ini berfungsi untuk membaca nilai suhu atau temperature, nilai pH,       //
//              nilai TDS dan menyimpan data tersebut ke database MySQL.               //
//                   Untuk keperluan Skripsi S1 Teknologi Informasi                    //
//                       Universitas Insan Pembangunan Indonesia.                      //
//=====================================================================================//

//=====================================================================================//
//                                    NOTE!!!                                          //
// Editor : Arduino IDE 2.3.2                                                          //
// Board  : ESP32 by Espressif Systems 3.0.2                                           //
// URL    : https://espressif.github.io/arduino-esp32/package_esp32_dev_index.json     //
//=====================================================================================//
//=====================================================================================//
//                                   LIBRARY                                           //
// -> Pre Installed                                                                    //
//   - WiFi at version 2.0.0                                                           //
//   - Networking at version 1.0.0                                                     //
//   - Ticker at version 2.0.0                                                         //
//   - HTTPClient at version 2.0.0                                                     //
//   - NetworkClientSecure at version 2.0.0                                            //
//   - Wire at version 2.0.0                                                           //
//                                                                                     //
// -> External                                                                         //
//   - ArduinoJson at version 6.21.3 by Benoit Blanchon                                //
//   - LiquidCrystal I2C at version 1.1.2 by Frank de Brabander                        //
//   - DallasTemperature at version 3.9.0 by Milles Burton                             //
//   - OneWire at version 2.3.8 by Jim Studt                                           //
//                                                                                     //
//=====================================================================================//

//=====================================================================================//
//                            Konfigurasi PIN / GPIO                                   //
// -> Sensor Suhu DS18B20                                                              //
//   - DATA => ESP32-GPIO27                                                            //
//   - VCC  => 3V3                                                                     //
//   - GND  => GND                                                                     //
//                                                                                     //
// -> Sensor TDS                                                                       //
//   - SIGNAL => ESP32-GPIO33                                                          //
//   - VCC    => 3V3 / 5V                                                              //
//   - GND    => GND                                                                   //
//                                                                                     //
// -> Relay TDS Controll                                                               //
//   - ESP32-GPIO26                                                                    //
//                                                                                     //
// -> Sensor pH                                                                        //
//   - AO   => ESP32-GPIO32                                                            //
//   - TO   => ESP32-GPIO35                                                            //
//   - VCC  => 3V3 / 5V                                                                //
//   - GND  => GND                                                                     //
//                                                                                     //
// -> Relay PH Controll                                                                //
//   - ESP32-GPIO25                                                                    //
//                                                                                     //
// -> LED Status                                                                       //
//   - ESP32-GPIO23                                                                    //
//                                                                                     //
// -> LED WiFi                                                                         //
//   - ESP32-GPIO13                                                                    //
//                                                                                     //
// -> LCD 16X2 I2C                                                                     //
//   - SDA => ESP32-GPIO21                                                             //
//   - SCL => ESP32-GPIO22                                                             //
//   - VCC  => 5V                                                                      //
//   - GND  => GND                                                                     //
//                                                                                     //
//=====================================================================================//

// Include Library ======================================================================
#include <WiFi.h>
#include <Ticker.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include <UrlEncode.h>
//=======================================================================================

// Connection Config ====================================================================
const char *WIFI_SSID = "Google Pixel 4XL";
const char *WIFI_PASSWORD = "ahadigoogle";
const char *HOST = "192.168.125.226";  // Ipv4 server / localhost
//=======================================================================================

// #define sendWA // enable/disable send to whatsapp

//=======================================================================================
String phoneNumber = "6283891495814";
String apiKey = "8692310";
//=======================================================================================

//=======================================================================================
int BATAS_SUHU = 30;  // set batas suhu tinggi (Celcius)
//=======================================================================================

// PH Sensor Config =====================================================================
const int PH_SENSOR_PIN = 32;
const int RELAY_PH_PIN = 25;
float phValue, phStep;
int analogValue;
double phVolt;
float ph4 = 3.266;  // get value from calibration
float ph7 = 2.691;  // get value from calibartion
//=======================================================================================

// TDS Sensor Config ====================================================================
const int RELAY_TDS_PIN = 26;
const int TDS_SENSOR_PIN = 33;
#define SCOUNT 30
int analogBufferPh[SCOUNT];
int analogBufferTempPh[SCOUNT];
int analogBufferTds[SCOUNT];
int analogBufferTempTds[SCOUNT];
int analogBufferIndexPh = 0, analogBufferIndexTds = 0;
float averageVoltagePh = 0, averageVoltageTds = 0, tdsValue = 0;
//=======================================================================================

// Temperature Sensor Config ============================================================
const int SENSOR_TEMP_PIN = 27;
float tempValue = 0;
OneWire oneWire(SENSOR_TEMP_PIN);
DallasTemperature sensor(&oneWire);
//=======================================================================================

// LCD Config ===========================================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);
//=======================================================================================

// Other Config =========================================================================
#define VREF 3.3
#define ADC_RANGE 4096.0
const int LED_STATUS_PIN = 23;
const int LED_WIFI_PIN = 13;
unsigned long interval = 5000;  // interval for sending data to database (ms)
unsigned long prevTime;
Ticker tick;
//=======================================================================================

//=======================================================================================
void sendMessage(String pesan) {
  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(pesan);
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200) {
    Serial.print("Message sent successfully");
  } else {
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}
//=======================================================================================

// Read ADC PH Function =================================================================
void readADCPh() {
  analogBufferPh[analogBufferIndexPh] = analogRead(PH_SENSOR_PIN);
  analogBufferIndexPh++;
  if (analogBufferIndexPh == SCOUNT) {
    analogBufferIndexPh = 0;
  }
}
//=======================================================================================

// Read ADC TDS Function ================================================================
void readADCTds() {
  analogBufferTds[analogBufferIndexTds] = analogRead(TDS_SENSOR_PIN);
  analogBufferIndexTds++;
  if (analogBufferIndexTds == SCOUNT) {
    analogBufferIndexTds = 0;
  }
}
//=======================================================================================

// Read Sensor Function =================================================================
void readSensor() {
  // Read PH
  digitalWrite(RELAY_PH_PIN, HIGH);
  tick.attach_ms(40, readADCPh);
  delay(500);
  for (int i = 0; i < SCOUNT; i++) {
    analogBufferTempPh[i] = analogBufferPh[i];
  }
  averageVoltagePh = getMedianNum(analogBufferTempPh, SCOUNT) * (float)VREF / ADC_RANGE;
  phStep = (ph4 - ph7) / 3;
  phValue = 6.00 + ((ph7 - averageVoltagePh) / phStep);
  lcd.setCursor(0, 1);
  lcd.print(String(phValue, 1));
  lcd.print(F(" "));
  Serial.print(F("PH Value   = "));
  Serial.print(phValue);
  Serial.print(F("\n"));
  delay(1000);
  tick.detach();
  digitalWrite(RELAY_PH_PIN, LOW);
  delay(500);

  // Read TDS
  digitalWrite(RELAY_TDS_PIN, HIGH);
  tick.attach_ms(40, readADCTds);
  delay(500);
  for (int i = 0; i < SCOUNT; i++) {
    analogBufferTempTds[i] = analogBufferTds[i];
  }
  averageVoltageTds = getMedianNum(analogBufferTempTds, SCOUNT) * (float)VREF / ADC_RANGE;
  float compensationCoefficient = 1.0 + 0.02 * (tempValue - 25.0);
  float compensationVolatge = averageVoltageTds / compensationCoefficient;
  tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5;
  lcd.setCursor(5, 1);
  lcd.print(String(tdsValue, 1));
  lcd.print(F(" "));
  Serial.print(F("TDS Value  = "));
  Serial.print(tdsValue);
  Serial.print(F("\n"));
  delay(1000);
  tick.detach();
  digitalWrite(RELAY_TDS_PIN, LOW);
  delay(500);

  // Read Temperature
  sensor.requestTemperatures();
  float temp = sensor.getTempCByIndex(0);
  if (temp != DEVICE_DISCONNECTED_C) {
    tempValue = temp;
  } else {
    tempValue = 0;
  }
  lcd.setCursor(11, 1);
  lcd.print(String(tempValue, 1));
  lcd.print(F(" "));
  Serial.print(F("Suhu Value = "));
  Serial.print(tempValue);
  Serial.print(F("\n"));
}
//=======================================================================================

// Sending Data Function ================================================================
void sendingData() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_WIFI_PIN, HIGH);
    HTTPClient http;
    NetworkClient client;

    if (!client.connect(HOST, 80)) {
      Serial.print(F("Not connect to server\n"));
      blink(3);
      return;
    }

    String url, data;
    url = "http://" + String(HOST) + "/monitoring/api/create.php";

    StaticJsonDocument<200> doc;
    doc["ph"] = String(phValue, 1);
    doc["tds"] = String(tdsValue, 1);
    doc["suhu"] = String(tempValue, 1);

    // http.begin(client, url);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    serializeJson(doc, data);
    Serial.print(F("POST Data => "));
    Serial.print(data);
    Serial.print(F("\n"));
    Serial.print(F("Endpoint => "));
    Serial.print(url);
    Serial.print(F("\n"));

    int httpCode = http.POST(data);
    String payload;
    if (httpCode == 200) {
      payload = http.getString();
      payload.trim();
      if (payload.length() > 0) {
        Serial.print(payload + "\n");
      }
      blink(1);
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      Serial.print(F("\n"));
      blink(2);
    }
    http.end();
  } else {
    Serial.print(F("Not connected to WiFi"));
    Serial.print(F("\n"));
    digitalWrite(LED_WIFI_PIN, LOW);
  }

  // Cek Nilai suhu
  if (tempValue > BATAS_SUHU) {
    Serial.println(F("Suhu tinggi"));
    String pesan = "PERHATIAN..!!!\n";
    pesan += "Suhu terlalu tinggi";
#ifdef sendWA
    sendMessage(pesan);
#endif
  } else {
    Serial.println(F("Suhu aman"));
  }

  // Cek Nilai pH
  if (phValue > 10) {
    Serial.println(F("Level pH tinggi"));
    String pesan = "PERHATIAN..!!!\n";
    pesan += "Level pH terlalu tinggi";
#ifdef sendWA
    sendMessage(pesan);
#endif
  } else if (phValue < 6) {
    Serial.println(F("Level pH rendah"));
    String pesan = "PERHATIAN..!!!\n";
    pesan += "Level pH terlalu rendah";
#ifdef sendWA
    sendMessage(pesan);
#endif
  } else {
    Serial.println(F("Level pH aman"));
  }

  // Cek Nilai TDS
  if (tdsValue > 250) {
    Serial.println(F("Level TDS tinggi"));
    String pesan = "PERHATIAN..!!!\n";
    pesan += "Level TDS terlalu tinggi";
#ifdef sendWA
    sendMessage(pesan);
#endif
  } else if (tdsValue < 20) {
    Serial.println(F("Level TDS rendah"));
    String pesan = "PERHATIAN..!!!\n";
    pesan += "Level TDS terlalu rendah";
#ifdef sendWA
    sendMessage(pesan);
#endif
  } else {
    Serial.println(F("Level TDS aman"));
  }
}
//=======================================================================================

// LCD Print Welcome Function ===========================================================
void lcdPrintWelcome() {
  lcd.setCursor(0, 0);
  lcd.print(F("     SISTEM     "));
  delay(500);
  lcd.setCursor(0, 1);
  lcd.print(F("   MONITORING   "));
  delay(1500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("  DAN  LOGGING  "));
  delay(500);
  for (int i = 5; i < 11; i++) {
    lcd.setCursor(i, 1);
    lcd.print(F("-"));
    delay(300);
  }
  delay(1500);
}
//=======================================================================================

// setup Function =======================================================================
void setup() {
  //==========================================================================
  Serial.begin(115200);
  delay(1000);
  //==========================================================================
  lcd.init();
  lcd.clear();
  lcd.backlight();
  //==========================================================================
  lcdPrintWelcome();
  //==========================================================================
  Serial.print(F("\n================== System Setup ==================\n"));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("  SYSTEM SETUP  "));
  delay(1000);
  for (int i = 5; i < 11; i++) {
    lcd.setCursor(i, 1);
    lcd.print(F("-"));
    delay(500);
  }
  delay(300);
  //==========================================================================
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(LED_WIFI_PIN, OUTPUT);
  //==========================================================================
  // PH Sensor Setup
  //==========================================================================
  pinMode(PH_SENSOR_PIN, INPUT);
  pinMode(RELAY_PH_PIN, OUTPUT);
  digitalWrite(RELAY_PH_PIN, LOW);
  //==========================================================================
  // TDS Sensor Setup
  //==========================================================================
  pinMode(TDS_SENSOR_PIN, INPUT);
  pinMode(RELAY_TDS_PIN, OUTPUT);
  digitalWrite(RELAY_TDS_PIN, LOW);
  //==========================================================================
  lcd.setCursor(0, 1);
  lcd.print(F("      DONE      "));
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F(" CONNECTING  TO "));
  lcd.setCursor(0, 1);
  lcd.print(F("      WIFI      "));
  tick.attach(1, blinker);
  //==========================================================================
  Serial.print(F("\n================ Connecting  WiFi ================\n"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(500);
  }
  Serial.print(F("\nWiFi Connected, IP : "));
  Serial.print(WiFi.localIP());
  Serial.print(F("\n"));
  WiFi.setAutoReconnect(true);
  //==========================================================================
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("   CONNECTION   "));
  lcd.setCursor(0, 1);
  lcd.print(F("    SUCCESS     "));
  tick.detach();
  delay(1000);
  //==========================================================================
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F(" PH   TDS  SUHU "));
  lcd.setCursor(0, 1);
  lcd.print(F("0.0   0.0   0.0 "));
  //==========================================================================
  prevTime = millis();
  //==========================================================================
}
//=======================================================================================

// loop Function ========================================================================
void loop() {
  //==========================================================================
  if (millis() - prevTime >= interval) {
    Serial.print(F("\n================= Reading Sensor =================\n"));
    readSensor();
    Serial.print(F("\n================== Sending Data ==================\n"));
    sendingData();
    prevTime = millis();
  }
  //==========================================================================
}
//=======================================================================================

//=======================================================================================
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    bTemp = bTab[(iFilterLen - 1) / 2];
  } else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}
//=======================================================================================

//=======================================================================================
void blink(uint8_t n) {
  while (n > 0) {
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(150);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(150);
    n--;
  }
}
//=======================================================================================

//=======================================================================================
void blinker() {
  int state = digitalRead(LED_WIFI_PIN);
  digitalWrite(LED_WIFI_PIN, !state);
}
//=======================================================================================