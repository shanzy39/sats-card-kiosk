// Sats Card Kiosk — configuration TEMPLATE.
//
// Copy this file to `config.h` (same folder) and fill in your real values.
// config.h is gitignored and must NEVER be committed — it holds secrets.
//
// In the Arduino IDE, config.h must sit in the same sketch folder as the .ino.

// --- WiFi (2.4GHz only — the ESP32 cannot use 5GHz networks) ---
#define WIFI_SSID     "your-2.4ghz-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"

// --- LNbits (your instance; no https:// prefix, no trailing slash) ---
#define LNBITS_HOST        "your-lnbits-domain.com"
#define LNBITS_INVOICE_KEY "your-kiosk-wallet-invoice-key"
