/*
 * C4 BOMB SIMULATOR - PASSIVE BUZZER
 * Blue Pill (STM32F103C8) - Arduino framework
 * Passive Buzzer (coi thu dong) noi tai PB8
 * LCD1602 PARALLEL (4-bit mode)
 * 🤡 C4 Simulator v2
 *
 * Nut bam:
 *  PA1 -> GND: Nhan de Kich hoat (ARM) / Giu 5s de Go bom (DEFUSE)
 *  PB6 -> GND: Nhan de Reset game
 *
 * Ket noi LCD1602:
 *  RS -> PB12, E -> PB13, D4 -> PB14, D5 -> PB15, D6 -> PB4, D7 -> PB5
 *
 * Ket noi Buzzer Thu Dong:
 *  PB8 -> Chan tin hieu (I/O hoac S) cua Passive Buzzer
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// ==== CAU HINH CHAN ====
#define PIN_BUZZER     PB8   // Passive Buzzer (dung ham tone())
#define PIN_BTN_ACTION PA1   // Nut ARM / DEFUSE
#define PIN_BTN_RESET  PB6   // Nut Reset

// ==== CAU HINH LCD (RS, E, D4, D5, D6, D7) ====
LiquidCrystal lcd(PB12, PB13, PB14, PB15, PB4, PB5);

// ==== THOI GIAN GAME ====
const uint32_t TOTAL_TIME_SEC = 40;       // 40 giay dem nguoc
const uint32_t DEFUSE_HOLD_TIME_MS = 5000; // Giu nut 5 giay de go bom

enum BombState {
  STATE_IDLE,      // Cho dat bom
  STATE_ARMED,     // Bom dang dem nguoc
  STATE_DEFUSED,   // Da go bom thanh cong
  STATE_EXPLODED   // Bom no!
};

BombState state = STATE_IDLE;

uint32_t startTime = 0;
uint32_t lastBeepTime = 0;
uint32_t defuseStartTime = 0;
bool isDefusing = false;

// Phat 1 tieng bip voi tan so (Hz) va thoi gian (ms)
void playBeep(uint16_t freq, uint32_t durationMs) {
  tone(PIN_BUZZER, freq, durationMs);
  delay(durationMs);
  noTone(PIN_BUZZER);
}

// Hieu ung am thanh Victory khi go bom thanh cong
void playDefusedSound() {
  tone(PIN_BUZZER, 1047, 100); delay(120); // Not C6
  tone(PIN_BUZZER, 1318, 100); delay(120); // Not E6
  tone(PIN_BUZZER, 1568, 250); delay(300); // Not G6
  noTone(PIN_BUZZER);
}

// Gia lap tieng no: quat tan so tu cao xuong thap tao am thanh rền ầm ầm
void playExplosionSound() {
  uint32_t startExplosion = millis();
  while (millis() - startExplosion < 2500) {
    for (int freq = 600; freq >= 80; freq -= 20) {
      tone(PIN_BUZZER, freq, 8);
      delay(3);
    }
  }
  noTone(PIN_BUZZER);
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_ACTION, INPUT_PULLUP);
  pinMode(PIN_BTN_RESET, INPUT_PULLUP);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  C4 SIMULATOR  ");
  lcd.setCursor(0, 1);
  lcd.print("Nhan PA1 de ARM!");
}

void loop() {
  uint32_t now = millis();

  // Nhan PB6 bat ky luc nao de Reset ve ban dau
  if (digitalRead(PIN_BTN_RESET) == LOW) {
    state = STATE_IDLE;
    noTone(PIN_BUZZER);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  C4 SIMULATOR  ");
    lcd.setCursor(0, 1);
    lcd.print("Nhan PA1 de ARM!");
    delay(300);
  }

  switch (state) {
    case STATE_IDLE: {
      // Nhan PA1 de Kich hoat / Dat bom
      if (digitalRead(PIN_BTN_ACTION) == LOW) {
        state = STATE_ARMED;
        startTime = millis();
        lastBeepTime = 0;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BOMB HAS BEEN");
        lcd.setCursor(0, 1);
        lcd.print("PLANTED! 00:40");
        
        playBeep(2400, 150); // Bip cao khi plant
        delay(500);
      }
      break;
    }

    case STATE_ARMED: {
      uint32_t elapsed = (now - startTime) / 1000;

      // Kiem tra het thoi gian -> NO!
      if (elapsed >= TOTAL_TIME_SEC) {
        state = STATE_EXPLODED;
        break;
      }

      uint32_t remaining = TOTAL_TIME_SEC - elapsed;

      // Update LCD thoi gian con lai
      lcd.setCursor(0, 1);
      lcd.print("Time: 00:");
      if (remaining < 10) lcd.print("0");
      lcd.print(remaining);
      lcd.print("   ");

      // Tinh chu ky bip: cang gan het giay cang keu dồn dập
      uint32_t beepInterval;
      if (remaining > 20)      beepInterval = 1000; // 1s/lan
      else if (remaining > 10) beepInterval = 500;  // 0.5s/lan
      else if (remaining > 5)  beepInterval = 250;  // 0.25s/lan
      else if (remaining > 2)  beepInterval = 125;  // 0.125s/lan
      else                     beepInterval = 60;   // Dồn dập cực nhanh

      if (now - lastBeepTime >= beepInterval) {
        lastBeepTime = now;
        // Phat tieng tít chuẩn CS ở tần số 2000Hz (2kHz) trong 30ms
        tone(PIN_BUZZER, 2000, 30);
      }

      // XU LY GO BOM (Giu nut PA1)
      if (digitalRead(PIN_BTN_ACTION) == LOW) {
        if (!isDefusing) {
          isDefusing = true;
          defuseStartTime = now;
        } else {
          uint32_t holdTime = now - defuseStartTime;
          uint32_t progress = (holdTime * 100) / DEFUSE_HOLD_TIME_MS;
          if (progress > 100) progress = 100;

          lcd.setCursor(0, 0);
          lcd.print("DEFUSING... ");
          if (progress < 10) lcd.print(" ");
          lcd.print(progress);
          lcd.print("% ");

          if (holdTime >= DEFUSE_HOLD_TIME_MS) {
            state = STATE_DEFUSED;
            isDefusing = false;
          }
        }
      } else {
        if (isDefusing) {
          isDefusing = false;
          lcd.setCursor(0, 0);
          lcd.print("BOMB PLANTED!   ");
        }
      }
      break;
    }

    case STATE_DEFUSED: {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BOMB HAS BEEN");
      lcd.setCursor(0, 1);
      lcd.print("DEFUSED! CT WIN");
      
      playDefusedSound();

      // Dung cho den khi nhan PB6 de reset
      while (digitalRead(PIN_BTN_RESET) == HIGH) {
        delay(50);
      }
      break;
    }

    case STATE_EXPLODED: {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  *** BOOM! *** ");
      lcd.setCursor(0, 1);
      lcd.print(" TERRORISTS WIN ");

      playExplosionSound();

      // Dung cho den khi nhan PB6 de reset
      while (digitalRead(PIN_BTN_RESET) == HIGH) {
        delay(50);
      }
      break;
    }
  }
}
