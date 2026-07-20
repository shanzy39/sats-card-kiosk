# sats-card-kiosk

**Tap a hotel keycard on a little box. Watch Bitcoin appear.**

`sats-card-kiosk` is a DIY, self-contained Lightning kiosk. You lay a repurposed
MIFARE Classic hotel keycard — one that's been loaded with a Bitcoin Lightning
voucher — onto the reader, and the kiosk automatically reads the voucher, claims
it over the internet, and melts the sats into a wallet. A little OLED screen
narrates the whole thing: `tap a card` → `Reading` → `10 sats / claiming...` →
`SUCCESS! melted!`.

No phone, no app, no computer needed at run time. It's an ESP32 microcontroller,
a $7 NFC reader, and a tiny screen — powered by any USB plug — talking to a
self-hosted [LNbits](https://lnbits.com) instance.

This is the redeemer half of a two-part project. Its companion,
[**Sats-Giveaway-Tag-Writer**](https://github.com/shanzy39/Sats-Giveaway-Tag-Writer),
is the *writer* — the browser tool that loads the vouchers onto the cards in the
first place. This repo is the physical machine that claims them back.

---

## What problem does this solve?

A cracked hotel keycard is just a rewritable MIFARE Classic 1K chip. The writer
project turns one into a **bearer card** — it carries a Lightning `LNURL`
withdraw voucher in its memory, like a gift card that holds real value.

But those cards are *closed-loop*: iPhones can't read MIFARE Classic at all, and
neither can many Android phones. So to actually redeem one, **you need a reader
you control.** That's this kiosk. Tap-to-redeem, in a box, that you own.

The result is a fun, physical way to hand out Bitcoin: load a stack of old hotel
cards with small amounts, and let people melt them at the kiosk. Great for demos,
events, and content.

---

## How it works (the flow)

```
   ┌────────────┐     ┌──────────┐     ┌─────────────┐     ┌──────────────┐
   │  Tap card  │ ──▶ │  Read &  │ ──▶ │  Decode the │ ──▶ │  Claim via   │
   │ on reader  │     │  parse   │     │   LNURL     │     │  LNbits over │
   │            │     │  payload │     │  (bech32)   │     │    HTTPS     │
   └────────────┘     └──────────┘     └─────────────┘     └──────────────┘
        NFC              sectors            on-device            WiFi
                         2–15               decode           sats → wallet
```

1. **Read.** The PN532 detects the card and authenticates memory sectors 2–15
   with the factory-default key, reading out the 672-byte payload region.
2. **Parse.** The firmware checks for the `SATS` header the writer wrote
   (`"SATS"` magic + version + type + length) and extracts the voucher string.
3. **Decode.** If it's an `LNURL`, the firmware bech32-decodes it on-device into
   a real `https://…` withdraw URL.
4. **Claim.** Over WiFi + HTTPS: it fetches the withdraw parameters, creates an
   invoice on the kiosk's own LNbits wallet, and submits it to the callback.
   LNbits pays the invoice — the sats move from the voucher's source wallet into
   the kiosk wallet. Because both live on the same LNbits instance, it's an
   instant, feeless internal transfer.
5. **Show.** The OLED reports every step and lands on `SUCCESS! / N sats / melted!`.

Spent, blank, or non-Lightning cards are handled gracefully (`Voucher dead or
used`, `Not a sats card`, etc.) and the kiosk returns to its idle screen.

---

## Bill of materials (~$25)

| Part | Notes |
|------|-------|
| **ESP32 dev board** (WROOM-32, 38-pin) | The brain. Built-in WiFi. |
| **PN532 NFC module (V3)** | The reader. Set its DIP switches to **I2C** mode (`1 0`). |
| **SSD1306 0.96" OLED (I2C, 128×64)** | The status screen. Address `0x3C`. |
| Female–female jumper wires | 8 total. No breadboard needed. |
| Micro-USB cable + any 5V USB power | Powers the whole thing. |
| A blank/loaded MIFARE Classic 1K card | The bearer card (e.g. a cracked hotel keycard). |

You also need a running **LNbits** instance (self-hosted or hosted) with a
wallet for the kiosk. See Configuration below.

---

## Wiring

Everything connects directly to the ESP32 with jumper wires — no breadboard.
The PN532 and OLED each get their own I2C bus so they never conflict.

**PN532 → ESP32** (I2C bus `Wire`):

| PN532 | ESP32 |
|-------|-------|
| GND | GND |
| VCC | 3V3 |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

**OLED → ESP32** (I2C bus `Wire1`):

| OLED | ESP32 |
|------|-------|
| GND | GND |
| VCC | VIN (5V) |
| SCL | GPIO 33 |
| SDA | GPIO 32 |

> The PN532 ships with header pins loose — you solder the 4-pin I2C header on.
> Set the PN532's DIP switches to I2C (`1` up / `0` down). The OLED's header is
> usually pre-soldered.

---

## Build & flash

1. **Install the [Arduino IDE](https://www.arduino.cc/en/software).**
2. **Add ESP32 board support.** In *Settings → Additional Boards Manager URLs*, add:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
   Then in *Boards Manager*, install **esp32 by Espressif Systems**. Select board
   **ESP32 Dev Module**.
3. **Install libraries** (*Library Manager*):
   - `Adafruit PN532`
   - `Adafruit SSD1306` (accept its dependencies: Adafruit GFX, BusIO)
4. **Create your config.** Copy `config.example.h` to `config.h` and fill in your
   WiFi and LNbits details (see below). `config.h` is gitignored — your secrets
   never get committed.
5. **Open `kiosk/kiosk.ino`**, select your board's serial port, and click Upload.
6. **Watch it go.** Plug the kiosk into any USB power and tap a card. For debug
   output, open a serial monitor at **115200 baud** (on macOS, the Arduino IDE's
   built-in monitor can be flaky — `screen /dev/cu.usbserial-XXXX 115200` in a
   terminal works well).

### Configuration (`config.h`)

```c
#define WIFI_SSID          "your-wifi-name"        // 2.4GHz only — ESP32 can't do 5GHz
#define WIFI_PASSWORD      "your-wifi-password"
#define LNBITS_HOST        "your-lnbits-domain.com"  // no https://, no trailing slash
#define LNBITS_INVOICE_KEY "your-kiosk-wallet-invoice-key"
```

> **Use a dedicated kiosk wallet.** Create a separate LNbits wallet just for the
> kiosk and use its **invoice/read key** (not the admin key). The invoice key can
> only *create* invoices and read — it can't spend — so even if someone extracted
> the firmware from a kiosk in the wild, they couldn't drain anything.

---

## Repo layout

| Path | What |
|------|------|
| `kiosk/kiosk.ino` | The full firmware — reader + screen + WiFi + LNbits payout. |
| `reader_test/` | Minimal sketch: detect a card and print its UID. First bring-up test. |
| `payload_reader/` | Reads and parses the on-card `SATS` payload (no network). |
| `config.example.h` | Template for your secrets. Copy to `config.h`. |
| `enclosure/` | 3D-printable case — `kiosk_base.stl`, `kiosk_lid.stl`, and the OpenSCAD source. |
| `HANDOFF.md` | Detailed build log / project state (dev reference). |

---

## Enclosure

The `enclosure/` folder has a two-part, print-in-place-free case (base + lid, no
supports). The lid has a window for the OLED, a thin "tap zone" over the reader
(NFC reads right through ~1.5mm of plastic), and a slot for the USB cable. Print
in any color; the top is embossed with **TAP** over the reader and **SATS KIOSK**
along the front. Edit `kiosk_enclosure.scad` if your boards sit differently.

---

## The on-card format

The contract between the writer and this kiosk. The payload lives in MIFARE
Classic sectors 2–15 (data blocks only), all under the default key
`FFFFFFFFFFFF`:

| Offset | Field | |
|--------|-------|-|
| 0–3 | `"SATS"` | magic bytes |
| 4 | version | `0x01` |
| 5 | type | `0x00` = LNURL, `0x01` = Cashu |
| 6–7 | length | uint16, big-endian |
| 8… | payload | UTF-8 (the `LNURL1…` string) |

v1 firmware supports **LNURL** cards. Cashu is reserved in the format but not yet
melted on-device.

---

## Status & roadmap

**Working end-to-end:** tap → read → decode → claim → sats in wallet, with live
OLED status. Proven on real hardware.

Ideas for later:
- Buzzer + status LEDs for tap feedback
- Custom-key cards (so only *your* kiosk can read them)
- On-device Cashu melting
- A small battery for a fully cordless kiosk

---

## Credits

Built by [shanzy39](https://github.com/shanzy39). Companion to
[Sats-Giveaway-Tag-Writer](https://github.com/shanzy39/Sats-Giveaway-Tag-Writer).
Not affiliated with any hotel or card brand — any MIFARE Classic 1K card works.
