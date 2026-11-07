#pragma once
#include <Arduino.h>
#include "setup.h"

struct WifiProfile {
    char ssid[33];
    char pass[65];
    bool enabled;
    uint8_t priority; // 1 = prioritate maxima, 2, 3
};

struct AppConfig {
    // 3 Profile Wi-Fi
    WifiProfile wifi[3];

    // Culori indicatoare (0x00RRGGBB)
    uint32_t colorHour;
    uint8_t  brightHour;  // 0 - 100%
    bool     hourWidth3;  // true = 3 LED-uri (centru + 2 de garda); false = 1 LED
    uint32_t colorHourGuard;
    uint8_t  brightHourGuard;

    uint32_t colorMin;
    uint8_t  brightMin;   // 0 - 100%
    bool     minWidth3;   // true = 3 LED-uri (centru + 2 de garda); false = 1 LED
    uint32_t colorMinGuard;
    uint8_t  brightMinGuard;

    uint32_t colorSec;
    uint8_t  brightSec;   // 0 - 100%

    uint8_t  globalBright;// 0 - 100%
    bool     showSeconds; // Afisare secundar activ/inactiv
    bool     smoothHour;  // true = fluid/interpolat, false = salt discret
    bool     smoothMin;   // true = fluid/interpolat, false = salt discret
    bool     smoothSec;   // true = fluid/interpolat, false = salt discret
    uint8_t  overlapMode;     // 0=Aditiv, 1=Ora>Min>Sec, 2=Min>Sec>Ora, 3=Sec>Min>Ora
    uint8_t  guardCutoffPct;  // Prag minim stingere garda (1 - 20%, default 5% din absolut)

    // Mod Lampa (Iluminat ambiental)
    bool     lampEnabled;
    uint8_t  lampEffect;     // 0=Static, 1=Warm White, 2=Cold White, 3=Rainbow Flow, 4=Rainbow Breathe, 5=Candle
    uint32_t lampColor;      // Culoare HEX
    uint8_t  lampBrightness; // 0 - 100%
    uint8_t  lampSpeed;      // 1 - 100 viteza animatie

    // Cadran
    int8_t   dialOffset;  // Offset in numar de LED-uri (-30 la +30)
    bool     clockwise;   // true = sens orar

    // Mod Nocturn (Night Mode)
    bool     nightModeEnabled;
    uint8_t  nightStartHour;  // ex: 23
    uint8_t  nightEndHour;    // ex: 7
    uint8_t  nightBright;     // luminozitate redusa noaptea (0 - 100%)

    // NTP & Fus Orar (Human-readable)
    char     ntpServer1[64];
    char     ntpServer2[64];
    char     ntpServer3[64];
    int8_t   tzHours;         // Offset ore fata de UTC (-12 .. +14)
    int8_t   tzMinutes;       // Offset minute (0, 15, 30, 45)
    bool     dstActive;       // Ora de vara activa (+1 ora)
    uint16_t slewDurationSec; // Secunde pentru corectie graduala
};

extern AppConfig config;

void configInit();
void configLoad();
void configSave();
void configResetDefaults();

