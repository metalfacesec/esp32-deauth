#include "esp_wifi.h"
#include <vector>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

using namespace std;

#define maxCh 11
#define MAX_MACS_ON_SCREEN 5
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int curChannel = 1;
String currentMac = "";
String defaultTTL = "60";

const wifi_promiscuous_filter_t filt = {
    .filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
};

typedef struct {
    uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct {
  int16_t fctl;
  int16_t duration;
  MacAddr da;
  MacAddr sa;
  MacAddr bssid;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;

vector<String> macArray;

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
    WifiMgmtHdr *wh = (WifiMgmtHdr*)p->payload;

    MacAddr mac_add = (MacAddr)wh->sa;
    String macAttack;
    for (int i = 0; i < sizeof(mac_add.mac); i++) {
        String macDigit = String(mac_add.mac[i], HEX);
        if (macDigit.length() == 1) {
            macDigit = "0" + macDigit;
        }
        
        macAttack += macDigit;
        if (i != sizeof(mac_add.mac) - 1) {
          macAttack += ":";
        }
    }

    if (macArray.size() >= 5) {
        return;
    }

    macAttack.toUpperCase();
    
    for (int i = 0; i < macArray.size(); i++) {
        if (macAttack == macArray[i]) {
            Serial.println("Returning");
            return;
        }
    }

    macArray.push_back(macAttack);   
}

void setup() {
    Serial.begin(115200);
  
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(&sniffer);
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
  
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
  
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    display.display();
    delay(2000);
    display.clearDisplay();
  
    display.println("Hello, world!");
  
    display.display();
}

void displayFoundMac() {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    delay(2000);
    display.clearDisplay();

    display.setCursor(0, 6);
    display.println("Active Wifi MACs");

    for (int i = i; i < macArray.size(); i++) {
        display.setCursor(0, (i * 9) + 16);
        display.println(macArray[i]);
    }
  
    display.display();
}

void loop() {
    if(curChannel > maxCh){ 
        curChannel = 1;
    }
    
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
    delay(5000);
    curChannel++;

    displayFoundMac();
}
