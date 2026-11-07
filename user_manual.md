# DIY Smart NeoPixel Wall Clock (ESP32 & 60 WS2812B LED Ring)
### *A Beginner-Friendly, Step-by-Step Guide (Instructables Style)*

---

## Overview

Welcome to the build guide for the **Smart NeoPixel Wall Clock**! In this project, you will build a stunning, ultra-precise wall or desk clock using a 60-LED WS2812B addressable ring and a compact ESP32-C3 SuperMini (or ESP32-S3 / classic ESP32) microcontroller.

### Why this clock is special:
- **Never needs time setting:** Automatically synchronizes with atomic time servers over Wi-Fi (NTP).
- **Smooth analog-like motion:** LED hands slide fluidly around the circle, or tick discretely second-by-second.
- **Wide 3-LED hands:** Hour and Minute hands can be styled as a bright center LED flanked by dimmer "guard" LEDs, maintaining a strict 3-LED active window at all times.
- **Zero apps needed:** Built-in web dashboard accessible from your smartphone, tablet, or PC browser (`http://clock.local`).
- **Dual personality:** Flip a switch in the web portal, and your clock transforms into a soothing **Ambient Mood Lamp** (Warm White, Rainbow Flow, Candle Flicker, etc.).

---

## Step 1: What You Need (Bill of Materials)

| Component | Quantity | Notes |
| :--- | :---: | :--- |
| **ESP32-C3 SuperMini** (or ESP32-S3 / D1 Mini) | 1 | Microcontroller with built-in Wi-Fi and USB-C |
| **WS2812B 60-LED Circular Ring** | 1 | 5050 RGB LEDs arranged in a circle (outer diameter ~15–17 cm) |
| **5V Power Supply (≥ 2A)** | 1 | Standard 5V USB charger or dedicated 5V power adapter |
| **330Ω to 470Ω Resistor** | 1 | Protects the LED ring DATA line from voltage spikes |
| **1000 µF, 6.3V–16V Capacitor** (Optional but recommended) | 1 | Buffers power supply surges across +5V and GND |
| **Hookup Wires & Soldering Iron** | — | 22–24 AWG flexible wires |

---

## Step 2: Wiring & Assembly

Connecting the hardware is straightforward. The most important rule in addressable LED projects is **Common Ground** and **Parallel Power Feed**.

### Wiring Schematic

```text
               +------------------------------------+
               |     5V DC Power Source (≥ 2A)      |
               +-----------------+------------------+
                        +5V (Red)|    GND (Black)
                                 |
         +-----------------------+-----------------------+
         |                                               |
         v (+5V)                                         v (GND)
 +-----------------------+                       +-----------------------+
 |   WS2812B LED Ring    |                       |   ESP32-C3 SuperMini  |
 |       (60 LEDs)       |                       |                       |
 |                       |                       |                       |
 |   +5V (VCC) <---------+                       |   5V (Vin) <----------+
 |   GND <---------------+-----------------------+   GND <---------------+ (Common Ground!)
 |                       |                       |                       |
 |   DIN (Data In) <-----+---[ 330Ω Resistor ]---+   GPIO 4 (DATA OUT)   |
 +-----------------------+                       +-----------------------+
```

### Pro Assembly Tips:
1. **Always tie grounds together:** The GND of the ESP32 and the GND of the LED ring **must** be connected.
2. **Data Resistor:** Solder a 330Ω–470Ω resistor inline on the wire connected to **DIN** (Data In). Keep the wire between the ESP32 and DIN relatively short (< 20 cm / 8 inches) to avoid signal distortion.
3. **Data Pin Selection:**
   - **ESP32-C3 SuperMini / S3:** Use **GPIO 4**. *(Do not use GPIO 26 on S3/C3 as it is reserved internally for flash memory).*
   - **ESP32 D1 Mini (Classic):** Can use GPIO 4 or GPIO 26.
4. **Built-in Power Safety:** The clock firmware comes with a software safety limit (**350 mA / 5V**), meaning you can safely test the clock directly from your computer's USB port without overloading it!

---

## Step 3: Uploading the Firmware

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) (version 2.x recommended).
2. Add ESP32 board support:
   - Go to **File → Preferences**, and add this URL to *Additional Boards Manager URLs*:  
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Open **Tools → Board → Boards Manager**, search for `esp32` by Espressif, and install it.
3. Install the **FastLED** library:
   - Open **Sketch → Include Library → Manage Libraries...**
   - Search for **FastLED** and install version **3.9.13** (recommended for proven stability).
4. Select your board under **Tools → Board → esp32**:
   - For ESP32-C3 SuperMini: select **ESP32C3 Dev Module**.
   - Under **Tools**:
     - *USB CDC On Boot*: **Enabled** (essential for USB Serial output).
     - *Flash Size*: **4MB**.
5. Connect your ESP32 with a USB-C data cable, select your **COM Port**, and click **Upload**.

---

## Step 4: First Boot & Connecting to Your Wi-Fi

Once flashed, the clock starts up and guides you through an effortless setup:

1. **Fallback Hotspot:** On its very first start (or whenever your home router is out of reach), the clock automatically broadcasts its own Wi-Fi network:
   - **Wi-Fi Name (SSID):** `Clock`
   - **Password:** *(None / Open Network)*
2. **Open the Portal:**
   - Connect your phone or computer to the `Clock` Wi-Fi network.
   - Open your web browser and go to:  
     👉 **`http://192.168.4.1`** (or **`http://clock.local`**)
3. **Connect to Your Home Wi-Fi:**
   - Tap the **Wi-Fi** tab in the navigation bar.
   - Click **Scan Networks** to see nearby routers.
   - Click on your home network name, enter your password, and click **Save & Reconnect**.
4. The clock will connect to your router, automatically pull atomic time via NTP, and display the live time! From now on, you can open the dashboard at any time by visiting **`http://clock.local`** from any device on your home network.

---

## Step 5: How to Use the Web Dashboard

The web portal is designed with a sleek Dark Mode theme and divided into clear tabs:

```
[ Dashboard ]  [ Lamp ]  [ Look & Feel ]  [ Clock / NTP ]  [ Wi-Fi ]  [ System ]
```

### 1. Dashboard (`/`)
- **Live SVG Clock:** An interactive preview that mirrors the actual 60 LEDs in real-time right on your phone screen.
- **Status at a Glance:** Live digital time, calendar date, NTP synchronization state, IP address, and Wi-Fi signal strength (RSSI).
- **Sync NTP Now:** One-click button to force an instant time refresh.

### 2. Look & Feel (`/leds`) — Customize Your Clock Face
- **Hour, Minute, & Second Indicators:**
  - Choose individual colors using an intuitive color picker.
  - Set distinct brightness levels for each hand (1% to 100%).
- **3-LED Mode (Center + Guard):**
  - Enable **3 LEDs** for the Hour and/or Minute hands.
  - The center LED acts as the primary pointer, while the two flanking LEDs emit a softer guard color of your choice.
  - **Strict 3-LED Fluid Motion:** As time advances, exactly 3 physical LEDs remain illuminated. The configurable **Cutoff Threshold** (default: 5%) guarantees that as the trailing LED fades, it turns off firmly while the incoming lead LED turns on firmly—no floating gaps!
- **Smooth Transition vs. Discrete Tick:**
  - Smooth ON: Hands slide seamlessly like an analog timepiece.
  - Smooth OFF: Hands tick crisply to the next LED index.
- **Color Overlap Priority (When hands meet on the dial):**
  - `Additive RGB Blending`: Colors combine dynamically (e.g., Red Minute + Blue Second = Magenta).
  - `Priority: Hour > Minute > Second`: The Hour hand stays visibly on top, with seconds hidden underneath.
  - `Priority: Hour > Second > Minute`: The Hour hand stays on top, seconds in the middle, minutes underneath.
  - `Priority: Minute > Second > Hour`: Minute hand takes topmost visual priority.
  - `Priority: Second > Minute > Hour`: The Second hand takes topmost visual priority.
- **Dial Orientation & Mechanical Alignment:**
  - **Dial Offset (-30 to +30 LEDs):** Adjust this slider to align the 12:00 mark with your physical casing without re-soldering.
  - **Rotation Direction:** Switch between Clockwise and Counter-clockwise depending on how your ring is mounted.
- **Night Mode:**
  - Set custom start and end hours (e.g., 23:00 to 07:00) and a dim nighttime brightness (e.g., 5%) so your clock never disturbs your sleep.
- **Quick Presets:** One-click themes: *Classic (White/Red/Blue)*, *Ice (Cyan/Blue)*, *Sunset (Orange/Red)*, and *Neon (Magenta/Green)*.

### 3. Ambient Lamp Mode (`/lamp`)
Turn your wall clock into a stylish room accent light:
- Check **Enable Lamp Mode** (replaces clock hands with full-ring illumination).
- Choose from 6 stunning lighting styles:
  1. **Custom Static Color:** Any color you pick from the palette.
  2. **Warm White (~2700K):** Cozy, golden evening light.
  3. **Cold White (~6000K):** Crisp daylight illumination.
  4. **Rainbow Flow:** A smooth, rotating spectrum around the circle.
  5. **Rainbow Breathe:** A gentle, harmoniously pulsing color bloom.
  6. **Candle Flicker:** Realistic, cozy fireplace/candle simulation.
- Adjust brightness and animation speed dynamically.

### 4. Clock / NTP Settings (`/clock`)
- Set your **Timezone Offset** from UTC (-12 to +14 hours, plus minutes).
- Check the **Daylight Saving Time (DST)** box to add +1 hour during summer.
- Configure primary, secondary, and tertiary NTP servers.
- **Gradual Time Correction (Slew):** If a clock sync detects a minor deviation, it gently glides into the exact time over a smooth slew window (default: 60s) rather than jolting abruptly.

### 5. Wi-Fi Settings (`/wifi`)
- Store up to **3 different Wi-Fi networks** with priority ranking (1 = highest). If network #1 drops, the clock seamlessly shifts to network #2.

### 6. System Settings (`/system`)
- View firmware version, free memory (RAM), and system uptime.
- **Reboot Device:** Restart without unplugging.
- **Reset to Factory Defaults:** Safely wipe NVS storage back to out-of-the-box defaults.

---

## Step 6: Troubleshooting & FAQ

#### Q: The LEDs flicker randomly or glow weird colors!
- **A:** Make sure you have connected the **GND** of the ESP32 and the **GND** of the LED ring together. Without a shared ground, the high-speed data signal has no reference voltage. Also, ensure you have the 330Ω–470Ω resistor on the DIN line.

#### Q: The clock restarts when all LEDs turn bright white.
- **A:** This is a *brownout reset* caused by insufficient current. White color draws the most current. Use a good quality 5V 2A power brick, or lower the **Global Brightness** setting in the Look & Feel tab.

#### Q: I moved the clock to a new house/network and it cannot connect.
- **A:** If the clock cannot find any of its saved Wi-Fi networks, it automatically starts its fallback access point named **`Clock`**. Simply connect to it from your phone, go to `http://192.168.4.1`, and enter your new router credentials!

#### Q: What if my 12:00 position is rotated 90 degrees?
- **A:** No need to unscrew or re-solder anything! Just go to **Look & Feel** in the web portal, adjust the **Dial Offset** number, and click Save. Each step shifts the 12 o'clock position by one LED.

---

*Enjoy your handmade Smart NeoPixel Wall Clock!*

