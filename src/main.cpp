#include <Mouse.h>

void setup() {
  Serial2.begin(115200); // PA2=TX, PA3=RX <-> ESP8266
  Mouse.begin();
}

void loop() {
  if (Serial2.available()) {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("M,")) {
      int c1 = cmd.indexOf(',', 2);
      if (c1 > 0) {
        int dx = cmd.substring(2, c1).toInt();
        int dy = cmd.substring(c1 + 1).toInt();
        Mouse.move(dx, dy);
      }
    } else if (cmd == "CL") {
      Mouse.click(MOUSE_LEFT);
    } else if (cmd == "DL") {
      Mouse.press(MOUSE_LEFT);
    } else if (cmd == "UL") {
      Mouse.release(MOUSE_LEFT);
    }
  }
}
