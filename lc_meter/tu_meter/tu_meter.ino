/*
 * TU METER - RC CHARGING METHOD
 * Blue Pill (STM32F103C8) - Arduino framework
 * R chuan = 24kOhm 1%
 * 🤡 by Tinh
 *
 * Nguyen ly:
 * 1. Xa tu ve 0V truoc
 * 2. Kich nap qua PB0 -> R 24k -> Cx -> GND
 * 3. Doc ADC lien tuc tai diem giua (PA0), do thoi gian dat 63.2% Vcc (= 1 hang so thoi gian tau = R*C)
 * 4. Cx = tau / R
 * 5. Hien thi len LCD1602 qua I2C
 *
 * Ket noi:
 *  PB0  -> 1 chan R 24k
 *  Chan kia R 24k -> Nut A -> Cx -> GND
 *  Nut A -> PA0 (ADC input)
 *  PB8 (SCL), PB9 (SDA) -> LCD I2C
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==== CAU HINH CHAN ====
#define PIN_CHARGE   PB0   // kich nap tu
#define PIN_ADC      PA0   // doc dien ap tai nut A
#define PIN_DISCHARGE PB1  // xa tu qua tro nho (tuy chon, xem ghi chu duoi)

// ==== GIA TRI CHUAN ====
const float R_REF = 24000.0;   // 24 kOhm 1%
const float VCC = 3.3;
const float THRESHOLD_RATIO = 0.632; // 63.2% Vcc = 1 tau

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Doc ADC ra dien ap (STM32 ADC 12-bit, 0-4095)
float readVoltage() {
  int raw = analogRead(PIN_ADC);
  return (raw / 4095.0) * VCC;
}

// Xa tu ve gan 0V truoc khi do
void dischargeCap() {
  pinMode(PIN_CHARGE, OUTPUT);
  digitalWrite(PIN_CHARGE, LOW);
  // Cho xa qua chinh R_REF (khong can chan xa rieng neu cho du lau)
  delay(50); // du cho tu nho, tang neu do tu lon (vai trieu uF thi phai lau hon)
}

// Do C: kich nap va bat thoi gian den khi dat nguong 63.2%
float measureCapacitance() {
  dischargeCap();

  float vStart = readVoltage();
  if (vStart > 0.1) {
    // Tu chua xa het, co the ro ri hoac C qua lon
    return -1;
  }

  float threshold = VCC * THRESHOLD_RATIO;

  uint32_t t0 = micros();
  digitalWrite(PIN_CHARGE, HIGH); // bat dau nap

  uint32_t timeout = 2000000; // 2 giay toi da (cho tu lon)
  float v;
  do {
    v = readVoltage();
    if (micros() - t0 > timeout) {
      digitalWrite(PIN_CHARGE, LOW);
      return -2; // qua thoi gian, co the ho mach hoac C qua lon
    }
  } while (v < threshold);

  uint32_t tau_us = micros() - t0;
  digitalWrite(PIN_CHARGE, LOW); // ngung nap, chuan bi xa cho lan do sau

  float tau_s = tau_us / 1e6;
  float Cx = tau_s / R_REF; // don vi Farad

  return Cx;
}

void showResult(float Cx) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Do tu dien (C)");

  lcd.setCursor(0, 1);
  if (Cx == -1) {
    lcd.print("Tu chua xa het");
  } else if (Cx == -2) {
    lcd.print("Qua thoi gian!");
  } else if (Cx < 1e-9) {
    lcd.print(Cx * 1e12, 2); lcd.print(" pF");
  } else if (Cx < 1e-6) {
    lcd.print(Cx * 1e9, 3); lcd.print(" nF");
  } else {
    lcd.print(Cx * 1e6, 3); lcd.print(" uF");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_CHARGE, OUTPUT);
  digitalWrite(PIN_CHARGE, LOW);

  analogReadResolution(12); // STM32 ADC 12-bit

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Tu Meter 8=)");
  lcd.setCursor(0, 1);
  lcd.print("R chuan: 24k 1%");
  delay(1500);
}

void loop() {
  float Cx = measureCapacitance();
  showResult(Cx);

  if (Cx > 0) {
    Serial.print("Cx = ");
    Serial.print(Cx * 1e9, 4);
    Serial.println(" nF");
  } else {
    Serial.println(Cx == -1 ? "Loi: tu chua xa het" : "Loi: qua thoi gian");
  }

  delay(800); // giai lao truoc lan do tiep theo, cho tu xa het hoan toan
}
