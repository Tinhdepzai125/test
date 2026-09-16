/*
 * TU METER - RC CHARGING METHOD
 * Blue Pill (STM32F103C8) - Arduino framework
 * R chuan = 24kOhm 1%
 * LCD1602 dau PARALLEL (4-bit mode) - khong dung I2C
 * 🤡 by Tinh
 *
 * Ket noi mach do:
 *  PB0  -> 1 chan R 24k
 *  Chan kia R 24k -> Nut A -> Cx -> GND
 *  Nut A -> PA0 (ADC input)
 *
 * Ket noi LCD1602 (parallel, 4-bit):
 *  VSS -> GND
 *  VDD -> 5V
 *  VO  -> giua bien tro 10k (chinh contrast)
 *  RS  -> PB12
 *  RW  -> GND
 *  E   -> PB13
 *  D4  -> PB14
 *  D5  -> PB15
 *  D6  -> PB4
 *  D7  -> PB5
 *
 * LUU Y: khong dung PA9/PA10 cho LCD (trung UART1 TX/RX voi Serial.begin())
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

#define PIN_CHARGE   PB0
#define PIN_ADC      PA0

LiquidCrystal lcd(PB12, PB13, PB14, PB15, PB4, PB5);

const float R_REF = 24000.0;
const float VCC = 3.3;
const float THRESHOLD_RATIO = 0.632;

float readVoltage() {
  int raw = analogRead(PIN_ADC);
  return (raw / 4095.0) * VCC;
}

void dischargeCap() {
  pinMode(PIN_CHARGE, OUTPUT);
  digitalWrite(PIN_CHARGE, LOW);
  delay(50);
}

float measureCapacitance() {
  dischargeCap();

  float vStart = readVoltage();
  if (vStart > 0.1) {
    return -1;
  }

  float threshold = VCC * THRESHOLD_RATIO;

  uint32_t t0 = micros();
  digitalWrite(PIN_CHARGE, HIGH);

  uint32_t timeout = 2000000;
  float v;
  do {
    v = readVoltage();
    if (micros() - t0 > timeout) {
      digitalWrite(PIN_CHARGE, LOW);
      return -2;
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

  analogReadResolution(12);

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
