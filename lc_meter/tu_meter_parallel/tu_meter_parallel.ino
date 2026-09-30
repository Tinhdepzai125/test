/*
 * BAD APPLE! - FULL SONG (PASSIVE BUZZER EDITION)
 * Board: Blue Pill (STM32F103C8)
 * Buzzer: Passive Buzzer o PB8
 * LCD: LCD1602 Parallel (PB12, PB13, PB14, PB15, PB4, PB5)
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

#define PIN_BUZZER PB8

// Định nghĩa tần số nốt nhạc (Hz)
#define REST 0
#define NOTE_A3  220
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880

// Cấu trúc nén dữ liệu: Tần số (2 byte) + Độ dài (2 byte) = 4 byte / nốt
struct Note {
  uint16_t freq;
  uint16_t durationMs;
};

LiquidCrystal lcd(PB12, PB13, PB14, PB15, PB4, PB5);

// Toàn bộ giai điệu full bài Bad Apple! lưu trên FLASH memory (RODATA)
const Note fullMelody[] PROGMEM = {
  // === INTRO ===
  {NOTE_D4, 200}, {NOTE_E4, 200}, {NOTE_F4, 200}, {NOTE_G4, 200},
  {NOTE_A4, 400}, {NOTE_F4, 400}, {NOTE_D4, 200}, {NOTE_C4, 200},
  {NOTE_D4, 200}, {NOTE_E4, 200}, {NOTE_F4, 200}, {NOTE_D4, 200},
  {NOTE_C4, 200}, {NOTE_A3, 200}, {NOTE_C4, 200}, {NOTE_D4, 400},
  
  {NOTE_D4, 200}, {NOTE_E4, 200}, {NOTE_F4, 200}, {NOTE_G4, 200},
  {NOTE_A4, 400}, {NOTE_F4, 400}, {NOTE_D4, 200}, {NOTE_C4, 200},
  {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_D4, 200}, {NOTE_C4, 200}, {NOTE_D4, 800},

  // === VERSE 1 ===
  {NOTE_D4, 200}, {NOTE_E4, 200}, {NOTE_F4, 200}, {NOTE_G4, 200},
  {NOTE_A4, 200}, {NOTE_A4, 200}, {NOTE_A4, 200}, {NOTE_C5, 200},
  {NOTE_G4, 200}, {NOTE_G4, 200}, {NOTE_G4, 200}, {NOTE_A4, 200},
  {NOTE_F4, 200}, {NOTE_F4, 200}, {NOTE_E4, 200}, {NOTE_D4, 200},

  {NOTE_D4, 200}, {NOTE_E4, 200}, {NOTE_F4, 200}, {NOTE_G4, 200},
  {NOTE_A4, 200}, {NOTE_A4, 200}, {NOTE_A4, 200}, {NOTE_C5, 200},
  {NOTE_G4, 400}, {NOTE_A4, 400}, {NOTE_D4, 800},

  // === CHORUS 1 (Điệp khúc cao trào) ===
  {NOTE_D5, 200}, {NOTE_C5, 200}, {NOTE_A4, 200}, {NOTE_F4, 200},
  {NOTE_G4, 400}, {NOTE_A4, 400}, {NOTE_D5, 200}, {NOTE_C5, 200},
  {NOTE_A4, 200}, {NOTE_F4, 200}, {NOTE_G4, 200}, {NOTE_A4, 200},
  {NOTE_F4, 200}, {NOTE_E4, 200}, {NOTE_D4, 200}, {NOTE_C4, 200},

  {NOTE_D4, 200}, {NOTE_E4, 200}, {NOTE_F4, 200}, {NOTE_G4, 200},
  {NOTE_A4, 400}, {NOTE_F4, 400}, {NOTE_D4, 200}, {NOTE_C4, 200},
  {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_D4, 200}, {NOTE_C4, 200}, {NOTE_D4, 800},

  // === VERSE 2 ===
  {NOTE_A4, 200}, {NOTE_C5, 200}, {NOTE_D5, 400}, {NOTE_D5, 200}, {NOTE_C5, 200}, {NOTE_A4, 400},
  {NOTE_G4, 200}, {NOTE_A4, 200}, {NOTE_C5, 400}, {NOTE_A4, 200}, {NOTE_G4, 200}, {NOTE_F4, 400},
  {NOTE_D4, 200}, {NOTE_F4, 200}, {NOTE_G4, 200}, {NOTE_A4, 200}, {NOTE_C5, 400}, {NOTE_D5, 800},

  // === BRIDGE / SOLO ===
  {NOTE_F5, 200}, {NOTE_E5, 200}, {NOTE_D5, 200}, {NOTE_C5, 200},
  {NOTE_D5, 400}, {NOTE_A4, 400}, {NOTE_C5, 200}, {NOTE_A4, 200},
  {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_D4, 800},

  // === REPEAT CHORUS (MAX SPEED) ===
  {NOTE_D5, 180}, {NOTE_C5, 180}, {NOTE_A4, 180}, {NOTE_F4, 180},
  {NOTE_G4, 360}, {NOTE_A4, 360}, {NOTE_D5, 180}, {NOTE_C5, 180},
  {NOTE_A4, 180}, {NOTE_F4, 180}, {NOTE_G4, 180}, {NOTE_A4, 180},
  {NOTE_F4, 180}, {NOTE_E4, 180}, {NOTE_D4, 180}, {NOTE_C4, 180},
  {NOTE_F4, 360}, {NOTE_E4, 360}, {NOTE_D4, 720},

  // === OUTRO ===
  {NOTE_D4, 250}, {NOTE_E4, 250}, {NOTE_F4, 250}, {NOTE_G4, 250},
  {NOTE_A4, 500}, {NOTE_F4, 500}, {NOTE_D4, 1000}, {REST, 500}
};

const uint16_t TOTAL_NOTES = sizeof(fullMelody) / sizeof(Note);

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  lcd.begin(16, 2);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" BAD APPLE FULL ");
  lcd.setCursor(0, 1);
  lcd.print("  STM32F103C8T6 ");
  delay(2000);
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Playing BadApple");

  for (uint16_t i = 0; i < TOTAL_NOTES; i++) {
    // Đọc trực tiếp dữ liệu nốt từ Flash memory
    Note currentNote;
    memcpy_P(&currentNote, &fullMelody[i], sizeof(Note));

    if (currentNote.freq != REST) {
      tone(PIN_BUZZER, currentNote.freq, currentNote.durationMs * 0.88);
    } else {
      noTone(PIN_BUZZER);
    }

    // Cập nhật Progress bar trên LCD1602
    uint8_t progress = map(i, 0, TOTAL_NOTES - 1, 0, 16);
    lcd.setCursor(0, 1);
    for (uint8_t p = 0; p < 16; p++) {
      if (p < progress) lcd.write(255);
      else lcd.print(" ");
    }

    delay(currentNote.durationMs);
    noTone(PIN_BUZZER);
  }

  // Kết thúc bài nghỉ 3s rồi hát lại
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   FINISHED!    ");
  delay(3000);
}
