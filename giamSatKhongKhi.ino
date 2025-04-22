#define BLYNK_TEMPLATE_ID "TMPL6Qg6_8eTZ"
#define BLYNK_TEMPLATE_NAME "Giám sát chất lượng không khí"
#define BLYNK_AUTH_TOKEN "Lervbwy0A5z_NRMNSUBSEvANjXurSiSU"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Thông tin WiFi
#define WIFI_SSID "lab"
#define WIFI_PASS "12345678"

// Cấu hình cảm biến
#define DHT_PIN 26
#define DHT_TYPE DHT11
#define LED_PIN 5
#define ANALOG_PIN 34
#define BUZZER_PIN 25

// Chân ảo Blynk
#define VIRTUAL_TEMP V0
#define VIRTUAL_HUMI V1
#define VIRTUAL_DUST V2
#define VIRTUAL_ALERT V4

// Khởi tạo thiết bị
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Biến kiểm soát
bool alertSent = false;  //Biến trạng thái gửi cảnh báo
bool alertEnabled = true; //Biến trạng thái bật/tắt cảnh báo
unsigned long previousMillis = 0;
const long interval = 2000;

void connectWiFiBlynk() {
  // Kết nối Wifi
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
  
  // Kết nối Blynk
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

void displayLCD(float temp, float humi, float dust, bool warning = false) {
  lcd.clear();
  if (warning) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CANH BAO: BUI");
    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(dust, 1);
    lcd.print(" ug/m3 ");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(humi, 1);
    lcd.print("%");
    
    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(dust, 1);
    lcd.print(" ug/m3 ");
  }
}

void checkAlert(float dustDensity) {
  static unsigned long lastAlertTime = 0;
  const unsigned long alertInterval = 30000; // 30 giây
  
  if (dustDensity > 55 && alertEnabled) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
    
    if (!alertSent || millis() - lastAlertTime > alertInterval) {
      Blynk.logEvent("high_dust_warning", "Nồng độ bụi quá cao! Kiểm tra ngay!");
      alertSent = true;
      lastAlertTime = millis();
      Serial.println("Đã gửi cảnh báo Blynk!");
    }
  } else {
    alertSent = false;
  }
}

BLYNK_WRITE(VIRTUAL_ALERT) {
  alertEnabled = param.asInt();
  Serial.print("Chế độ cảnh báo: ");
  Serial.println(alertEnabled ? "Bật" : "Tắt");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Canh bao:");
  lcd.setCursor(0, 1);
  lcd.print(alertEnabled ? "BAT " : "TAT ");
  delay(2000);
  lcd.clear();
}

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo cảm biến
  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Khởi động LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Khoi dong...");
  delay(2000);
  lcd.clear();
  
  // Kết nối mạng và Blynk
  connectWiFiBlynk();
}

void loop() {
  Blynk.run();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Đọc cảm biến
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    float dustDensity = readDustDensity();
    
    if (!isnan(humidity) && !isnan(temperature)) {
      // Gửi dữ liệu lên Blynk
      Blynk.virtualWrite(VIRTUAL_TEMP, temperature);
      Blynk.virtualWrite(VIRTUAL_HUMI, humidity);
      Blynk.virtualWrite(VIRTUAL_DUST, dustDensity);
      
      // Kiểm tra cảnh báo
      checkAlert(dustDensity);
      
      // Hiển thị LCD
      displayLCD(temperature, humidity, dustDensity, dustDensity > 55);
      
      // In ra Serial Monitor
      Serial.print("Nhiệt độ: ");
      Serial.print(temperature, 1);
      Serial.print("°C | Độ ẩm: ");
      Serial.print(humidity, 1);
      Serial.print("% | Bụi: ");
      Serial.print(dustDensity, 1);
      Serial.println(" µg/m³");
    } else {
      Serial.println("Lỗi đọc DHT11!");
      lcd.clear();
      lcd.print("Loi cam bien!");
    }
  }
}