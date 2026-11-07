/*
*
*Ceas de perete cu ESP32, sincro NTP si afisare cu LEDuri adresabile WS2812 5050 NeoPixel 
* Autor: Adrian Florescu YO3JHV
*
*ATENTIE: Se va instala biblioteca FastLED 3.9.13 datorita unei incompatibilitati  cu Parlio pentru ESP32 C3.
*/

#define FW version 0.0.2   // Unica sursa de adevar pentru versiune. Se modifica doar de mine.

#include <Arduino.h>
#include "setup.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "ntp_manager.h"
#include "led_clock.h"
#include "web_portal.h"

void setup() {
    Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
    delay(1000); // Permite portului USB CDC nativ pe ESP32-S3 sa se enumereze pe PC
#else
    delay(300);
#endif

#if DEBUG_POST
    Serial.println("\n\n========================================");
    Serial.println("   CEAS PERETE ESP32 WS2812B FASTLED   ");
    Serial.println("========================================");
    Serial.printf("[BOOT] Initializare sistem...\n");
#endif

    // 1. Incarcare configuratii din NVS (Preferences)
    configInit();

    // 2. Initializare afisaj LED-uri FastLED pe GPIO definit in setup.h & pornire animatie boot
    ledClockInit();

    // 3. Initializare subsistem Wi-Fi (Client cu 3 profile sau fallback AP 'Clock')
    wifiInit();

    // 4. Initializare NTP si fus orar (EET/EEST Romania)
    ntpInit();

    // 5. Initializare Web Server pe portul 80
    webPortalInit();

#if DEBUG_POST
    Serial.println("[BOOT] Initializare finalizata cu succes.");
    Serial.println("========================================\n");
#endif
}

void loop() {
    // 1. Gestiune neblocanta conexiune Wi-Fi (Client & SoftAP)
    wifiLoop();

    // 2. Gestiune timp NTP, verificare interval 12h (03:00 / 15:00) si corectie slew
    ntpLoop();

    // 3. Randare neblocanta indicatoare si animatii LED (limitat la ~60 FPS)
    ledClockLoop();

    // 4. Procesare cereri HTTP Web Server
    webPortalLoop();
}
