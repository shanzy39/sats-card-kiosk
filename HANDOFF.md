# Build Notes — Sats Card Kiosk

_Status: **built & working end-to-end**, assembled in its 3D-printed case. Tap a
bearer card → sats melt into the kiosk's LNbits wallet, with live OLED status._

This is the deeper technical reference and build log. For the overview, wiring,
and step-by-step build guide, start with the [README](README.md). This document
covers the architecture, the on-card format, and the hard-won gotchas — useful if
you're extending the project or debugging your own build.

The companion project that *writes* the cards is
[Sats-Giveaway-Tag-Writer](https://github.com/shanzy39/Sats-Giveaway-Tag-Writer).

---

## Architecture

A tap triggers a five-stage pipeline, all on the ESP32:

1. **Read** — PN532 detects the card, authenticates sectors 2–15 with the default
   key, and reads the 672-byte payload region.
2. **Parse** — verify the `SATS` header and extract the voucher (type + length + bytes).
3. **Decode** — bech32-decode the `LNURL1…` string into an `https://…` withdraw URL.
4. **Claim** — over WiFi/HTTPS:
   - `GET` the withdraw URL → `{callback, k1, maxWithdrawable, …}`
   - `POST /api/v1/payments` on the kiosk wallet (`X-Api-Key: <invoice key>`,
     body `{"out":false,"amount":<sats>,"memo":"kiosk melt"}`) → get a bolt11 invoice
   - `GET callback?k1=<k1>&pr=<bolt11>` → LNbits pays it
5. **Show** — the OLED narrates each stage and lands on `SUCCESS! / N sats / melted!`.

Because the voucher's source wallet and the kiosk wallet live on the same LNbits
instance, the payout is an instant, feeless internal transfer.

Verified end-to-end with a fresh 10-sat voucher:
```
[1/3] Fetching withdraw params...   status 200,  voucher = 10 sats
[2/3] Creating invoice...           status 201,  invoice created
[3/3] Claiming voucher...           status 200,  {"status":"OK"}
*** SUCCESS! 10 sats melted into the kiosk wallet ***
```

## The on-card format

The contract between the writer and this kiosk. Payload lives in MIFARE Classic
1K **sectors 2–15, data blocks 0–2 of each** (blocks `s*4+b` for `s` in 2..15,
`b` in 0..2) = 42 blocks = 672 bytes, all under the default key `FFFFFFFFFFFF`:

| Offset | Field | Notes |
|--------|-------|-------|
| 0–3 | `"SATS"` | magic |
| 4 | version | `0x01` |
| 5 | type | `0x00` = LNURL, `0x01` = Cashu |
| 6–7 | length | uint16, big-endian |
| 8… | payload | UTF-8; the bare uppercase `LNURL1…` string |

The kiosk never touches sector trailers, block 0, or sector 1 (which may hold a
card's original custom-keyed data). v1 firmware melts **LNURL** cards; Cashu is
reserved in the format but not yet melted on-device.

## Pins & wiring

Two independent I2C buses so the reader and screen never conflict — all direct
jumpers, no breadboard.

| Device | Bus | ESP32 pins |
|--------|-----|-----------|
| PN532 (VCC→3V3) | `Wire` | SDA 21, SCL 22 |
| SSD1306 OLED (VCC→VIN/5V) | `Wire1` | SDA 32, SCL 33 |

Firmware essentials: `Wire.begin(21,22)`; `Wire1.begin(32,33)`;
`Adafruit_PN532 nfc(25,26)` (dummy IRQ/RESET — I2C polls fine without them);
`Adafruit_SSD1306 display(128,64,&Wire1,-1)` at address `0x3C`. The PN532's DIP
switches must be set to I2C (`1 0`).

## Bill of materials

| Item | Role |
|------|------|
| ESP32 dev board (WROOM-32) | The brain + WiFi |
| PN532 V3 NFC module | Reader (headers ship unsoldered; solder the 4-pin I2C row) |
| SSD1306 0.96" I2C OLED | Status screen |
| 8× female–female jumpers | Wiring |
| Micro-USB cable + 5V USB power | Power |
| MIFARE Classic 1K card(s) | The bearer cards (e.g. cracked hotel keycards) |
| 3D printer (optional) | For the `enclosure/` case |

Plus a running LNbits instance with a dedicated kiosk wallet. Total electronics
cost ≈ $25.

## Security model

- The kiosk stores only WiFi creds and an LNbits **invoice/read key** — which
  **cannot spend**. It can create invoices and read balances, nothing more. Even
  if a deployed kiosk's firmware were extracted, no funds could be drained.
- Vouchers are single-use, so a card can't be double-claimed.
- Secrets live in `config.h`, which is gitignored. Only `config.example.h`
  (placeholders) is committed.
- **v1 TLS uses `setInsecure()`** (skips cert validation) for simplicity —
  fine on a trusted home network; pin the ISRG Root X1 CA before deploying
  anywhere untrusted. (Tracked in Roadmap.)

## Gotchas (hard-won)

- **Arduino IDE 2.3.10's Serial Monitor can crash the whole IDE to a white screen
  on macOS**, especially around board resets. Reliable alternative: read serial
  from a terminal with `screen /dev/cu.usbserial-XXXX 115200`.
- **`screen` and the uploader both need the port.** Always quit `screen`
  (`Ctrl+A` `K` `Y`) before uploading. A *detached* zombie `screen` session will
  hold the port and produce garbled output — clear it with `screen -ls` then
  `screen -X -S <id> quit`. "Resource busy / could not open port" on upload means
  a session is still attached.
- **ESP32 boot chatter is normal garbage.** The bootloader prints at 74880 baud,
  so the first burst on connect looks like `◊◊◊` at 115200 — ignore it; clean
  text follows.
- **PN532 DIP switches** often ship with a protective film over them and are stiff
  — peel the film, then firmly set to I2C (`1 0`).
- **Hold the card flat and still** for the full read — the kiosk authenticates 14
  sectors in sequence (~2s), and a card that shifts mid-read gives a transient
  `Read failed` before it retries.
- **ESP32 is 2.4GHz-only** — it can't see 5GHz WiFi networks.

## Roadmap

- Buzzer + green/red status LEDs for tap feedback
- Pin the ISRG Root CA (drop `setInsecure()`)
- On-device Cashu melting
- Custom-key cards, so only this kiosk can read them
- Optional battery for a cordless kiosk

## Repo layout

| Path | What |
|------|------|
| `kiosk/kiosk.ino` | Full firmware — reader + OLED + WiFi + LNbits payout |
| `payload_reader/` | Reads & parses the on-card `SATS` payload (no network) |
| `reader_test/` | Minimal card-UID detector — first bring-up test |
| `config.example.h` | Secrets template → copy to `config.h` |
| `enclosure/` | 3D-printable case (STLs + OpenSCAD source) |
| `README.md` | Overview + full build guide |
