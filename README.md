# ESP32 / ESP32-C3 Wall Clock with 60 WS2812B LED Ring

> Modern Wi-Fi synchronized wall clock powered by ESP32 and a 60-pixel WS2812B NeoPixel ring.

**Developed by Adrian Florescu (YO3HJV)**
**Under Creative Commons Attribution-NonCommercial 4.0 (CC BY-NC 4.0) License**

![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![Firmware](https://img.shields.io/badge/Firmware-v0.0.1-green)
![Framework](https://img.shields.io/badge/Framework-Arduino-orange)
![LEDs](https://img.shields.io/badge/LEDs-60%20x%20WS2812B-red)

## Features

- NTP time synchronization with automatic updates
- Smooth or discrete hand movement
- 1-LED or enhanced 3-LED hand visualization
- Fully responsive dark-mode web interface
- Ambient lamp mode with multiple effects
- Wi-Fi fallback access point
- mDNS support via `http://clock.local`
- Persistent configuration storage in NVS
- Low-power startup animation
- Software Limited Maximum Current (approx.320mA)

---

# 1. Project Overview

This project transforms a **60-LED WS2812B addressable LED ring** into a modern, precise, and highly configurable wall clock powered by an **ESP32-C3 SuperMini**, **ESP32-S3**, or classic **ESP32**.

The clock automatically synchronizes with NTP servers, supports smooth sub-pixel interpolation, offers multiple display styles, and includes a complete web-based configuration portal accessible from phones, tablets, and desktop computers.

Compilation is done with Arduino IDE with ESP32 support.
---

# 2. Hardware Specifications & Wiring

## Main Components

| Component | Description |
|------------|------------|
| Microcontroller | ESP32-C3 SuperMini (160 MHz RISC-V, 4 MB Flash) or Wemos D1 Mini ESP32|
| Display | 60 x WS2812B RGB Addressable LEDs | https://www.vanallesenmeer.nl/NeoPixel-RGB-Ring-60x-WS2812-5050-Rgb-Led
| Data Pin | GPIO4 (ESP32-C3/S3) or GPIO26 (ESP32 D1 Mini) |
| Power Supply | 5V DC, minimum 2A |

## Wiring Diagram

```text
               +-----------------------------+
               |      5V Power Supply        |
               +--------------+--------------+
                       +5V    |    GND
                        |     |
        +---------------+     +---------------+
        |                                     |
        v (+5V)                               v (GND)
+---------------+                     +---------------+
|   LED Ring    |                     |   ESP32-C3    |
| WS2812B (60)  |                     |  SuperMini    |
|               |                     |               |
|  +5V <--------+                     | 5V (Vin) <----+
|  GND <------------------------------+ GND <---------+
|  DIN <----[330-470Ω Resistor]-------+ IO4 (DATA)    |
+---------------+                     +---------------+
```

> **Surge Protection**
>
> FastLED power limiting is configured to **350 mA @ 5V** to avoid brownout resets during startup.

---

# 3. Key Features

## A. NTP Synchronization & Time Management

- Automatic synchronization at startup
- Scheduled synchronization every 12 hours (03:00 and 15:00)
- Manual synchronization from the web interface
- Support for up to 3 configurable NTP servers
- Timezone and DST support
- Smooth slew correction during resynchronization

Default NTP servers:

```text
pool.ntp.org
time.google.com
time.cloudflare.com
```

---

## B. Look & Feel Customization

### Display Modes

- One LED per hand
- Enhanced 3-LED hand mode
  - Bright center LED
  - Two guard LEDs
  - Independent brightness controls

### Smooth Transitions

- Guaranteed 3-LED continuity
- Adjustable cutoff threshold (1-20%, default 5%)
- Smooth intensity migration between LEDs

### Motion Modes

Each hand can operate independently:

- Smooth interpolation
- Discrete stepping

### Overlap Rendering Modes

1. Additive RGB blending
2. Hour > Minute > Second
3. Minute > Second > Hour
4. Second > Minute > Hour

### Calibration

- Clockwise or counter-clockwise rotation
- Physical offset adjustment (-30 to +30 LEDs)

### Night Mode

Automatic brightness reduction during configurable hours.

### Color Presets

- Classic (White / Red / Blue)
- Ice (Cyan / Blue)
- Sunset (Orange / Red)
- Neon (Magenta / Green)

---

## C. Ambient Lamp Mode

Available modes:

1. Static Color
2. Warm White (~2700K)
3. Cold White (~6000K)
4. Rainbow Flow
5. Rainbow Breathe
6. Candle Flicker

Includes adjustable brightness and animation speed.

---

## D. Wi-Fi Connectivity & Fallback Mode

### Wi-Fi Profiles

Supports three prioritized Wi-Fi profiles.

### Automatic Fallback Access Point

```text
SSID: Clock
Password: None
IP: 192.168.4.1
```

### mDNS Access

```text
http://clock.local
```

### Network Scanning

- One-click SSID selection
- Nearby network discovery from the web portal

---

## E. Persistence & Low-Power Startup

- All settings stored in ESP32 NVS memory
- Wi-Fi credentials preserved
- Display configuration preserved
- Lightweight startup effect with four random stars

---

# 4. Software Architecture

| File | Description |
|------|-------------|
| `esp_ceas_perete.ino` | Main entry point and non-blocking execution loop |
| `setup.h` | Hardware definitions and defaults |
| `config_manager.h/.cpp` | NVS configuration storage |
| `wifi_manager.h/.cpp` | Wi-Fi FSM, AP fallback, mDNS |
| `ntp_manager.h/.cpp` | NTP synchronization and slew correction |
| `led_clock.h/.cpp` | FastLED rendering engine and lamp mode |
| `web_portal.h/.cpp` | Embedded web interface |
| `FSD_Clock_ESP32_WS2812B.txt` | Functional specification document |

---

# 5. Installation & Arduino IDE Setup

## Step 1: Install Required Libraries

Install:

### FastLED

Recommended version:

```text
3.9.13
```

Compatibility define:

```cpp
#define FASTLED_RMT_BUILTIN_DRIVER 1
```

---

## Step 2: Board Configuration (ESP32-C3 SuperMini)

| Setting | Value |
|----------|----------|
| Board | ESP32C3 Dev Module |
| CPU Frequency | 160MHz (WiFi) |
| USB CDC On Boot | Enabled |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | Huge APP (3MB No OTA / 1MB SPIFFS) |
| Upload Mode | UART0 / Hardware CDC |
| JTAG Adapter | Integrated USB JTAG |

For ESP32-S3 use:

```text
ESP32S3 Dev Module
```

---

## Step 3: First Startup

1. Upload firmware.
2. Connect to Wi-Fi network **Clock**.
3. Open:

```text
http://192.168.4.1
```

or

```text
http://clock.local
```

4. Configure Wi-Fi.
5. Save settings.
6. Device reconnects automatically.
7. NTP synchronization starts.
8. Clock begins normal operation.

---

# License

Developed by **Adrian Florescu (YO3HJV)** 
Free for electronics enthusiasts, amateur radio operators, makers, and IoT developers.

The source code is provided free for personal and educational use.
