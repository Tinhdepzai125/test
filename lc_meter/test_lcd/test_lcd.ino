/*
 * I2C SCANNER - tim dia chi thiet bi I2C dang cam
 * Blue Pill (STM32F103C8)
 * Dung de tim dia chi LCD I2C (thuong la 0x27 hoac 0x3F)
 *
 * Ket noi: PB9 = SDA, PB8 = SCL (giu nguyen nhu module LCD)
 */

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(1000);
  Serial.println("I2C Scanner bat dau...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Dang quet...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Tim thay thiet bi tai dia chi 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("Khong tim thay thiet bi nao! Kiem tra day SDA/SCL, GND, VCC.");
  } else {
    Serial.println("Quet xong.");
  }

  delay(3000);
}
