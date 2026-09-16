/*
 * TU METER - RC CHARGING METHOD
 * Blue Pill (STM32F103C8) - Arduino framework
 * R chuan = 24kOhm 1%
 * LCD1602 dau PARALLEL (4-bit mode) - khong dung I2C
 * 🤡 by Tinh
 *
 * Nguyen ly:
 * 1. Xa tu ve 0V truoc
 * 2. Kich nap qua PB0 -> R 24k -> Cx -> GND
 * 3. Doc ADC lien tuc tai diem giua (PA0), do thoi gian dat 63.2% Vcc (= 1 hang so thoi gian tau = R*C)
 * 4. Cx = tau / R
 * 5. Hien thi len LCD1602 dau truc tiep (parallel, 4-bit mode)
 *
 * Ket noi mach do:
 *  PB0  -> 1 chan R 24k
 *  Chan kia R 24k -> Nut A -> Cx -> GND
 *  Nut A -> PA0 (ADC input)
 *
 * Ket noi LCD1602 (parallel, 4-bit):
 *  VSS -> GND
 *  VDD -> 5V
 *  VO  -> giua bien tro 10k (chinh contrast), 2 chan con lai cua bien tro noi 5V va GND
 *  RS  -> PB12
 *  RW  -> GND
 *  E   -> PB13
 *  D4  -> PB14
 *  D5  -> PB15
 *  D6  -> PA8
 *  D7  -> PA9
 *  A (backlight+) -> 5V (qua tro han che dong neu module khong co san)
 *  K (backlight-) -> GND
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// ==== CAU HINH CHAN DO ====
#define PIN_CHARGE   PB0   // kich nap tu
#define PIN_ADC      PA0   // doc dien ap tai nut A

// ==== CAU HINH CHAN LCD (RS, E, D4, D5, D6, D7) ====
LiquidCrystal lcd(PB12, PB13, PB14, PB15, PA8, PA9);

// ==== GIA TRI CHUAN ====
const float R_REF = 24000.0;   // 24 kOhm 1%
const float VCC = 3.3;
const float THRESHOLD_RATIO = 0.632; // 63.2% Vcc = 1 tau

// Doc ADC ra dien ap (STM32 ADC 12-bit, 0-4095)
float readVoltage() {
  int raw = analogRead(PIN_ADC);
  return (raw / 4095.0) * VCC;
}

// Xa tu ve gan 0V truoc khi do
void dischargeCap() {
  pinMode(PIN_CHARGE, OUTPUT);
  digitalWrite(PIN_CHARGE, LOW);
  delay(50); // du cho tu nho, tang neu do tu lon
}

// Do C: kich nap va bat thoi gian den khi dat nguong 63.2%
float measureCapacitance() {
  dischargeCap();

  float vStart = readVoltage();
  if (vStart > 0.1) {
    return -1; // tu chua xa het
  }

  float threshold = VCC * THRESHOLD_RATIO;

  uint32_t t0 = micros();
  digitalWrite(PIN_CHARGE, HIGH);

  uint32_t timeout = 2000000; // 2 giay toi da
  float v;
  do {
    v = readVoltage();
    if (micros() - t0 > timeout) {
      digitalWrite(PIN_CHARGE, LOW);
      return -2; // qua thoi gian
    }
  } while (v < threshold);

  uint32_t tau_us = micros() - t0;
  digitalWrite(PIN_CHARGE, LOW);

  float tau_s = tau_us / 1e6;
  float Cx = tau_s / R_REF;

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

  lcd.begin(16, 2);
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

  delay(800);
}
