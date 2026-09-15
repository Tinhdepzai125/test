/*
 * LC METER - RING DOWN METHOD + LCD1602 (I2C)
 * Blue Pill (STM32F103C8T6) - Arduino framework
 * 🤡 by Tinh
 *
 * Nguyên lý:
 * 1. Kích 1 xung ngắn vào tank LC qua GPIO
 * 2. Tank dao động tắt dần
 * 3. Comparator LM393 biến dao động thành sóng vuông
 * 4. Timer Input Capture (PA6, TIM3 CH1) đo chu kỳ
 * 5. Tính f = 1/T, rồi suy ra L hoặc C theo công thức Thomson
 * 6. Hiển thị kết quả lên LCD1602 qua module I2C (PCF8574)
 *
 * Kết nối:
 *  PB0        -> GPIO kích xung -> qua R 100R -> nút LC tank
 *  PA6        -> TIM3_CH1 input capture <- output của LM393
 *  PB8 (SCL)  -> LCD I2C SCL
 *  PB9 (SDA)  -> LCD I2C SDA
 *  PA0        -> nút bấm chọn chế độ (nối GND khi nhấn, dùng INPUT_PULLUP)
 *
 * Thư viện cần cài (Library Manager / ArduinoDroid):
 *  - LiquidCrystal_I2C (Frank de Brabander hoặc tương đương, hỗ trợ STM32)
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==== CẤU HÌNH CHÂN ====
#define PIN_EXCITE   PB0   // kích xung vào LC tank
#define PIN_CAPTURE  PA6   // input capture từ comparator (TIM3_CH1)
#define PIN_BUTTON   PA0   // nút chọn chế độ đo (nối GND khi nhấn)

// ==== LCD I2C (địa chỉ thường 0x27 hoặc 0x3F, dò bằng I2C scanner nếu không lên) ====
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==== GIÁ TRỊ CHUẨN (SỬA THEO LINH KIỆN THẬT CỦA MÀY) ====
const float L_REF = 100e-6;   // 100 µH - cuộn cảm chuẩn (dùng khi đo Cx)
const float C_REF = 1e-9;     // 1 nF   - tụ chuẩn (dùng khi đo Lx)

// ==== CHẾ ĐỘ ĐO ====
enum Mode { MODE_C, MODE_L };
Mode currentMode = MODE_C;

// ==== BIẾN INPUT CAPTURE ====
volatile uint32_t lastCapture = 0;
volatile uint32_t period = 0;
volatile bool newEdge = false;
volatile uint8_t edgeCount = 0;

HardwareTimer *timer3;

void captureISR() {
  uint32_t now = timer3->getCaptureCompare(1);
  if (edgeCount > 0) {
    if (now >= lastCapture) {
      period = now - lastCapture;
    } else {
      period = (0xFFFF - lastCapture) + now; // tràn timer
    }
    newEdge = true;
  }
  lastCapture = now;
  edgeCount++;
}

void setupCaptureTimer() {
  timer3 = new HardwareTimer(TIM3);
  timer3->setMode(1, TIMER_INPUT_CAPTURE_RISING, PIN_CAPTURE);
  timer3->setPrescaleFactor(72); // 72MHz / 72 = 1MHz -> 1 tick = 1us
  timer3->setOverflow(0xFFFF);
  timer3->attachInterrupt(1, captureISR);
  timer3->resume();
}

void exciteTank() {
  edgeCount = 0;
  newEdge = false;

  pinMode(PIN_EXCITE, OUTPUT);
  digitalWrite(PIN_EXCITE, HIGH);
  delayMicroseconds(5);
  digitalWrite(PIN_EXCITE, LOW);
  pinMode(PIN_EXCITE, INPUT); // thả nổi, để LC tự dao động tắt dần
}

float measureFrequency() {
  exciteTank();

  uint32_t t0 = micros();
  uint8_t samples = 0;
  uint32_t sumPeriod = 0;

  while (micros() - t0 < 2000) {
    if (newEdge) {
      newEdge = false;
      if (edgeCount > 2 && edgeCount < 8) {
        sumPeriod += period;
        samples++;
      }
      if (edgeCount >= 8) break;
    }
  }

  if (samples == 0) return 0;
  float avgPeriodUs = (float)sumPeriod / samples;
  return 1e6 / avgPeriodUs; // Hz
}

// Debounce nút bấm, đổi chế độ khi nhấn
void checkButton() {
  static bool lastState = HIGH;
  static uint32_t lastDebounce = 0;
  bool state = digitalRead(PIN_BUTTON);

  if (state != lastState) {
    lastDebounce = millis();
  }
  if ((millis() - lastDebounce) > 50) {
    if (state == LOW && lastState == HIGH) {
      currentMode = (currentMode == MODE_C) ? MODE_L : MODE_C;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(currentMode == MODE_C ? "Che do: Do TU" : "Che do: Do CUON");
      delay(500);
    }
  }
  lastState = state;
}

void showResult(float f, float value, bool isCap) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(isCap ? "Do TU (C)" : "Do CUON (L)");

  lcd.setCursor(0, 1);
  if (f <= 0) {
    lcd.print("Ko bat duoc dao");
    return;
  }

  if (isCap) {
    if (value < 1e-9) {
      lcd.print(value * 1e12, 2); lcd.print(" pF");
    } else {
      lcd.print(value * 1e9, 3); lcd.print(" nF");
    }
  } else {
    if (value < 1e-6) {
      lcd.print(value * 1e9, 2); lcd.print(" nH");
    } else {
      lcd.print(value * 1e6, 3); lcd.print(" uH");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_EXCITE, INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("LC Meter 8=)");
  lcd.setCursor(0, 1);
  lcd.print("Nhan nut de do");

  setupCaptureTimer();
  Serial.println("LC Meter ready. Nhan nut de doi che do, tu dong do lien tuc.");
  delay(1000);
}

void loop() {
  checkButton();

  float f = measureFrequency();
  float LC = (f > 0) ? 1.0 / (4 * PI * PI * f * f) : 0;

  if (currentMode == MODE_C) {
    float Cx = (f > 0) ? LC / L_REF : 0;
    showResult(f, Cx, true);
    if (f > 0) {
      Serial.print("f="); Serial.print(f); Serial.print(" Hz  Cx=");
      Serial.print(Cx * 1e9, 4); Serial.println(" nF");
    }
  } else {
    float Lx = (f > 0) ? LC / C_REF : 0;
    showResult(f, Lx, false);
    if (f > 0) {
      Serial.print("f="); Serial.print(f); Serial.print(" Hz  Lx=");
      Serial.print(Lx * 1e6, 4); Serial.println(" uH");
    }
  }

  delay(400); // giãn chu kỳ đo, tránh LCD nháy quá nhanh
}
