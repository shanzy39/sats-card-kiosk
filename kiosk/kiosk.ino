// Sats Card Kiosk — full firmware (reader + OLED + WiFi + LNbits payout)
// Tap a MIFARE Classic bearer card -> read SATS payload -> decode LNURL ->
// fetch withdraw params -> create invoice on kiosk wallet -> claim -> sats land,
// with live status on a 0.96" SSD1306 OLED.
//
// Hardware (all direct female-female jumpers to the ESP32, no breadboard):
//   PN532 (I2C bus "Wire"):  GND->GND, VCC->3V3, SDA->21, SCL->22
//   OLED  (I2C bus "Wire1"): GND->GND, VCC->VIN(5V), SDA->32, SCL->33   addr 0x3C
// Two independent I2C buses so the devices never share pins.
//
// Needs config.h (gitignored) alongside this file:
//   WIFI_SSID, WIFI_PASSWORD, LNBITS_HOST, LNBITS_INVOICE_KEY
// Libraries: Adafruit PN532, Adafruit SSD1306, Adafruit GFX, Adafruit BusIO.
// Serial @ 115200. v1 uses setInsecure() (skips TLS cert check) — harden later.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

Adafruit_PN532 nfc(25, 26);
Adafruit_SSD1306 display(128, 64, &Wire1, -1);
uint8_t defaultKey[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
const char* BECH32_CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

void oled(const String& big, const String& small1="", const String& small2=""){
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2); display.setCursor(0,0); display.println(big);
  display.setTextSize(1);
  if(small1.length()){ display.setCursor(0,28); display.println(small1); }
  if(small2.length()){ display.setCursor(0,44); display.println(small2); }
  display.display();
}
void idle(){
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);  display.println("SATS");
  display.setCursor(0,20); display.println("KIOSK");
  display.setTextSize(1); display.setCursor(0,52); display.println("tap a card");
  display.display();
}

int bech32CharValue(char c){ if(c>='A'&&c<='Z')c=c-'A'+'a'; for(int i=0;i<32;i++)if(BECH32_CHARSET[i]==c)return i; return -1; }
int decodeLnurl(const char* lnurl,int lnurlLen,char* out,int outMax){
  int sep=-1; for(int i=0;i<lnurlLen;i++)if(lnurl[i]=='1')sep=i; if(sep<0)return -1;
  int dataStart=sep+1; int dataLen=lnurlLen-dataStart-6; if(dataLen<=0)return -1;
  uint32_t acc=0; int bits=0,outIdx=0;
  for(int i=0;i<dataLen;i++){ int v=bech32CharValue(lnurl[dataStart+i]); if(v<0)return -1;
    acc=(acc<<5)|v; bits+=5; while(bits>=8){bits-=8; if(outIdx>=outMax-1)return -1; out[outIdx++]=(char)((acc>>bits)&0xFF);} }
  out[outIdx]='\0'; return outIdx;
}
String jsonStr(const String& b, const char* k){ String p=String("\"")+k+"\":\""; int i=b.indexOf(p); if(i<0)return ""; i+=p.length(); int j=b.indexOf('"',i); if(j<0)return ""; return b.substring(i,j); }
long jsonNum(const String& b, const char* k){ String p=String("\"")+k+"\":"; int i=b.indexOf(p); if(i<0)return -1; i+=p.length(); int j=i; while(j<(int)b.length()&&b[j]>='0'&&b[j]<='9')j++; return b.substring(i,j).toInt(); }
void connectWiFi(){ if(WiFi.status()==WL_CONNECTED)return; WiFi.begin(WIFI_SSID,WIFI_PASSWORD); int t=0; while(WiFi.status()!=WL_CONNECTED&&t<40){delay(500);t++;} }
String httpsGET(const String& u, int& s){ WiFiClientSecure c; c.setInsecure(); HTTPClient h; String b=""; if(h.begin(c,u)){s=h.GET(); if(s>0)b=h.getString(); h.end();} else s=-999; return b; }
String httpsPOST(const String& u, const String& k, const String& body, int& s){ WiFiClientSecure c; c.setInsecure(); HTTPClient h; String b=""; if(h.begin(c,u)){h.addHeader("Content-Type","application/json"); h.addHeader("X-Api-Key",k); s=h.POST(body); if(s>0)b=h.getString(); h.end();} else s=-999; return b; }

void setup(){
  Serial.begin(115200); delay(1000);
  Wire1.begin(32, 33);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled("SATS","kiosk","connecting wifi...");
  connectWiFi();
  Wire.begin(21, 22); nfc.begin();
  if(!nfc.getFirmwareVersion()){ oled("ERROR","reader not","found"); while(1)delay(1000); }
  nfc.SAMConfig();
  idle();
  Serial.println("Ready.");
}

void loop(){
  uint8_t uid[7],uidLen;
  if(!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A,uid,&uidLen))return;
  if(uidLen!=4){delay(1500);return;}

  oled("Reading","hold still...");
  uint8_t payload[672]; int idx=0;
  for(uint8_t s=2;s<=15;s++){
    if(!nfc.mifareclassic_AuthenticateBlock(uid,uidLen,s*4,0,defaultKey)){ oled("Re-tap","hold flat"); delay(2000); idle(); return; }
    for(uint8_t b=0;b<3;b++){ if(!nfc.mifareclassic_ReadDataBlock(s*4+b,payload+idx)){ oled("Re-tap","hold flat"); delay(2000); idle(); return; } idx+=16; }
  }
  if(!(payload[0]=='S'&&payload[1]=='A'&&payload[2]=='T'&&payload[3]=='S')){ oled("Not a","sats card"); delay(2500); idle(); return; }
  uint8_t type=payload[5]; uint16_t len=((uint16_t)payload[6]<<8)|payload[7];
  if(len>664||type!=0x00){ oled("Bad card","or Cashu"); delay(2500); idle(); return; }

  char lnurlBuf[680]; for(uint16_t i=0;i<len;i++)lnurlBuf[i]=(char)payload[8+i]; lnurlBuf[len]='\0';
  char url[512]; if(decodeLnurl(lnurlBuf,len,url,sizeof(url))<0){ oled("Decode","failed"); delay(2500); idle(); return; }

  connectWiFi();
  oled("Melting","fetching...");
  int st; String params=httpsGET(String(url),st);
  if(st!=200 || jsonStr(params,"tag")!="withdrawRequest"){ oled("Voucher","dead or","used"); delay(3000); idle(); return; }
  String callback=jsonStr(params,"callback"), k1=jsonStr(params,"k1");
  long sats=jsonNum(params,"maxWithdrawable")/1000;
  if(callback==""||k1==""||sats<=0){ oled("Bad","voucher"); delay(3000); idle(); return; }

  oled(String(sats)+" sats","creating invoice...");
  String invUrl=String("https://")+LNBITS_HOST+"/api/v1/payments";
  String invBody=String("{\"out\":false,\"amount\":")+sats+",\"memo\":\"kiosk melt\"}";
  int st2; String invResp=httpsPOST(invUrl,LNBITS_INVOICE_KEY,invBody,st2);
  String pr=jsonStr(invResp,"payment_request"); if(pr=="")pr=jsonStr(invResp,"bolt11");
  if(pr==""){ oled("Invoice","failed"); delay(3000); idle(); return; }

  oled(String(sats)+" sats","claiming...");
  String claimUrl=callback+"&k1="+k1+"&pr="+pr;
  int st3; String claimResp=httpsGET(claimUrl,st3);
  if(jsonStr(claimResp,"status")=="OK"){
    oled("SUCCESS!",String(sats)+" sats","melted!");
    Serial.printf("SUCCESS %ld sats\n",sats);
  } else { oled("Claim","failed"); Serial.println(claimResp); }
  delay(6000);
  idle();
}
