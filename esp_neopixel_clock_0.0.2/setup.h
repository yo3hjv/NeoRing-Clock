#pragma once
#include <Arduino.h>

// ============================================================================
// CEAS DE PERETE CIRCULAR CU ESP32 D1 MINI & 60 LED-URI WS2812B (FastLED)
// ============================================================================

// ----------------------------------------------------------------------------
// DEBUG FLAGS (1 = activ pe Serial 115200, 0 = inactiv / silentios)
// ----------------------------------------------------------------------------
#define DEBUG_POST            1   // Mesaje autotest si initializare la boot (POST)
#define DEBUG_WIFI            1   // Diagnoza conexiune Wi-Fi Client si SoftAP
#define DEBUG_NTP             1   // Diagnoza sincronizare NTP, fus orar si slew
#define DEBUG_LEDS            0   // Diagnoza calcul pozitii LED-uri si randare
#define DEBUG_WEB             1   // Diagnoza cereri Web Server si rutare

// ----------------------------------------------------------------------------
// HARDWARE DEFINITIONS (ESP32-S3 SuperMini)
// ----------------------------------------------------------------------------
// Pin DATA pentru inelul de 60 LED-uri WS2812B
// NOTA ESP32-S3 SuperMini: GPIO 4 este un pin curat, sigur si comod pe latura modulului.
// Pe ESP32-S3, GPIO 26 este conectat intern la memoria Flash/PSRAM si NU trebuie folosit ca GPIO extern!
#define PIN_LED_DATA          4
#define NUM_LEDS              60
#define LED_CHIPSET           WS2812B
#define LED_COLOR_ORDER       GRB

// ----------------------------------------------------------------------------
// PROTECTIE SUPRACONSUM / SURGE PROTECTION (FastLED Power Limiter)
// ----------------------------------------------------------------------------
#define ENABLE_SURGE_PROTECTION   true   // true = limitare automata consum; false = dezactivat
#define SURGE_MAX_VOLTS           5      // Tensiune alimentare inel (V)
#define SURGE_MAX_MILLIAMPS       350    // Consum maxim permis global pe inel (mA)

// ----------------------------------------------------------------------------
// CONFIGURARE CADRAN & ORIENTARE
// ----------------------------------------------------------------------------
#define CLOCK_LED_TOP_INDEX   0    // Indexul LED-ului de la ora 12:00 (implicit 0)
#define CLOCK_IS_CLOCKWISE    true // true = sens orar (0 -> 1 -> ... -> 59)

// ----------------------------------------------------------------------------
// CULORI SI LUMINOZITATI IMPLICITE
// Ora alb (30%), Minut rosu (20%), Secunda albastru (10%)
// ----------------------------------------------------------------------------
#define DEFAULT_HOUR_COLOR       0xFFFFFF  // Alb
#define DEFAULT_HOUR_BRIGHTNESS  30        // 30% (~76 / 255)
#define DEFAULT_HOUR_WIDTH_3     false     // true = 3 LED-uri (centru + garda); false = 1 LED simplu
#define DEFAULT_HOUR_GUARD_COLOR 0x888888  // Culoare garda Ora (gri deschis / alb temperat)
#define DEFAULT_HOUR_GUARD_BRIGHT 15       // 15% luminozitate garda Ora

#define DEFAULT_MIN_COLOR        0xFF0000  // Rosu
#define DEFAULT_MIN_BRIGHTNESS   20        // 20% (~51 / 255)
#define DEFAULT_MIN_WIDTH_3      false     // true = 3 LED-uri (centru + garda); false = 1 LED simplu
#define DEFAULT_MIN_GUARD_COLOR  0x880000  // Culoare garda Minut (rosu temperat)
#define DEFAULT_MIN_GUARD_BRIGHT 10        // 10% luminozitate garda Minut

#define DEFAULT_SEC_COLOR        0x0000FF  // Albastru
#define DEFAULT_SEC_BRIGHTNESS   10        // 10% (~25 / 255)

#define DEFAULT_GLOBAL_BRIGHT    100       // Luminozitate generala (0 - 100%)
#define DEFAULT_SHOW_SECONDS     true      // Secundar activat implicit
#define DEFAULT_SMOOTH_HOUR      true      // Ore: miscare fluida / interpolata
#define DEFAULT_SMOOTH_MIN       true      // Minute: miscare fluida / interpolata
#define DEFAULT_SMOOTH_SEC       false     // Secunde: salt discret (false) sau fluid (true)
#define DEFAULT_GUARD_CUTOFF_PCT 5         // Prag minim stingere garda (5% din absolut) la tranzitie fluida pe 3 LED-uri
// Moduri tratare suprapunere culori indicatoare (de la prioritar la mascat):
#define OVERLAP_MODE_ADDITIVE       0  // Amestec Aditiv RGB (culorile se combina)
#define OVERLAP_MODE_HOUR_MIN_SEC   1  // Prioritate: Ora > Minute > Secunde
#define OVERLAP_MODE_HOUR_SEC_MIN   2  // Prioritate: Ora > Secunde > Minute
#define OVERLAP_MODE_MIN_SEC_HOUR   3  // Prioritate: Minute > Secunde > Ore
#define OVERLAP_MODE_SEC_MIN_HOUR   4  // Prioritate: Secunde > Minute > Ore

#define DEFAULT_OVERLAP_MODE        OVERLAP_MODE_ADDITIVE

// ----------------------------------------------------------------------------
// MOD LAMPA (ILUMINAT AMBIENTAL)
// ----------------------------------------------------------------------------
#define DEFAULT_LAMP_ENABLED     false     // false = Mod Ceas; true = Mod Lampa
#define DEFAULT_LAMP_EFFECT      0         // 0=Static, 1=Warm White, 2=Cold White, 3=Rainbow Flow, 4=Rainbow Breathe, 5=Candle
#define DEFAULT_LAMP_COLOR       0xFFA040  // Culoare ambientala calda implicita
#define DEFAULT_LAMP_BRIGHTNESS  40        // Luminozitate lampa (0 - 100%)
#define DEFAULT_LAMP_SPEED       30        // Viteza animatie (1 - 100)

// ----------------------------------------------------------------------------
// ACCESS POINT IMPLICIT (FALLBACK CAND NU EXISTA WI-FI)
// ----------------------------------------------------------------------------
#define DEFAULT_AP_SSID          "Clock"
#define DEFAULT_AP_PASS          ""        // Fara parola (Open AP)
#define DEFAULT_AP_IP            IPAddress(192, 168, 4, 1)
#define DEFAULT_AP_GATEWAY       IPAddress(192, 168, 4, 1)
#define DEFAULT_AP_SUBNET        IPAddress(255, 255, 255, 0)

// Nume gazda mDNS (accesibil in reteaua locala la http://clock.local)
#define MDNS_HOSTNAME            "clock"

// ----------------------------------------------------------------------------
// SINCRONIZARE NTP SI TIMP
// ----------------------------------------------------------------------------
#define DEFAULT_NTP_SERVER1      "pool.ntp.org"
#define DEFAULT_NTP_SERVER2      "time.google.com"
#define DEFAULT_NTP_SERVER3      "time.cloudflare.com"

// Fus orar implicit (Human-Readable): Romania UTC+2, cu posibilitate de DST (+1 ora)
#define DEFAULT_TZ_HOURS         2         // Diferenta in ore fata de UTC (-12 .. +14)
#define DEFAULT_TZ_MINUTES       0         // Diferenta in minute (0, 15, 30, 45)
#define DEFAULT_DST_ACTIVE       true      // true = Ora de vara (+1h activa)

// Sincronizare la fiecare 12 ore: la 03:00 AM si 03:00 PM (15:00)
#define NTP_SYNC_HOUR_AM         3
#define NTP_SYNC_HOUR_PM         15

// Durata de aplicare graduala a corectiei de timp (slew), in secunde
#define DEFAULT_SLEW_SECONDS     60

