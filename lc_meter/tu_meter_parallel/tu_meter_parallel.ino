/*
 * C4 BOMB SIMULATOR - PASSIVE BUZZER
 * Blue Pill (STM32F103C8) - Arduino framework
 * Passive Buzzer noi tai PB8
 * LCD1602 PARALLEL (4-bit mode)
 * 🤡 C4 Simulator v2.1 - Now with rap
 *
 * Nut bam:
 *  PA1 -> GND: Nhan de ARM / Giu 5s de DEFUSE
 *  PB6 -> GND: Nhan de Reset
 *
 * LCD1602:
 *  RS->PB12, E->PB13, D4->PB14, D5->PB15, D6->PB4, D7->PB5
 *
 * Buzzer Thu Dong:
 *  PB8 -> Chan tin hieu
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// ==== CHAN ====
#define PIN_BUZZER     PB8
#define PIN_BTN_ACTION PA1
#define PIN_BTN_RESET  PB6

// ==== LCD ====
LiquidCrystal lcd(PB12, PB13, PB14, PB15, PB4, PB5);

// ==== CONFIG ====
const uint32_t TOTAL_TIME_SEC     = 40;
const uint32_t DEFUSE_HOLD_MS     = 5000;
const uint32_t RAP_TRIGGER_SEC    = 5;   // Bat dau rap khi con 5 giay

// ==== NOTE DEFINES (Hz) ====
// Octave 4
#define C4  262
#define D4  294
#define E4  330
#define F4  349
#define G4  392
#define A4  440
#define B4  494
// Octave 5
#define C5  523
#define D5  587
#define E5  659
#define F5  698
#define G5  784
#define A5  880
// Special
#define REST 0

// ==== ENUM ====
enum BombState {
  STATE_IDLE,
  STATE_ARMED,
  STATE_RAPPING,   // Trang thai moi: dang phat rap truoc khi no
  STATE_DEFUSED,
  STATE_EXPLODED
};

BombState state = STATE_IDLE;

uint32_t startTime       = 0;
uint32_t lastBeepTime    = 0;
uint32_t defuseStartTime = 0;
bool     isDefusing      = false;

// ===========================================================
//  HELPER: playNote - phat 1 not, tra quyen dieu khien ngay
//  (khong dung delay() de loop() van chay duoc)
//  Goi tone() voi duration, caller tu quan ly thoi gian.
// ===========================================================
inline void playNote(uint16_t freq, uint32_t dur) {
  if (freq == REST) {
    noTone(PIN_BUZZER);
  } else {
    tone(PIN_BUZZER, freq, dur);
  }
}

// ===========================================================
//  BEEP DON GIAN
// ===========================================================
void playBeep(uint16_t freq, uint32_t durationMs) {
  tone(PIN_BUZZER, freq, durationMs);
  delay(durationMs);
  noTone(PIN_BUZZER);
}

// ===========================================================
//  VICTORY SOUND
// ===========================================================
void playDefusedSound() {
  tone(PIN_BUZZER, 1047, 100); delay(120);
  tone(PIN_BUZZER, 1318, 100); delay(120);
  tone(PIN_BUZZER, 1568, 250); delay(300);
  noTone(PIN_BUZZER);
}

// ===========================================================
//  EXPLOSION SOUND
// ===========================================================
void playExplosionSound() {
  uint32_t t0 = millis();
  while (millis() - t0 < 2500) {
    for (int f = 600; f >= 80; f -= 20) {
      tone(PIN_BUZZER, f, 8);
      delay(3);
    }
  }
  noTone(PIN_BUZZER);
}

// ===========================================================
//  RAP SEQUENCE
//  Am muu: phat 1 melody 8-bit kieu "boom bap" ~4 giay
//  truoc khi no, LCD hien lyrics cho vui
//
//  Cau rap (buzzer rap): "Thoi gian het / Chay di thoi"
//  Beat: kick pattern bang tan so thap + melody tren cao
//
//  Format moi note: {freq, duration_ms}
//  Tong thoi gian ~4800ms, fit vao khoang RAP_TRIGGER_SEC
// ===========================================================

struct Note {
  uint16_t freq;
  uint16_t dur;   // ms
};

// Melody chinh - phat tren buzzer
// Kieu 8-bit chiptune rap hook, nhip 120bpm
// 1 beat = 500ms | 1/2 beat = 250ms | 1/4 = 125ms
const Note rapMelody[] = {
  // "Thoi -- gian -- het -- roi"
  {G4,  125}, {REST, 50},
  {G4,  125}, {REST, 50},
  {A4,  250}, {REST, 50},
  {G4,  125}, {REST, 50},
  {E4,  375}, {REST, 100},

  // "Chay -- di -- thoi -- mau"
  {F4,  125}, {REST, 50},
  {G4,  125}, {REST, 50},
  {A4,  125}, {REST, 50},
  {G4,  250}, {REST, 50},
  {E4,  375}, {REST, 100},

  // "BOOM" impact note - rising
  {C4,  80},  {REST, 30},
  {E4,  80},  {REST, 30},
  {G4,  80},  {REST, 30},
  {C5,  500}, {REST, 80},

  // Final warning trill
  {A4, 60}, {G4, 60}, {A4, 60}, {G4, 60},
  {A4, 60}, {G4, 60}, {A4, 60}, {REST, 50},
};
const uint8_t RAP_LEN = sizeof(rapMelody) / sizeof(rapMelody[0]);

// Lyrics hien tren LCD dong vao (thay doi theo tien trinh)
// Moi dong toi da 16 ky tu
const char* lyricsLine0[] = {
  ">> THOI GIAN HET",
  ">> CHAY DI THOI ",
  "***  B O O M  ***",
  "!!! TERRORISTS !!",
};
const char* lyricsLine1[] = {
  "   ROI ANH OI..  ",
  "   MAU LEN MAU!  ",
  "  3... 2... 1... ",
  "     W I N       ",
};
const uint8_t LYRICS_LEN = 4;

// ===========================================================
//  PLAY RAP - BLOCKING (chay 1 lan roi chuyen STATE_EXPLODED)
//  Duoc goi tu loop() khi vao STATE_RAPPING
// ===========================================================
void playRap() {
  lcd.clear();

  uint8_t lyricIdx = 0;
  uint32_t lyricChangeMs = 1200; // doi lyrics moi 1.2s

  lcd.setCursor(0, 0); lcd.print(lyricsLine0[0]);
  lcd.setCursor(0, 1); lcd.print(lyricsLine1[0]);
  uint32_t lastLyricTime = millis();

  for (uint8_t i = 0; i < RAP_LEN; i++) {
    // Update lyrics theo thoi gian
    uint32_t now = millis();
    if (now - lastLyricTime >= lyricChangeMs) {
      lyricIdx++;
      if (lyricIdx >= LYRICS_LEN) lyricIdx = LYRICS_LEN - 1;
      lcd.setCursor(0, 0); lcd.print(lyricsLine0[lyricIdx]);
      lcd.setCursor(0, 1); lcd.print(lyricsLine1[lyricIdx]);
      lastLyricTime = now;
      // Tang toc doi lyrics o cuoi
      if (lyricIdx >= 2) lyricChangeMs = 700;
    }

    // Phat not
    playNote(rapMelody[i].freq, rapMelody[i].dur);
    delay(rapMelody[i].dur);
  }

  noTone(PIN_BUZZER);
}

// ===========================================================
//  RESET GAME
// ===========================================================
void resetGame() {
  state        = STATE_IDLE;
  isDefusing   = false;
  noTone(PIN_BUZZER);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("  C4 SIMULATOR  ");
  lcd.setCursor(0, 1); lcd.print("Nhan PA1 de ARM!");
  delay(300);
}

// ===========================================================
//  SETUP
// ===========================================================
void setup() {
  pinMode(PIN_BUZZER,     OUTPUT);
  pinMode(PIN_BTN_ACTION, INPUT_PULLUP);
  pinMode(PIN_BTN_RESET,  INPUT_PULLUP);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("  C4 SIMULATOR  ");
  lcd.setCursor(0, 1); lcd.print("Nhan PA1 de ARM!");
}

// ===========================================================
//  LOOP
// ===========================================================
void loop() {
  uint32_t now = millis();

  // PB6 reset bat ky luc nao (tru khi dang rap/no de khong cat ngang)
  if (digitalRead(PIN_BTN_RESET) == LOW &&
      state != STATE_RAPPING &&
      state != STATE_EXPLODED &&
      state != STATE_DEFUSED) {
    resetGame();
    return;
  }

  switch (state) {

    // ----------------------------------------------------------
    case STATE_IDLE: {
      if (digitalRead(PIN_BTN_ACTION) == LOW) {
        state     = STATE_ARMED;
        startTime = millis();
        lastBeepTime = 0;

        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("BOMB HAS BEEN");
        lcd.setCursor(0, 1); lcd.print("PLANTED! 00:40");

        playBeep(2400, 150);
        delay(400);
      }
      break;
    }

    // ----------------------------------------------------------
    case STATE_ARMED: {
      uint32_t elapsed   = (now - startTime) / 1000;
      uint32_t remaining = (elapsed >= TOTAL_TIME_SEC) ? 0 : (TOTAL_TIME_SEC - elapsed);

      // Chuyen sang trang thai RAP khi con RAP_TRIGGER_SEC giay
      if (remaining <= RAP_TRIGGER_SEC) {
        noTone(PIN_BUZZER);
        state = STATE_RAPPING;
        break;
      }

      // Update LCD
      lcd.setCursor(0, 0); lcd.print("BOMB PLANTED!   ");
      lcd.setCursor(0, 1);
      lcd.print("Time: 00:");
      if (remaining < 10) lcd.print("0");
      lcd.print(remaining);
      lcd.print("   ");

      // Beep interval theo thoi gian con lai
      uint32_t beepInterval;
      if      (remaining > 20) beepInterval = 1000;
      else if (remaining > 10) beepInterval = 500;
      else if (remaining > 5)  beepInterval = 250;
      else                     beepInterval = 125;

      if (now - lastBeepTime >= beepInterval) {
        lastBeepTime = now;
        tone(PIN_BUZZER, 2000, 30);
      }

      // Xu ly DEFUSE
      if (digitalRead(PIN_BTN_ACTION) == LOW) {
        if (!isDefusing) {
          isDefusing      = true;
          defuseStartTime = now;
        }
        uint32_t holdTime = now - defuseStartTime;
        uint32_t progress = min((holdTime * 100) / DEFUSE_HOLD_MS, (uint32_t)100);

        lcd.setCursor(0, 0);
        lcd.print("DEFUSING... ");
        if (progress < 10) lcd.print(" ");
        lcd.print(progress);
        lcd.print("% ");

        if (holdTime >= DEFUSE_HOLD_MS) {
          state      = STATE_DEFUSED;
          isDefusing = false;
        }
      } else {
        if (isDefusing) {
          isDefusing = false;
          lcd.setCursor(0, 0); lcd.print("BOMB PLANTED!   ");
        }
      }
      break;
    }

    // ----------------------------------------------------------
    case STATE_RAPPING: {
      // Blocking: phat het bai rap roi chuyen STATE_EXPLODED
      playRap();
      state = STATE_EXPLODED;
      break;
    }

    // ----------------------------------------------------------
    case STATE_DEFUSED: {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("BOMB HAS BEEN");
      lcd.setCursor(0, 1); lcd.print("DEFUSED! CT WIN");

      playDefusedSound();

      while (digitalRead(PIN_BTN_RESET) == HIGH) delay(50);
      resetGame();
      break;
    }

    // ----------------------------------------------------------
    case STATE_EXPLODED: {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("  *** BOOM! *** ");
      lcd.setCursor(0, 1); lcd.print(" TERRORISTS WIN ");

      playExplosionSound();

      while (digitalRead(PIN_BTN_RESET) == HIGH) delay(50);
      resetGame();
      break;
    }
  }
}
