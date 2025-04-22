// Định nghĩa Blynk Template
#define BLYNK_TEMPLATE_ID "TMPL6Qg6_8eTZ"
#define BLYNK_TEMPLATE_NAME "Giám sát chất lượng không khí"
#define BLYNK_AUTH_TOKEN "Lervbwy0A5z_NRMNSUBSEvANjXurSiSU"

// Thư viện
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp32.h>
#include <EEPROM.h>

// Chân kết nối
#define DHT_PIN             26
#define DHT_TYPE            DHT11
#define DUST_LED_PIN        5
#define DUST_MEASURE_PIN    34
#define BUZZER_PIN          25

// WiFi cấu hình mặc định
const char* ssid = "lab";
const char* password = "12345678";

// Virtual Pin
#define VIRTUAL_TEMP V0
#define VIRTUAL_HUMI V1
#define VIRTUAL_DUST V2
#define VIRTUAL_ALERT V4

// Khởi tạo đối tượng
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Biến đo lường
float temperature = 0;
float humidity = 0;
float dustDensity = 0;

// Ngưỡng cảnh báo
float tempThreshold = 40.0;
float humiThreshold = 85.0;
float dustThreshold = 60.0;

// Trạng thái
bool connectedToWifi = false;
bool connectedToBlynk = false;
bool alertSent = false;
bool alertEnabled = true;

// Kết nối WiFi
void connectToWifi() {
  Serial.println("Đang kết nối WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 20) {
    delay(500);
    Serial.print(".");
  }

  connectedToWifi = WiFi.status() == WL_CONNECTED;
  Serial.println(connectedToWifi ? "\nWiFi connected" : "\nWiFi FAILED");
}

// Kết nối Blynk
void connectToBlynk() {
  if (!connectedToWifi) return;

  Serial.println("Đang kết nối Blynk...");
  Blynk.config(BLYNK_AUTH_TOKEN);

  int attempts = 0;
  while (!Blynk.connected() && attempts++ < 10) {
    Blynk.connect();
    delay(1000);
  }

  connectedToBlynk = Blynk.connected();
  Serial.println(connectedToBlynk ? "Blynk connected" : "Blynk FAILED");
}

// Đọc bụi mịn từ GP2Y1010AU0F
float readDustSensor() {
  const int samples = 10;
  const float vRef = 3.3;
  const float zeroVolt = 0.6;
  const float ratio = 0.005;

  long total = 0;
  for (int i = 0; i < samples; i++) {
    digitalWrite(DUST_LED_PIN, LOW);
    delayMicroseconds(280);
    total += analogRead(DUST_MEASURE_PIN);
    delayMicroseconds(40);
    digitalWrite(DUST_LED_PIN, HIGH);
    delay(10);
  }

  float avgAdc = total / (float)samples;
  float voltage = avgAdc * (vRef / 4095.0);
  float density = (voltage - zeroVolt) / ratio;
  return max(density, 0.0f);
}

// Nhận lệnh bật/tắt cảnh báo từ Blynk
BLYNK_WRITE(VIRTUAL_ALERT) {
  alertEnabled = param.asInt();
  Serial.printf("Chế độ cảnh báo: %s\n", alertEnabled ? "Bật" : "Tắt");

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

  pinMode(DUST_LED_PIN, OUTPUT);
  pinMode(DUST_MEASURE_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(DUST_LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Air Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  connectToWifi();
  if (connectedToWifi) connectToBlynk();
}

void loop() {
  if (connectedToBlynk) Blynk.run();

  static unsigned long lastRead = 0;
  if (millis() - lastRead > 2000) {
    lastRead = millis();

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    dustDensity = readDustSensor();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Lỗi cảm biến DHT!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Loi cam bien!");
      delay(1000);
      return;
    }

    // In Serial
    Serial.printf("Nhiet do: %.1f°C | Do am: %.1f%% | Bui: %.1f ug/m3\n", temperature, humidity, dustDensity);

    // Hiển thị LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Nhiet do: %.1fC", temperature);
    lcd.setCursor(0, 1);
    lcd.printf("Do am: %.1f%%", humidity);
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Bui: %.1f ug/m3", dustDensity);
    delay(2000);

    // Gửi Blynk
    if (connectedToBlynk) {
      Blynk.virtualWrite(VIRTUAL_TEMP, temperature);
      Blynk.virtualWrite(VIRTUAL_HUMI, humidity);
      Blynk.virtualWrite(VIRTUAL_DUST, dustDensity);
    }

    // Kiểm tra cảnh báo
    if (alertEnabled &&
        (temperature > tempThreshold || humidity > humiThreshold || dustDensity > dustThreshold)) {

      digitalWrite(BUZZER_PIN, HIGH);
      delay(1000);
      digitalWrite(BUZZER_PIN, LOW);

      if (!alertSent && connectedToBlynk) {
        if (temperature > tempThreshold) {
          Blynk.logEvent("high_temp_warning", "Nhiệt độ vượt ngưỡng!");
          Serial.println(">> Cảnh báo nhiệt độ!");
        }
        if (humidity > humiThreshold) {
          Blynk.logEvent("high_humi_warning", "Độ ẩm vượt ngưỡng!");
          Serial.println(">> Cảnh báo độ ẩm!");
        }
        if (dustDensity > dustThreshold) {
          Blynk.logEvent("high_dust_warning", "Nồng độ bụi cao! Kiểm tra ngay!");
          Serial.println(">> Cảnh báo bụi!");
        }
        alertSent = true;
      }

      lcd.clear();
      if (temperature > tempThreshold) {
        lcd.setCursor(0, 0);
        lcd.print("Nhiet do cao!");
        lcd.setCursor(0, 1);
        lcd.printf("%.1fC", temperature);
      } else if (humidity > humiThreshold) {
        lcd.setCursor(0, 0);
        lcd.print("Do am cao!");
        lcd.setCursor(0, 1);
        lcd.printf("%.1f%%", humidity);
      } else if (dustDensity > dustThreshold) {
        lcd.setCursor(0, 0);
        lcd.print("Bui min cao!");
        lcd.setCursor(0, 1);
        lcd.printf("%.1fug/m3", dustDensity);
      }

      delay(2000); // Cho người dùng xem thông tin cảnh báo
    } else {
      alertSent = false;  // Reset nếu không cảnh báo
    }
  }
}
