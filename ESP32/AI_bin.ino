#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "time.h"
#include <ESP32Servo.h>

const char* WIFI_SSID = "Minh Trong";
const char* WIFI_PASS = "0123456789";
const char* SERVER_URL = "http://172.20.10.7:5000/predict";

const int TRIG_PIN = 18;
const int ECHO_KPH = 12;
const int ECHO_PH  = 14;

const int LED1 = 32;
const int LED2 = 33;

const int TRIG_NEW = 18; 
const int ECHO_NEW = 13;

Servo myservo;
int servoAngle = 180;

LiquidCrystal_I2C lcd(0x27, 16, 2);

const float EMPTY_KPH = 8.7;
const float FULL_KPH  = 2.9;
const float EMPTY_PH  = 8.57;
const float FULL_PH   = 1.7;

volatile unsigned long startEchoKPH = 0, durationKPH = 0;
volatile bool kphReady = false;
volatile unsigned long startEchoPH = 0, durationPH = 0;
volatile bool phReady = false;

float percentKPH = 0.0, percentPH = 0.0;
float prevPercentKPH = -1.0, prevPercentPH = -1.0;
unsigned long prevTimeKPH = 0, prevTimePH = 0;
float kph_rate_ewma = 0.0, ph_rate_ewma = 0.0;
volatile float currentDistKPH = 0.0; 
volatile float currentDistPH = 0.0;  
const float ALPHA = 0.05;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

unsigned long lastRateCalcKPH = 0; 
unsigned long lastRateCalcPH = 0;
const unsigned long RATE_INTERVAL = 10000; 

String phFullAt = "--:--";
String kphFullAt = "--:--";
String phDay = "";
String kphDay = "";
bool hasPrediction = false;

void IRAM_ATTR echoKPH_ISR() {
  if (digitalRead(ECHO_KPH) == HIGH) startEchoKPH = micros();
  else { durationKPH = micros() - startEchoKPH; kphReady = true; }
}
void IRAM_ATTR echoPH_ISR() {
  if (digitalRead(ECHO_PH) == HIGH) startEchoPH = micros();
  else { durationPH = micros() - startEchoPH; phReady = true; }
}

float durationToCm(unsigned long dur_us) {
  if (dur_us == 0) return 999.0;
  return (dur_us * 0.034) / 2.0;
}
float calcPercent(float dist, float empty, float full) {
  if (dist <= full) return 100.0;
  if (dist >= empty) return 0.0;
  return 100.0 * (empty - dist) / (empty - full);
}

float readNewSensor() {
  digitalWrite(TRIG_NEW, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_NEW, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_NEW, LOW);

  long duration = pulseIn(ECHO_NEW, HIGH, 30000);
  if (duration == 0) return 999;

  return duration * 0.034 / 2.0;
}

void TaskMeasureKPH(void* pvParams) {
  static float smoothDistKPH = 0.0; 

  for (;;) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(4);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    unsigned long start = millis();
    while (!kphReady && millis() - start < 50) taskYIELD();
    
    if (kphReady) {
      noInterrupts();
      unsigned long d = durationKPH;
      kphReady = false;
      interrupts();
      float rawDist = durationToCm(d);

      if (smoothDistKPH == 0.0 || smoothDistKPH > 500) {
          smoothDistKPH = rawDist;
      } else {
          smoothDistKPH = 0.9 * smoothDistKPH + 0.1 * rawDist;
      }
      currentDistKPH = smoothDistKPH;

      float p = calcPercent(smoothDistKPH, EMPTY_KPH, FULL_KPH);
      percentKPH = p; 

      unsigned long now = millis();
      if (now - lastRateCalcKPH >= RATE_INTERVAL) {
        if (prevPercentKPH >= 0) {
          float delta = p - prevPercentKPH;
          float hours = (now - lastRateCalcKPH) / 3600000.0;
          
          if (hours > 0) {
            float rate = delta / hours;
            kph_rate_ewma = (kph_rate_ewma == 0.0) ? rate : (ALPHA * rate + (1 - ALPHA) * kph_rate_ewma);
          }
        }
        prevPercentKPH = p;
        lastRateCalcKPH = now;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(400));
  }
}

void TaskMeasurePH(void* pvParams) {
  static float smoothDistPH = 0.0;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(150)); 
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(4);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long start = millis();
    while (!phReady && millis() - start < 50) taskYIELD();

    if (phReady) {
      noInterrupts();
      unsigned long d = durationPH;
      phReady = false;
      interrupts();
      float rawDist = durationToCm(d);

      if (smoothDistPH == 0.0 || smoothDistPH > 500) {
          smoothDistPH = rawDist;
      } else {
          smoothDistPH = 0.9 * smoothDistPH + 0.1 * rawDist;
      }

      currentDistPH = smoothDistPH;
      float p = calcPercent(smoothDistPH, EMPTY_PH, FULL_PH);
      percentPH = p; 

      unsigned long now = millis();
      if (now - lastRateCalcPH >= RATE_INTERVAL) {
        if (prevPercentPH >= 0) {
          float delta = p - prevPercentPH;
          float hours = (now - lastRateCalcPH) / 3600000.0;
          
          if (hours > 0) {
            float rate = delta / hours;
            ph_rate_ewma = (ph_rate_ewma == 0.0) ? rate : (ALPHA * rate + (1 - ALPHA) * ph_rate_ewma);
          }
        }
        prevPercentPH = p;
        lastRateCalcPH = now;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(400));
  }
}

void TaskSendPredict(void *pvParameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      
      struct tm timeinfo;
      getLocalTime(&timeinfo);

      HTTPClient http;
      http.begin(SERVER_URL);
      http.addHeader("Content-Type", "application/json");

      String jsonData = "{\"PH_percent\":" + String(percentPH, 2) +
                        ",\"KPH_percent\":" + String(percentKPH, 2) +
                        ",\"hour\":" + String(timeinfo.tm_hour) +
                        ",\"day\":" + String(timeinfo.tm_wday == 0 ? 6 : timeinfo.tm_wday - 1) +
                        ",\"PH_change\":" + String(ph_rate_ewma, 4) +
                        ",\"KPH_change\":" + String(kph_rate_ewma, 4) + "}";

      Serial.println("Sending: " + jsonData);
      int httpCode = http.POST(jsonData);

      if (httpCode > 0) {
        String payload = http.getString();
        Serial.println("Received: " + payload);

        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
          phFullAt  = doc["PH_Full_At"]  | "--:--";
          phDay     = doc["PH_Day"]     | "";
          kphFullAt = doc["KPH_Full_At"] | "--:--";
          kphDay    = doc["KPH_Day"]    | "";
          hasPrediction = true;
        }
      }
      http.end();
    }
    vTaskDelay(pdMS_TO_TICKS(10000)); 
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_KPH, INPUT);
  pinMode(ECHO_PH, INPUT);
  attachInterrupt(digitalPinToInterrupt(ECHO_KPH), echoKPH_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ECHO_PH), echoPH_ISR, CHANGE);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);

  pinMode(TRIG_NEW, OUTPUT);
  pinMode(ECHO_NEW, INPUT);

  myservo.attach(19);
  myservo.write(180);

  Wire.begin(22, 23);
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("Connecting WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  lcd.clear(); lcd.print("WiFi Connected");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  xTaskCreatePinnedToCore(TaskMeasureKPH, "KPH", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskMeasurePH,  "PH",  4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskSendPredict, "SEND", 4096, NULL, 1, NULL, 0);
}

void loop() {
  float d = readNewSensor();
  Serial.print("NEW: ");
  Serial.print(d);
  Serial.print(" cm | KPH: ");
  Serial.print(currentDistKPH);
  Serial.print(" cm (");
  Serial.print(percentKPH);
  Serial.print("%) | PH: ");
  Serial.print(currentDistPH);
  Serial.print(" cm (");
  Serial.print(percentPH);
  Serial.println("%)");

  if (d < 5.0) {
    if (servoAngle != 900) {
      myservo.write(90);
      servoAngle = 90;
      Serial.println("Servo quay 90 do");
    }
  } else {
    if (servoAngle != 180) {
      myservo.write(180);
      servoAngle = 180;
      Serial.println("Servo ve 180 do");
    }
  }
  char lineBuffer[17];

  lcd.setCursor(0, 0);
  if (hasPrediction) {
    snprintf(lineBuffer, sizeof(lineBuffer), "KPH: %s %s",
             kphFullAt.c_str(), kphDay.c_str());
    lcd.print(lineBuffer);
    for (int i=strlen(lineBuffer); i<16; i++) lcd.print(" ");
  } else {
    lcd.print("Du doan day rac ");
  }

  lcd.setCursor(0, 1);
  if (hasPrediction) {
    snprintf(lineBuffer, sizeof(lineBuffer), "PH : %s %s",
             phFullAt.c_str(), phDay.c_str());
    lcd.print(lineBuffer);
    for (int i=strlen(lineBuffer); i<16; i++) lcd.print(" ");
  } else {
    lcd.print(" Dang du doan... ");
  }

  delay(1000);
}