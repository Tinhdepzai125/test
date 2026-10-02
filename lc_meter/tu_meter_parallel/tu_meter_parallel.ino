/*
 * HID KEYBOARD BRIDGE - Blue Pill (STM32F103C8)
 * Upload: ST-Link v2
 * Lenh phim: CP2102 -> PA10 (RX) @ 115200 baud
 * HID output: Micro USB -> Target device
 *
 * Format lenh (ket thuc bang '\n'):
 *  KEY:<char>         -> Nhan + tha phim (vi du: KEY:A)
 *  MOD:<combo>        -> To hop phim (vi du: MOD:CTRL+C)
 *  PRESS:<key>        -> Giu phim
 *  RELEASE            -> Tha tat ca
 *  TYPE:<string>      -> Go ca chuoi
 *  DELAY:<ms>         -> Cho N millisecond
 */

#include <Arduino.h>
#include <USBHIDKeyboard.h>

USBHIDKeyboard Keyboard;

#define CMD_SERIAL  Serial1   // PA9=TX, PA10=RX
#define CMD_BAUD    115200

String cmdBuffer = "";

// ---- Map modifier ----
uint8_t parseModifier(const String& s) {
  if (s == "CTRL")  return KEY_LEFT_CTRL;
  if (s == "SHIFT") return KEY_LEFT_SHIFT;
  if (s == "ALT")   return KEY_LEFT_ALT;
  if (s == "WIN")   return KEY_LEFT_GUI;
  return 0;
}

// ---- Map phim dac biet + ky tu thuong ----
uint8_t parseKey(const String& s) {
  if (s == "ENTER")     return KEY_RETURN;
  if (s == "ESC")       return KEY_ESC;
  if (s == "BACKSPACE") return KEY_BACKSPACE;
  if (s == "TAB")       return KEY_TAB;
  if (s == "SPACE")     return ' ';
  if (s == "DELETE")    return KEY_DELETE;
  if (s == "HOME")      return KEY_HOME;
  if (s == "END")       return KEY_END;
  if (s == "PGUP")      return KEY_PAGE_UP;
  if (s == "PGDN")      return KEY_PAGE_DOWN;
  if (s == "UP")        return KEY_UP_ARROW;
  if (s == "DOWN")      return KEY_DOWN_ARROW;
  if (s == "LEFT")      return KEY_LEFT_ARROW;
  if (s == "RIGHT")     return KEY_RIGHT_ARROW;
  if (s == "F1")        return KEY_F1;
  if (s == "F2")        return KEY_F2;
  if (s == "F3")        return KEY_F3;
  if (s == "F4")        return KEY_F4;
  if (s == "F5")        return KEY_F5;
  if (s == "F6")        return KEY_F6;
  if (s == "F7")        return KEY_F7;
  if (s == "F8")        return KEY_F8;
  if (s == "F9")        return KEY_F9;
  if (s == "F10")       return KEY_F10;
  if (s == "F11")       return KEY_F11;
  if (s == "F12")       return KEY_F12;
  if (s.length() == 1)  return (uint8_t)s[0];
  return 0;
}

// ---- Xu ly MOD:CTRL+C, MOD:CTRL+SHIFT+T, v.v. ----
void handleMod(const String& combo) {
  // Tach token bang dau '+'
  uint8_t modKeys[4] = {};
  int modCount = 0;
  uint8_t finalKey = 0;

  int start = 0;
  while (start < (int)combo.length()) {
    int plus = combo.indexOf('+', start);
    String token = (plus == -1)
      ? combo.substring(start)
      : combo.substring(start, plus);
    token.trim();

    uint8_t mod = parseModifier(token);
    if (mod != 0) {
      modKeys[modCount++] = mod;
    } else {
      finalKey = parseKey(token);
    }
    if (plus == -1) break;
    start = plus + 1;
  }

  for (int i = 0; i < modCount; i++) Keyboard.press(modKeys[i]);
  if (finalKey) Keyboard.press(finalKey);
  delay(30);
  Keyboard.releaseAll();
}

// ---- Xu ly 1 lenh hoan chinh ----
void processCommand(const String& cmd) {
  if (cmd.startsWith("KEY:")) {
    String key = cmd.substring(4);
    key.trim();
    uint8_t k = parseKey(key);
    if (k) {
      Keyboard.press(k);
      delay(20);
      Keyboard.release(k);
    }

  } else if (cmd.startsWith("MOD:")) {
    String combo = cmd.substring(4);
    combo.trim();
    handleMod(combo);

  } else if (cmd.startsWith("PRESS:")) {
    String key = cmd.substring(6);
    key.trim();
    uint8_t k = parseKey(key);
    if (k) Keyboard.press(k);

  } else if (cmd == "RELEASE") {
    Keyboard.releaseAll();

  } else if (cmd.startsWith("TYPE:")) {
    String text = cmd.substring(5);
    // Giu nguyen case, gõ tung ky tu
    for (int i = 0; i < (int)text.length(); i++) {
      Keyboard.print(text[i]);
      delay(10); // chong mat key
    }

  } else if (cmd.startsWith("DELAY:")) {
    uint32_t ms = cmd.substring(6).toInt();
    if (ms > 0 && ms <= 5000) delay(ms);
  }
}

void setup() {
  // Khoi dong USB HID truoc
  Keyboard.begin();
  delay(1500); // cho host nhan dien USB

  // Khoi dong UART nhan lenh
  CMD_SERIAL.begin(CMD_BAUD);
  cmdBuffer.reserve(64);
}

void loop() {
  while (CMD_SERIAL.available()) {
    char c = (char)CMD_SERIAL.read();

    if (c == '\n' || c == '\r') {
      cmdBuffer.trim();
      if (cmdBuffer.length() > 0) {
        processCommand(cmdBuffer);
        cmdBuffer = "";
      }
    } else {
      if (cmdBuffer.length() < 63) {
        cmdBuffer += c;
      }
    }
  }
}
