#include <Arduino.h>
#include <Mouse.h>

// Khai báo HardwareSerial2 phòng trường hợp core chưa mở sẵn (PA3 = RX, PA2 = TX)
#if !defined(ENABLE_HWSERIAL2)
HardwareSerial Serial2(PA3, PA2);
#endif

char rxBuffer[32];
uint8_t rxIndex = 0;

void processCommand(char* cmd);

void setup() {
  // Khởi tạo USB HID Mouse
  Mouse.begin();

  // Khởi tạo USART2 nhận từ ESP8266 (Baudrate 115200)
  Serial2.begin(115200);
}

void loop() {
  // Đọc dữ liệu từ ESP8266
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    
    if (c == '\n' || c == '\r') {
      if (rxIndex > 0) {
        rxBuffer[rxIndex] = '\0';
        processCommand(rxBuffer);
        rxIndex = 0;
      }
    } else if (rxIndex < sizeof(rxBuffer) - 1) {
      rxBuffer[rxIndex++] = c;
    }
  }
}

void processCommand(char* cmd) {
  // Gói tin di chuyển: "M,dx,dy"
  if (cmd[0] == 'M' && cmd[1] == ',') {
    int dx = 0, dy = 0;
    if (sscanf(cmd + 2, "%d,%d", &dx, &dy) == 2) {
      dx = constrain(dx, -127, 127);
      dy = constrain(dy, -127, 127);
      Mouse.move(dx, dy);
    }
  } 
  // Gói tin Click chuột trái: "CL"
  else if (strcmp(cmd, "CL") == 0) {
    Mouse.click(MOUSE_LEFT);
  }
}
