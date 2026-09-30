/*
 * C4 BOMB SIMULATOR
 * Blue Pill (STM32F103C8) - Arduino framework
 * Active Buzzer (mach dao dong tich hop) o PB8
 * LCD1602 PARALLEL (4-bit mode)
 * 🤡 C4 Simulator
 *
 * Nut bam:
 *  PA1 -> GND: Nhan de Kich hoat (ARM) / Giu 5s de Go bom (DEFUSE)
 *  PB6 -> GND: Nhan de Reset game
 *
 * Ket noi LCD1602:
 *  RS -> PB12, E -> PB13, D4 -> PB14, D5 -> PB15, D6 -> PB4, D7 -> PB5
 *
 * Ket noi Buzzer:
 *  PB8 -> Chan tin hieu / VCC cua Active Buzzer
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// ==== CAU HINH CHAN ====
#define PIN_BUZZER     PB8   // Active Buzzer (keo muc HIGH de keu)
#define PIN_BTN_ACTION PA1   // Nut ARM (Dat bom) va DEFUSE (Go bom)
#define PIN_BTN_RESET  PB6   // Nut Reset tro hoi

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

// Ham phat tieng bip ngan cho active buzzer
void playBeep(uint32_t durationMs) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(durationMs);
  digitalWrite(PIN_BUZZER, LOW);
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

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
    digitalWrite(PIN_BUZZER, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  C4 SIMULATOR  ");
    lcd.setCursor(0, 1);
    lcd.print("Nhan PA1 de ARM!");
    delay(300); // Debounce
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
        
        playBeep(200);
        delay(500);
      }
      break;
    }

    case STATE_ARMED: {
      uint32_t elapsed = (now - startTime) / 1000;

      // Kiem tra neu het thoi gian -> NO!
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
        playBeep(30);
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
      
      // Am thanh chien thang: Bip 3 nhịp dài
      for (int i = 0; i < 3; i++) {
        playBeep(150);
        delay(100);
      }

      // Dung cho den khi nhan PB6 de choi lai
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

      // Keo coi lien tuc 3 giay gia lap tieng no
      digitalWrite(PIN_BUZZER, HIGH);
      delay(3000);
      digitalWrite(PIN_BUZZER, LOW);

      // Dung cho den khi nhan PB6 de choi lai
      while (digitalRead(PIN_BTN_RESET) == HIGH) {
        delay(50);
      }
      break;
    }
  }
}
