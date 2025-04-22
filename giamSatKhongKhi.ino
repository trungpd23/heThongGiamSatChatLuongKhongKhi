#define BLYNK_TEMPLATE_ID "TMPL6Qg6_8eTZ"
#define BLYNK_TEMPLATE_NAME "Giám sát chất lượng không khí"
#define BLYNK_AUTH_TOKEN "Lervbwy0A5z_NRMNSUBSEvANjXurSiSU"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi
#define WIFI_SSID "lab"
#define WIFI_PASS "12345678"

// Cảm biến & thiết bị
#define DHT_PIN 26
#define DHT_TYPE DHT11
#define LED_PIN 5
#define ANALOG_PIN 34
#define BUZZER_PIN 25

// Ngưỡng cảnh báo
#define DUST_THRESHOLD 55
#define TEMP_THRESHOLD 38
#define HUMI_THRESHOLD 85

// Blynk chân ảo
#define VIRTUAL_TEMP V0
#define VIRTUAL_HUMI V1
#define VIRTUAL_DUST V2
#define VIRTUAL_ALERT V3

// Khởi tạo đối tượng
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool alertSentDust = false;
bool alertSentTemp = false;
bool alertSentHumi = false;
bool alertEnabled = true;
unsigned long previousMillis = 0;
const long interval = 2000;

void connectWiFiBlynk() {
  Serial.print("Đang kết nối WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nKết nối WiFi thất bại!");
    lcd.clear();
    lcd.print("WiFi FAIL!");
    delay(1000);
    return;
  }
  Serial.println("\nWiFi đã kết nối!");
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  if (!Blynk.connect()) {
    Serial.println("Kết nối Blynk thất bại!");
    lcd.clear();
    lcd.print("Blynk FAIL!");
    delay(1000);
  } else {
    Serial.println("Kết nối Blynk thành công");
  }
}

float readDustDensity() {
  digitalWrite(LED_PIN, LOW);
  delayMicroseconds(280);
  int analogValue = analogRead(ANALOG_PIN);
  delayMicroseconds(40);
  digitalWrite(LED_PIN, HIGH);
  delayMicroseconds(9680);

  float voltage = analogValue * (3.3 / 4095.0);
  float dustDensity = (voltage - 0.1) / 0.005;
  return max(dustDensity, (float)0);
}

void displayLCD(float temp, float humi, float dust) {
  bool showDust = dust > DUST_THRESHOLD;
  bool showTemp = temp > TEMP_THRESHOLD;
  bool showHumi = humi > HUMI_THRESHOLD;

  if (showDust || showTemp || showHumi) {
    if (showDust) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CANH BAO: BUI!");
      lcd.setCursor(0, 1);
      lcd.print("D:");
      lcd.print(dust, 1);
      lcd.print(" ug/m3");
      delay(2000);
    }
    if (showTemp) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CANH BAO: NHIET!");
      lcd.setCursor(0, 1);
      lcd.print("T:");
      lcd.print(temp, 1);
      lcd.print(" C");
      delay(2000);
    }
    if (showHumi) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CANH BAO: DO AM");
      lcd.setCursor(0, 1);
      lcd.print("H:");
      lcd.print(humi, 1);
      lcd.print(" %");
      delay(2000);
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(humi, 1);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(dust, 1);
    lcd.print(" ug/m3");
  }
}

void checkAlerts(float temp, float humi, float dust) {
  static unsigned long lastAlertTime = 0;
  const unsigned long alertInterval = 30000;

  if (dust > DUST_THRESHOLD && alertEnabled && (!alertSentDust || millis() - lastAlertTime > alertInterval)) {
    Blynk.logEvent("high_dust_warning", "Nồng độ bụi quá cao!");
    tone(BUZZER_PIN, 1000, 500);
    alertSentDust = true;
    lastAlertTime = millis();
    Serial.println("Cảnh báo bụi!");
  } else if (dust <= DUST_THRESHOLD) {
    alertSentDust = false;
  }

  if (temp > TEMP_THRESHOLD && alertEnabled && (!alertSentTemp || millis() - lastAlertTime > alertInterval)) {
    Blynk.logEvent("high_temp_warning", "Nhiệt độ quá cao!");
    tone(BUZZER_PIN, 1200, 500);
    alertSentTemp = true;
    lastAlertTime = millis();
    Serial.println("Cảnh báo nhiệt độ!");
  } else if (temp <= TEMP_THRESHOLD) {
    alertSentTemp = false;
  }

  if (humi > HUMI_THRESHOLD && alertEnabled && (!alertSentHumi || millis() - lastAlertTime > alertInterval)) {
    Blynk.logEvent("high_humi_warning", "Độ ẩm quá cao!");
    tone(BUZZER_PIN, 1400, 500);
    alertSentHumi = true;
    lastAlertTime = millis();
    Serial.println("Cảnh báo độ ẩm!");
  } else if (humi <= HUMI_THRESHOLD) {
    alertSentHumi = false;
  }
}

BLYNK_WRITE(VIRTUAL_ALERT) {
  alertEnabled = param.asInt();
  Serial.print("Cảnh báo: ");
  Serial.println(alertEnabled ? "BẬT" : "TẮT");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Canh bao:");
  lcd.setCursor(0, 1);
  lcd.print(alertEnabled ? "BAT" : "TAT");
  delay(2000);
  lcd.clear();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Khoi dong...");
  delay(2000);
  lcd.clear();
  connectWiFiBlynk();
}

void loop() {
  Blynk.run();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float humi = dht.readHumidity();
    float temp = dht.readTemperature();
    float dust = readDustDensity();

    if (!isnan(humi) && !isnan(temp)) {
      Blynk.virtualWrite(VIRTUAL_TEMP, temp);
      Blynk.virtualWrite(VIRTUAL_HUMI, humi);
      Blynk.virtualWrite(VIRTUAL_DUST, dust);

      checkAlerts(temp, humi, dust);
      displayLCD(temp, humi, dust);

      Serial.printf("Nhiet do: %.1f°C | Do am: %.1f%% | Bui: %.1f ug/m3\n", temp, humi, dust);
    } else {
      lcd.clear();
      lcd.print("Loi cam bien!");
      Serial.println("Lỗi đọc DHT11!");
    }
  }
}
