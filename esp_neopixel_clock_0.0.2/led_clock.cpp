#include "led_clock.h"
#include "ntp_manager.h"
#include "wifi_manager.h"

static CRGB leds[NUM_LEDS];
static LedClockState currentState = LED_STATE_BOOT_ANIM;

static unsigned long animStartTime = 0;
static const unsigned long BOOT_ANIM_DURATION_MS = 2500;
static const unsigned long CONNECTED_ANIM_DURATION_MS = 1500;
static const unsigned long ERROR_ANIM_DURATION_MS = 2000;

static int mapPhysicalIndex(int logicalIndex) {
    int idx = logicalIndex + config.dialOffset;
    if (!config.clockwise) {
        idx = (NUM_LEDS - idx);
    }
    idx = (idx % NUM_LEDS + NUM_LEDS) % NUM_LEDS;
    return idx;
}

static CRGB hexToCRGB(uint32_t hexColor) {
    uint8_t r = (hexColor >> 16) & 0xFF;
    uint8_t g = (hexColor >> 8) & 0xFF;
    uint8_t b = hexColor & 0xFF;
    return CRGB(r, g, b);
}

static uint8_t getActiveBrightnessScale() {
    uint8_t bright = config.globalBright;

    // Verificare Mod Nocturn
    if (config.nightModeEnabled && ntpIsTimeSynced()) {
        int hour = ntpGetCurrentHour();
        bool isNight = false;
        if (config.nightStartHour > config.nightEndHour) {
            // Interval peste miezul noptii (ex: 23:00 - 07:00)
            isNight = (hour >= config.nightStartHour || hour < config.nightEndHour);
        } else {
            // Interval in aceeasi zi (ex: 01:00 - 06:00)
            isNight = (hour >= config.nightStartHour && hour < config.nightEndHour);
        }

        if (isNight) {
            bright = (uint8_t)((bright * config.nightBright) / 100);
        }
    }
    return bright;
}

static void applyIndicatorColor(float pos, CRGB baseColor, uint8_t indBrightness, uint8_t globalScale, bool smooth, bool additive) {
    // Calcul intensitate finala scalata (0 - 255)
    uint16_t effBright = (uint16_t)indBrightness * globalScale; // 0 .. 10000
    uint8_t scale = (uint8_t)(effBright * 255 / 10000);

    CRGB scaledColor = baseColor;
    scaledColor.nscale8(scale);

    if (smooth) {
        int i0 = (int)floor(pos);
        float frac = pos - (float)i0;
        int i1 = (i0 + 1) % NUM_LEDS;

        int phys0 = mapPhysicalIndex(i0);
        int phys1 = mapPhysicalIndex(i1);

        uint8_t weight0 = (uint8_t)((1.0f - frac) * 255.0f);
        uint8_t weight1 = (uint8_t)(frac * 255.0f);

        CRGB c0 = scaledColor;
        c0.nscale8(weight0);

        CRGB c1 = scaledColor;
        c1.nscale8(weight1);

        if (additive) {
            leds[phys0] += c0;
            leds[phys1] += c1;
        } else {
            leds[phys0] = c0;
            leds[phys1] = c1;
        }
    } else {
        int idx = (int)round(pos) % NUM_LEDS;
        int phys = mapPhysicalIndex(idx);
        if (additive) {
            leds[phys] += scaledColor;
        } else {
            leds[phys] = scaledColor;
        }
    }
}

// Randare indicator pe exact 3 LED-uri (Centru + 2 de garda), garantat mereu 3 LED-uri aprinse
static void renderIndicator3Led(float pos, CRGB centerColor, uint8_t centerBright, 
                                CRGB guardColor, uint8_t guardBright, 
                                uint8_t globalScale, bool smooth, bool additive) {
    uint16_t effCenter = (uint16_t)centerBright * globalScale;
    uint8_t scaleCenter = (uint8_t)(effCenter * 255 / 10000);

    uint16_t effGuard = (uint16_t)guardBright * globalScale;
    uint8_t scaleGuard = (uint8_t)(effGuard * 255 / 10000);

    // Prag minim de stingere/aprindere ferma a garzii (default 5% din absolut)
    uint8_t cutoffPct = config.guardCutoffPct;
    if (cutoffPct < 1) cutoffPct = 1;
    if (cutoffPct > 25) cutoffPct = 25;
    uint8_t scaleCutoff = (uint8_t)((uint16_t)cutoffPct * globalScale * 255 / 10000);
    if (scaleCutoff > scaleGuard) scaleCutoff = scaleGuard;
    if (scaleCutoff < 1) scaleCutoff = 1;

    int idxL, idxC, idxR;
    CRGB cL, cC, cR;

    if (smooth) {
        int k = (int)floor(pos);
        float u = pos - (float)k; // interval [0.0, 1.0)
        k = (k % NUM_LEDS + NUM_LEDS) % NUM_LEDS;

        uint8_t midScale = (scaleCenter + scaleGuard) / 2;

        if (u < 0.5f) {
            // Faza 1: [k-1, k, k+1] sunt cele 3 LED-uri active
            idxL = (k - 1 + NUM_LEDS) % NUM_LEDS; // trailing (din urma)
            idxC = k;                             // centru curent
            idxR = (k + 1) % NUM_LEDS;             // leading (din fata)

            float w = u * 2.0f; // [0.0, 1.0)
            uint8_t bL = scaleGuard - (uint8_t)(w * (float)(scaleGuard - scaleCutoff));
            uint8_t bC = scaleCenter - (uint8_t)(w * (float)(scaleCenter - midScale));
            uint8_t bR = scaleGuard  + (uint8_t)(w * (float)(midScale - scaleGuard));

            cL = guardColor;
            cL.nscale8(bL);

            cC = blend(centerColor, guardColor, (uint8_t)(w * 128.0f));
            cC.nscale8(bC);

            cR = blend(guardColor, centerColor, (uint8_t)(w * 128.0f));
            cR.nscale8(bR);
        } else {
            // Faza 2: LED-ul k-1 a atins pragul si se stinge ferm; se aprinde ferm k+2 direct la pragul de cutoff
            // Fereastra devine strict [k, k+1, k+2]
            idxL = k;                             // fostul centru devine garda stanga
            idxC = (k + 1) % NUM_LEDS;             // noul centru
            idxR = (k + 2) % NUM_LEDS;             // noul LED din fata

            float w = (u - 0.5f) * 2.0f; // [0.0, 1.0)
            uint8_t bL = midScale    - (uint8_t)(w * (float)(midScale - scaleGuard));
            uint8_t bC = midScale    + (uint8_t)(w * (float)(scaleCenter - midScale));
            uint8_t bR = scaleCutoff + (uint8_t)(w * (float)(scaleGuard - scaleCutoff));

            cL = blend(centerColor, guardColor, (uint8_t)(128.0f + w * 127.0f));
            cL.nscale8(bL);

            cC = blend(guardColor, centerColor, (uint8_t)(128.0f + w * 127.0f));
            cC.nscale8(bC);

            cR = guardColor;
            cR.nscale8(bR);
        }
    } else {
        // Mod discret (salt pe 3 pozitii intregi fixe)
        idxC = (int)round(pos) % NUM_LEDS;
        idxC = (idxC + NUM_LEDS) % NUM_LEDS;
        idxL = (idxC - 1 + NUM_LEDS) % NUM_LEDS;
        idxR = (idxC + 1) % NUM_LEDS;

        cL = guardColor;  cL.nscale8(scaleGuard);
        cC = centerColor; cC.nscale8(scaleCenter);
        cR = guardColor;  cR.nscale8(scaleGuard);
    }

    int physL = mapPhysicalIndex(idxL);
    int physC = mapPhysicalIndex(idxC);
    int physR = mapPhysicalIndex(idxR);

    if (additive) {
        leds[physL] += cL;
        leds[physC] += cC;
        leds[physR] += cR;
    } else {
        leds[physL] = cL;
        leds[physC] = cC;
        leds[physR] = cR;
    }
}

void ledClockInit() {
    FastLED.addLeds<LED_CHIPSET, PIN_LED_DATA, LED_COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setCorrection(TypicalLEDStrip);

#if ENABLE_SURGE_PROTECTION
    FastLED.setMaxPowerInVoltsAndMilliamps(SURGE_MAX_VOLTS, SURGE_MAX_MILLIAMPS);
#endif

    FastLED.clear(); // Fara clear(true) la rece pentru a evita spike-ul de curent

    currentState = LED_STATE_BOOT_ANIM;
    animStartTime = millis();

#if DEBUG_POST
    Serial.printf("[LED] FastLED initializat pe GPIO%d cu %d LED-uri.\n", PIN_LED_DATA, NUM_LEDS);
#if ENABLE_SURGE_PROTECTION
    Serial.printf("[LED] Protectie supraconsum ACTIVA (max %d mA la %d V).\n", SURGE_MAX_MILLIAMPS, SURGE_MAX_VOLTS);
#else
    Serial.println("[LED] Protectie supraconsum DEZACTIVATA.");
#endif
#endif
}

void ledClockTriggerConnectedAnim() {
    currentState = LED_STATE_CONNECTED_ANIM;
    animStartTime = millis();
}

void ledClockTriggerErrorAnim() {
    currentState = LED_STATE_ERROR_ANIM;
    animStartTime = millis();
}

// Numara cate LED-uri sunt vizibil aprinse in acest moment
static int countLitLeds() {
    int count = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (leds[i].getLuma() > 10) count++;
    }
    return count;
}

// Joc de lumini la boot: maxim 4 stelute aleatorii de culori diferite, consum ultra-redus
static void renderBootAnimation(unsigned long elapsed) {
    // Stingere lina a pixelilor existenti
    fadeToBlackBy(leds, NUM_LEDS, 20);

    // Daca sunt mai putin de 4 stelute aprinse, adaugam o noua steluta intr-o pozitie aleatorie
    if (countLitLeds() < 4 && random8(100) < 35) {
        int idx = random8(NUM_LEDS);
        // Culoare aleatorie la intensitate redusa (~15-25% din 255)
        uint8_t hue = random8();
        uint8_t bright = random8(35, 60);
        leds[idx] = CHSV(hue, 220, bright);
    }
}

// Joc de lumini la conectare Wi-Fi: 4 stelute verzi/smarald/cyan in pozitii aleatorii
static void renderConnectedAnimation(unsigned long elapsed) {
    fadeToBlackBy(leds, NUM_LEDS, 25);

    if (countLitLeds() < 4 && random8(100) < 40) {
        int idx = random8(NUM_LEDS);
        // Nuanțe verzi / smarald / cyan (hue 90 .. 140)
        uint8_t hue = random8(90, 140);
        uint8_t bright = random8(40, 70);
        leds[idx] = CHSV(hue, 230, bright);
    }
}

static void renderErrorAnimation(unsigned long elapsed) {
    FastLED.clear();
    float progress = (float)elapsed / (float)ERROR_ANIM_DURATION_MS;
    uint8_t val = (uint8_t)(sin8((uint8_t)(progress * 255)));
    // Pulsare portocaliu / rosu la pozitiile 12, 3, 6, 9
    for (int i = 0; i < NUM_LEDS; i += 15) {
        leds[mapPhysicalIndex(i)] = CRGB(val, val / 4, 0);
    }
}

static void renderLampMode() {
    FastLED.clear();
    uint8_t bright = (uint8_t)((config.lampBrightness * 255) / 100);

    switch (config.lampEffect) {
        case 0: { // Culoare statica personalizata
            CRGB c = hexToCRGB(config.lampColor);
            c.nscale8(bright);
            for (int i = 0; i < NUM_LEDS; i++) leds[i] = c;
            break;
        }
        case 1: { // Warm White (~2700K lumina ambientala calda)
            CRGB c(255, 175, 55);
            c.nscale8(bright);
            for (int i = 0; i < NUM_LEDS; i++) leds[i] = c;
            break;
        }
        case 2: { // Cold White (~6000K lumina rece)
            CRGB c(235, 245, 255);
            c.nscale8(bright);
            for (int i = 0; i < NUM_LEDS; i++) leds[i] = c;
            break;
        }
        case 3: { // Rainbow Flow (spectru rotativ continuu)
            static uint16_t flowHue16 = 0;
            flowHue16 += (uint16_t)(config.lampSpeed * 12);
            uint8_t startHue = flowHue16 >> 8;
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[mapPhysicalIndex(i)] = CHSV(startHue + (i * 256 / NUM_LEDS), 240, bright);
            }
            break;
        }
        case 4: { // Rainbow Breathe (pulsare armonioasa)
            static uint16_t breatheHue16 = 0;
            breatheHue16 += (uint16_t)(config.lampSpeed * 5);
            uint8_t hue = breatheHue16 >> 8;
            uint8_t pulse = beatsin8((uint8_t)(config.lampSpeed / 2 + 5), (uint8_t)(bright / 4), bright);
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = CHSV(hue, 220, pulse);
            }
            break;
        }
        case 5: { // Candle Flicker (licarire realista de lumanare)
            for (int i = 0; i < NUM_LEDS; i++) {
                uint8_t flicker = random8(35);
                uint8_t b = (bright > flicker) ? (bright - flicker) : 5;
                leds[i] = CRGB(b, (uint8_t)(b * 0.58f), (uint8_t)(b * 0.12f));
            }
            break;
        }
        default:
            break;
    }
}

static void drawHourHand(float hourPos, uint8_t globalScale, bool additive) {
    CRGB colHour = hexToCRGB(config.colorHour);
    if (config.hourWidth3) {
        CRGB colHourG = hexToCRGB(config.colorHourGuard);
        renderIndicator3Led(hourPos, colHour, config.brightHour, colHourG, config.brightHourGuard,
                            globalScale, config.smoothHour, additive);
    } else {
        applyIndicatorColor(hourPos, colHour, config.brightHour, globalScale,
                            config.smoothHour, additive);
    }
}

static void drawMinHand(float minPos, uint8_t globalScale, bool additive) {
    CRGB colMin = hexToCRGB(config.colorMin);
    if (config.minWidth3) {
        CRGB colMinG = hexToCRGB(config.colorMinGuard);
        renderIndicator3Led(minPos, colMin, config.brightMin, colMinG, config.brightMinGuard,
                            globalScale, config.smoothMin, additive);
    } else {
        applyIndicatorColor(minPos, colMin, config.brightMin, globalScale,
                            config.smoothMin, additive);
    }
}

static void drawSecHand(float secPos, uint8_t globalScale, bool additive) {
    if (!config.showSeconds) return;
    CRGB colSec = hexToCRGB(config.colorSec);
    applyIndicatorColor(secPos, colSec, config.brightSec, globalScale,
                        config.smoothSec, additive);
}

static void renderClock() {
    FastLED.clear();
    uint8_t globalScale = getActiveBrightnessScale();

    if (!ntpIsTimeSynced()) {
        // Daca nu s-a sincronizat inca timpul, afisam o pulsare blanda la ora 12:00
        uint8_t pulse = beatsin8(30, 20, 140);
        leds[mapPhysicalIndex(0)] = CRGB(pulse, pulse / 2, 0);
        return;
    }

    float hourPos = 0, minPos = 0, secPos = 0;
    ntpGetClockPositions(hourPos, minPos, secPos, config.smoothHour, config.smoothMin, config.smoothSec);

    bool additive = (config.overlapMode == OVERLAP_MODE_ADDITIVE);

    // Ordinea randarii: elementul prioritar se deseneaza ultimul (pentru a suprascrie / ramane deasupra)
    switch (config.overlapMode) {
        case OVERLAP_MODE_HOUR_SEC_MIN:
            // Prioritate: Ora > Secunde > Minute (Ora pe deasupra, Minutul dedesubt)
            drawMinHand(minPos, globalScale, additive);
            drawSecHand(secPos, globalScale, additive);
            drawHourHand(hourPos, globalScale, additive);
            break;

        case OVERLAP_MODE_MIN_SEC_HOUR:
            // Prioritate: Minute > Secunde > Ore (Minutul pe deasupra, Ora dedesubt)
            drawHourHand(hourPos, globalScale, additive);
            drawSecHand(secPos, globalScale, additive);
            drawMinHand(minPos, globalScale, additive);
            break;

        case OVERLAP_MODE_SEC_MIN_HOUR:
            // Prioritate: Secunde > Minute > Ore (Secundarul pe deasupra, Ora dedesubt)
            drawHourHand(hourPos, globalScale, additive);
            drawMinHand(minPos, globalScale, additive);
            drawSecHand(secPos, globalScale, additive);
            break;

        case OVERLAP_MODE_HOUR_MIN_SEC:
        case OVERLAP_MODE_ADDITIVE:
        default:
            // Prioritate: Ora > Minute > Secunde (Ora pe deasupra, Secunda dedesubt)
            // (La aditiv culorile se cumuleaza comutativ)
            drawSecHand(secPos, globalScale, additive);
            drawMinHand(minPos, globalScale, additive);
            drawHourHand(hourPos, globalScale, additive);
            break;
    }
}

void ledClockLoop() {
    static unsigned long lastFrameTime = 0;
    unsigned long now = millis();

    // Limitare frame rate la ~60 FPS (o randare la 16 ms)
    if (now - lastFrameTime < 16) {
        return;
    }
    lastFrameTime = now;

    switch (currentState) {
        case LED_STATE_BOOT_ANIM:
            if (now - animStartTime < BOOT_ANIM_DURATION_MS) {
                renderBootAnimation(now - animStartTime);
            } else {
                currentState = LED_STATE_NORMAL_CLOCK;
            }
            break;

        case LED_STATE_CONNECTED_ANIM:
            if (now - animStartTime < CONNECTED_ANIM_DURATION_MS) {
                renderConnectedAnimation(now - animStartTime);
            } else {
                currentState = LED_STATE_NORMAL_CLOCK;
            }
            break;

        case LED_STATE_ERROR_ANIM:
            if (now - animStartTime < ERROR_ANIM_DURATION_MS) {
                renderErrorAnimation(now - animStartTime);
            } else {
                currentState = LED_STATE_NORMAL_CLOCK;
            }
            break;

        case LED_STATE_NORMAL_CLOCK:
        default:
            if (config.lampEnabled) {
                renderLampMode();
            } else {
                renderClock();
            }
            break;
    }

    FastLED.show();
}

void ledClockGetRingColors(uint32_t outColors[NUM_LEDS]) {
    for (int i = 0; i < NUM_LEDS; i++) {
        outColors[i] = ((uint32_t)leds[i].r << 16) | ((uint32_t)leds[i].g << 8) | (uint32_t)leds[i].b;
    }
}

