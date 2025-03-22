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

// Cấu hình LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Chân ảo Blynk
#define VIRTUAL_TEMP V0
#define VIRTUAL_HUMI V1
#define VIRTUAL_DUST V2
#define VIRTUAL_ALERT V4

// Biến kiểm soát cảnh báo
bool alertSent = false; 
bool alertEnabled = true; // Mặc định bật cảnh báo

// Xử lý nút bật/tắt cảnh báo từ Blynk
BLYNK_WRITE(VIRTUAL_ALERT) {
  alertEnabled = param.asInt();
  Serial.print("Chế độ cảnh báo: ");
  Serial.println(alertEnabled ? "Bật" : "Tắt");

  // Hiển thị trạng thái cảnh báo trên LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Canh bao:");
  lcd.setCursor(0, 1);
  lcd.print(alertEnabled ? "BAT " : "TAT ");
  delay(2000); 
  lcd.clear(); // Xóa màn hình để quay lại hiển thị dữ liệu môi trường
}

void setup() {
  Serial.begin(115200);

  // Kết nối WiFi nhanh hơn bằng cách tắt chế độ tự động quét
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Đang kết nối WiFi...");
  int timeout = 20;
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi đã kết nối!");
  } else {
    Serial.println("\nKết nối WiFi thất bại!");
  }

  // Kết nối Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  Serial.println("Kết nối Blynk thành công");

  // Khởi động cảm biến
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
}

void loop() {
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

  Blynk.virtualWrite(VIRTUAL_TEMP, temperature);
  Blynk.virtualWrite(VIRTUAL_HUMI, humidity);

  // Đọc dữ liệu từ cảm biến bụi GP2Y1010AU0F
  digitalWrite(LED_PIN, LOW);
  delayMicroseconds(280);      
  int analogValue = analogRead(ANALOG_PIN);
  delayMicroseconds(40);
  digitalWrite(LED_PIN, HIGH);
  delayMicroseconds(9680);

  // Chuyển đổi giá trị ADC thành nồng độ bụi (µg/m³)
  float voltage = analogValue * (3.3 / 4095.0);
  float dustDensity = (voltage - 0.1) / 0.005;
  if (dustDensity < 0) dustDensity = 0;

  Serial.print("Mật độ bụi: ");
  Serial.print(dustDensity, 1);
  Serial.println(" µg/m³");

  Blynk.virtualWrite(VIRTUAL_DUST, dustDensity);

  // Kiểm tra mức độ bụi
  if (dustDensity > 55) {
    Serial.println("CẢNH BÁO: Ô nhiễm không khí cao!");

    if (alertEnabled) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(1000);
      digitalWrite(BUZZER_PIN, LOW);

      if (!alertSent) {
        Blynk.logEvent("high_dust_warning", "Nồng độ bụi quá cao! Kiểm tra ngay!");
        alertSent = true;
        Serial.println("Đã gửi cảnh báo Blynk!");
      }
    }

    lcd.setCursor(0, 0);
    lcd.print("CANH BAO: BUI");
    lcd.setCursor(0, 1);
    lcd.print("D:");
    lcd.print(dustDensity, 1);
    lcd.print(" ug/m3 ");
    delay(2000);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    alertSent = false;

    // Hiển thị thông tin lên LCD
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
