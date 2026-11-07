#pragma once
#include <Arduino.h>
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(__riscv)
// Pe arhitectura RISC-V (ESP32-C3), RMT built-in driver previne crash-ul de vectorizare a intreruperilor
#define FASTLED_RMT_BUILTIN_DRIVER 1
#endif

#define FASTLED_INTERNAL
#include <FastLED.h>
#include "setup.h"
#include "config_manager.h"

enum LedClockState {
    LED_STATE_BOOT_ANIM,
    LED_STATE_CONNECTED_ANIM,
    LED_STATE_NORMAL_CLOCK,
    LED_STATE_ERROR_ANIM
};

void ledClockInit();
void ledClockLoop();

void ledClockTriggerConnectedAnim();
void ledClockTriggerErrorAnim();

// Getter pentru previzualizarea SVG din pagina web (vector de 60 culori hex)
void ledClockGetRingColors(uint32_t outColors[NUM_LEDS]);

