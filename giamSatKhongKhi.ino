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

// Cấu hình DHT11
#define DHT_PIN 26
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Cấu hình GP2Y1010AU0F
#define LED_PIN 5
#define ANALOG_PIN 34

// Cấu hình còi
#define BUZZER_PIN 25

// Cấu hình LCD (Thư viện LiquidCrystal_I2C cho ESP32)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Chân ảo Blynk
#define VIRTUAL_TEMP V0
#define VIRTUAL_HUMI V1
#define VIRTUAL_DUST V2

// Biến kiểm soát cảnh báo
bool alertSent = false; 

void setup() {
  Serial.begin(115200);

  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Đang kết nối WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối!");

  // Kết nối Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  Serial.println("Kết nối Blynk thành công");

  // Khởi động cảm biến
  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ban đầu tắt còi

  // Khởi động LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Khoi dong...");
}

void loop() {
  // Chạy Blynk
  Blynk.run();

  // Đọc dữ liệu từ DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Lỗi đọc DHT11!");
    lcd.setCursor(0, 0);
    lcd.print("Loi cam bien!");
    delay(2000);
    return;
  } 

  Serial.print("Nhiệt độ: ");
  Serial.print(temperature, 1);
  Serial.print("°C | Độ ẩm: ");
  Serial.print(humidity, 1);
  Serial.println("%");

  // Gửi dữ liệu lên Blynk
  Blynk.virtualWrite(VIRTUAL_TEMP, temperature);
  Blynk.virtualWrite(VIRTUAL_HUMI, humidity);

  // Đọc dữ liệu từ cảm biến bụi GP2Y1010AU0F
  digitalWrite(LED_PIN, LOW);
  delayMicroseconds(280);      
  int analogValue = analogRead(ANALOG_PIN);
  delayMicroseconds(40);
  digitalWrite(LED_PIN, HIGH);
  delayMicroseconds(9680);

  // Chuyển đổi giá trị ADC thành điện áp
  float voltage = analogValue * (3.3 / 4095.0);
  float dustDensity = (voltage - 0.1) / 0.005;

  // Ngăn giá trị bị âm
  if (dustDensity < 0) dustDensity = 0;

  Serial.print("Mật độ bụi: ");
  Serial.print(dustDensity, 1);
  Serial.println(" µg/m³");

  // Gửi dữ liệu lên Blynk
  Blynk.virtualWrite(VIRTUAL_DUST, dustDensity);

  // Kiểm tra và cảnh báo nếu bụi > 55 µg/m³
  if (dustDensity > 55) {
    Serial.println("CẢNH BÁO: Ô nhiễm không khí cao!");
    digitalWrite(BUZZER_PIN, HIGH); // Bật còi báo động
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
    // Gửi thông báo Blynk
    if (!alertSent) {
      Blynk.logEvent("high_dust_warning", "Nồng độ bụi quá cao! Kiểm tra ngay!");
      alertSent = true;
      Serial.println("Đã gửi cảnh báo Blynk!");
    }

    lcd.setCursor(0, 0);
    lcd.print("CANH BAO: BU!I");
    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(dustDensity, 1);
    lcd.print(" ug/m3");
    delay(2000);
  } else {
    digitalWrite(BUZZER_PIN, LOW); // Tắt còi
    alertSent = false;  // Reset lại để có thể gửi cảnh báo tiếp nếu bụi tăng cao lại

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature, 1);
    lcd.print("C H:");
    lcd.print(humidity, 1);
    lcd.print("%  ");

    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(dustDensity, 1);
    lcd.print(" ug/m3 ");
  }

  delay(2000);
}
