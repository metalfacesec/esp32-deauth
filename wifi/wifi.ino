#include "esp_wifi.h"
#include <vector>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

using namespace std;

#define MAX_CHANNELS 11
#define MAX_MACS_ON_SCREEN 5
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_LEFT_GPIO 15
#define BUTTON_RIGHT_GPIO 2


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int lastLeftButtonRead = 0;
int lastRightButtonRead = 0;
int curChannel = 1;
int currentPage = 1;
long lastPageChange = 0;
vector<String> macArray;

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

    macAttack.toUpperCase();

    // Prevent duplicates
    for (int i = 0; i < macArray.size(); i++) {
        if (macAttack == macArray[i]) {
            return;
        }
    }

    macArray.push_back(macAttack);   
}

int getMaxPages() {
    int maxPages = macArray.size() / MAX_MACS_ON_SCREEN;
    if (macArray.size() % MAX_MACS_ON_SCREEN > 0) {
        maxPages++;
    }
    return maxPages;
}

void setWifiPromiscuousMode() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(&sniffer);
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
}

void setupDisplay() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
  
    display.setTextSize(1);
    display.setTextColor(WHITE);
}

void setup() {
    Serial.begin(115200);
  
    setWifiPromiscuousMode();
    setupDisplay();
    displayFoundMac();
}

void changeWifiChannel() {
    if(curChannel > MAX_CHANNELS){ 
        curChannel = 1;
    }
    
    esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
    delay(100);
    curChannel++;
}

void displayFoundMac() {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    delay(100);
    display.clearDisplay();

    display.setCursor(0, 6);
    display.println("Active Clients");
    display.setCursor(90, 6);
    display.println(String(currentPage) + "/" + String(getMaxPages()));

    int row = 0;
    int startIndex = (currentPage - 1) * MAX_MACS_ON_SCREEN;
    int endIndex = startIndex + MAX_MACS_ON_SCREEN;
    for (int i = startIndex; i < endIndex; i++) {
        if (i < macArray.size()) {
            display.setCursor(0, (row * 9) + 16);
            display.println(macArray[i]);
            row++;
        }
    }
    display.display();
}

boolean wasButtonReleased(bool isRight, int reading) {
    if (lastRightButtonRead == 1 && reading == 0 && isRight) {
        return true;
    } else if (lastLeftButtonRead == 1 && reading == 0 && !isRight) {
        return true;
    }
    return false;
}

boolean hasMoreMACPages() {
   return currentPage < getMaxPages();
}

boolean hasLowerMACPages() {
   return currentPage > 1;
}

void loop() {
    changeWifiChannel();

    int leftButtonRead = digitalRead(BUTTON_LEFT_GPIO);
    int rightButtonRead = digitalRead(BUTTON_RIGHT_GPIO);

    if (wasButtonReleased(true, rightButtonRead) && hasMoreMACPages()) {
        currentPage++;
    } else if (wasButtonReleased(false, leftButtonRead) && hasLowerMACPages()) {
        currentPage--;
    }
    
    lastRightButtonRead = rightButtonRead;
    lastLeftButtonRead = leftButtonRead;

    displayFoundMac();
}
