// Sats Card Kiosk — payload reader
// Build day 2 (2026-07-19). Reads the SATS voucher payload out of a bearer card.
// ESP32 + PN532 over I2C (GND->GND, VCC->3V3, SDA->21, SCL->22, no IRQ/RESET wired).
// Auths sectors 2-15 with the default key, reads 672 bytes, parses the SATS header.
// Serial @ 115200. Read output with: screen /dev/cu.usbserial-0001 115200

#include <Wire.h>
#include <Adafruit_PN532.h>

Adafruit_PN532 nfc(25, 26);

uint8_t defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Sats Card Kiosk — payload reader ===");

  Wire.begin(21, 22);
  nfc.begin();

  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 not found — check wiring");
    while (1) delay(1000);
  }

  nfc.SAMConfig();
  Serial.println("Tap a bearer card...");
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLen;

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) return;

  Serial.print("\nCard detected, UID: ");
  for (uint8_t i = 0; i < uidLen; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
  }
  Serial.println();

  if (uidLen != 4) {
    Serial.println("Not a MIFARE Classic card — can't be a bearer card");
    delay(2000);
    return;
  }

  // Payload region: sectors 2-15, data blocks 0-2 of each = 672 bytes
  uint8_t payload[672];
  int idx = 0;

  for (uint8_t s = 2; s <= 15; s++) {
    if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLen, s * 4, 0, defaultKey)) {
      Serial.print("Auth failed at sector ");
      Serial.println(s);
      Serial.println("(keep the card still and flat, then try again)");
      delay(2000);
      return;
    }
    for (uint8_t b = 0; b < 3; b++) {
      if (!nfc.mifareclassic_ReadDataBlock(s * 4 + b, payload + idx)) {
        Serial.print("Read failed at block ");
        Serial.println(s * 4 + b);
        delay(2000);
        return;
      }
      idx += 16;
    }
  }

  // Parse the SATS header: "SATS" | version | type | length (uint16 BE) | payload
  if (!(payload[0] == 'S' && payload[1] == 'A' && payload[2] == 'T' && payload[3] == 'S')) {
    Serial.println("No SATS payload on this card (blank or hotel data)");
    delay(2000);
    return;
  }

  uint8_t version = payload[4];
  uint8_t type = payload[5];
  uint16_t len = ((uint16_t)payload[6] << 8) | payload[7];

  Serial.print("SATS card! format v");
  Serial.print(version);
  Serial.print(", type: ");
  Serial.print(type == 0x00 ? "LNURL" : type == 0x01 ? "Cashu" : "unknown");
  Serial.print(", length: ");
  Serial.println(len);

  if (len > 664) {
    Serial.println("Length is corrupt — re-tap the card");
    delay(2000);
    return;
  }

  Serial.println("--- Voucher on this card ---");
  for (uint16_t i = 0; i < len; i++) Serial.write(payload[8 + i]);
  Serial.println("\n----------------------------");

  delay(3000);
}
