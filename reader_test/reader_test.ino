// Sats Card Kiosk — reader test (first bring-up sketch)
// ESP32 + PN532 over I2C (GND->GND, VCC->3V3, SDA->21, SCL->22, no IRQ/RESET wired).
// Detects ISO14443A cards and prints their UID at 115200 baud.

#include <Wire.h>
#include <Adafruit_PN532.h>

// IRQ/RESET aren't wired — constructor wants pin numbers, I2C mode never uses them
Adafruit_PN532 nfc(25, 26);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Sats Card Kiosk — reader test ===");

  Wire.begin(21, 22);   // SDA, SCL
  nfc.begin();

  uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("PN532 not found — check DIP switches (1 ON, 2 OFF) and SDA/SCL wires");
    while (1) delay(1000);
  }

  Serial.print("Found PN532, firmware ");
  Serial.print((version >> 16) & 0xFF); Serial.print(".");
  Serial.println((version >> 8) & 0xFF);

  nfc.SAMConfig();
  Serial.println("Tap a card...");
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLen;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) {
    Serial.print("Card detected! UID: ");
    for (uint8_t i = 0; i < uidLen; i++) {
      if (uid[i] < 0x10) Serial.print("0");
      Serial.print(uid[i], HEX);
    }
    Serial.println();

    // Optional: greet a specific card by its UID (edit these bytes for your card)
    if (uidLen == 4 && uid[0] == 0xDE && uid[1] == 0xAD && uid[2] == 0xBE && uid[3] == 0xEF) {
      Serial.println(">>> Recognized your card! <<<");
    }
    delay(1500);
  }
}
