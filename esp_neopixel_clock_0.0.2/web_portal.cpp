#include "web_portal.h"
#include "wifi_manager.h"
#include "ntp_manager.h"
#include "led_clock.h"

static WebServer server(80);

static String colorToHexStr(uint32_t c) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%06X", c & 0xFFFFFF);
    return String(buf);
}

static uint32_t hexStrToColor(const String& str) {
    String s = str;
    if (s.startsWith("#")) s = s.substring(1);
    return (uint32_t)strtoul(s.c_str(), nullptr, 16);
}

// Redirectionare sigura compatibila cu toate browserele (HTTP 302 + Meta Refresh + JS Replace)
static void sendRedirect(const String& path) {
    server.sendHeader("Location", path, true);
    String body = "<!DOCTYPE html><html><head>";
    body += "<meta charset='UTF-8'>";
    body += "<meta http-equiv='refresh' content='0;url=" + path + "'>";
    body += "</head><body style='background:#181818;color:#00e5ff;font-family:Arial,sans-serif;padding:25px;text-align:center;'>";
    body += "<script>window.location.replace('" + path + "');</script>";
    body += "<p>Settings saved successfully. Redirecting to <a href='" + path + "' style='color:#00bcd4;'>" + path + "</a>...</p>";
    body += "</body></html>";
    server.send(302, "text/html", body);
}

// ============================================================================
// STIL CSS COMUN (DARK THEME - SIMILAR CU BAROCLOCK)
// ============================================================================
static String getPageHeader(const String& title, const String& activeTab) {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>WS2812B Clock - " + title + "</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;background:#181818;color:#eee;margin:0;padding:12px;}";
    html += ".container{max-width:600px;margin:0 auto;}";
    html += ".card{background:#262626;padding:16px;border-radius:8px;margin-bottom:14px;box-shadow:0 3px 8px rgba(0,0,0,0.5);}";
    html += "h2,h3{color:#00e5ff;margin-top:0;}";
    html += "label{display:block;margin:10px 0 4px;font-size:0.9em;color:#bbb;}";
    html += "input[type='text'],input[type='password'],input[type='number'],select{width:100%;padding:9px;border-radius:4px;border:1px solid #555;background:#333;color:#fff;box-sizing:border-box;}";
    html += "input[type='color']{width:100%;height:40px;border-radius:4px;border:1px solid #555;background:#333;cursor:pointer;box-sizing:border-box;}";
    html += "input[type='range']{width:100%;accent-color:#00bcd4;margin:8px 0;}";
    html += ".btn{background:#00bcd4;color:#000;border:none;padding:11px 15px;font-weight:bold;border-radius:4px;cursor:pointer;margin-top:10px;width:100%;text-align:center;text-decoration:none;display:inline-block;box-sizing:border-box;font-size:1em;}";
    html += ".btn:hover{background:#00acc1;}";
    html += ".btn-sec{background:#444;color:#fff;border:1px solid #666;margin-top:6px;}";
    html += ".btn-sec:hover{background:#555;}";
    html += ".btn-danger{background:#d32f2f;color:#fff;margin-top:6px;}";
    html += ".nav{display:flex;flex-wrap:wrap;gap:6px;margin-bottom:14px;}";
    html += ".nav a{flex:1;min-width:70px;text-align:center;padding:10px 4px;background:#333;color:#aaa;text-decoration:none;border-radius:4px;font-size:0.85em;font-weight:bold;}";
    html += ".nav a.active{background:#00bcd4;color:#000;}";
    html += ".grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px;}";
    html += ".grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;}";
    html += ".stat-box{background:#1f1f1f;padding:10px;border-radius:6px;border-left:3px solid #00bcd4;margin-bottom:8px;}";
    html += ".stat-title{font-size:0.8em;color:#888;}";
    html += ".stat-val{font-size:1.15em;font-weight:bold;color:#fff;margin-top:2px;}";
    html += ".clock-prev{display:flex;justify-content:center;margin:15px 0;}";
    html += ".badge{padding:3px 7px;border-radius:3px;font-size:0.8em;font-weight:bold;}";
    html += ".bg-green{background:#2e7d32;color:#fff;} .bg-amber{background:#f57f17;color:#fff;}";
    html += "</style>";
    html += "</head><body><div class='container'>";
    html += "<h2>WS2812B ESP32 Clock</h2>";

    // Bara navigare tab-uri
    html += "<div class='nav'>";
    html += "<a href='/' class='" + String(activeTab == "dash" ? "active" : "") + "'>Dashboard</a>";
    html += "<a href='/lamp' class='" + String(activeTab == "lamp" ? "active" : "") + "'>Lamp</a>";
    html += "<a href='/leds' class='" + String(activeTab == "leds" ? "active" : "") + "'>Look &amp; Feel</a>";
    html += "<a href='/clock' class='" + String(activeTab == "clock" ? "active" : "") + "'>Clock / NTP</a>";
    html += "<a href='/wifi' class='" + String(activeTab == "wifi" ? "active" : "") + "'>Wi-Fi</a>";
    html += "<a href='/system' class='" + String(activeTab == "sys" ? "active" : "") + "'>System</a>";
    html += "</div>";

    return html;
}

static String getPageFooter() {
    return "</div></body></html>";
}

// ============================================================================
// 1. DASHBOARD
// ============================================================================
static void handleDashboard() {
    String html = getPageHeader("Dashboard", "dash");

    html += "<div class='card'>";
    html += "<h3>Current Time &amp; Dial</h3>";
    html += "<div class='grid2'>";
    html += "<div class='stat-box'><div class='stat-title'>Current Time</div><div class='stat-val' id='clockTime'>" + ntpGetFormattedTime() + "</div></div>";
    html += "<div class='stat-box'><div class='stat-title'>Date</div><div class='stat-val' id='clockDate'>" + ntpGetFormattedDate() + "</div></div>";
    html += "</div>";

    // Previzualizare interactiva cadran circular cu SVG
    html += "<div class='clock-prev'>";
    html += "<svg id='clockRing' width='220' height='220' viewBox='-110 -110 220 220'>";
    html += "<circle cx='0' cy='0' r='100' fill='#151515' stroke='#333' stroke-width='2'/>";
    html += "<circle cx='0' cy='0' r='3' fill='#555'/>";
    
    // Punctele celor 60 LED-uri
    for (int i = 0; i < NUM_LEDS; i++) {
        float angle = (i * 6.0f - 90.0f) * 0.0174532925f; // Radiani, 0 = ora 12:00 sus
        float cx = 86.0f * cos(angle);
        float cy = 86.0f * sin(angle);
        html += "<circle id='led" + String(i) + "' cx='" + String(cx, 1) + "' cy='" + String(cy, 1) + "' r='3.5' fill='#282828'/>";
    }
    html += "</svg>";
    html += "</div>";

    html += "<div class='stat-box'><div class='stat-title'>NTP Sync Status</div><div class='stat-val'>";
    if (ntpIsTimeSynced()) {
        html += "<span class='badge bg-green'>Synchronized</span>";
    } else {
        html += "<span class='badge bg-amber'>Waiting</span>";
    }
    html += "</div></div>";

    html += "<div class='grid2'>";
    html += "<div class='stat-box'><div class='stat-title'>Last Sync</div><div class='stat-val' style='font-size:0.9em;'>" + ntpGetLastSyncTime() + "</div></div>";
    html += "<div class='stat-box'><div class='stat-title'>Next Scheduled</div><div class='stat-val' style='font-size:0.9em;'>" + ntpGetNextSyncTime() + "</div></div>";
    html += "</div>";

    html += "</div>";

    html += "<div class='card'>";
    html += "<h3>Connectivity Status</h3>";
    html += "<div class='stat-box'><div class='stat-title'>Operating Mode</div><div class='stat-val'>";
    if (wifiIsConnected()) {
        html += "Wi-Fi Client (<span style='color:#00e5ff;'>" + wifiGetSSID() + "</span>)";
    } else {
        html += "Access Point (<span style='color:#ff9800;'>" + String(DEFAULT_AP_SSID) + "</span>)";
    }
    html += "</div></div>";

    html += "<div class='grid2'>";
    html += "<div class='stat-box'><div class='stat-title'>IP Address / mDNS</div><div class='stat-val'>" + wifiGetIP() + "<br><span style='font-size:0.75em;color:#00bcd4;'>clock.local</span></div></div>";
    html += "<div class='stat-box'><div class='stat-title'>Signal Strength (RSSI)</div><div class='stat-val'>" + (wifiIsConnected() ? String(wifiGetRSSI()) + " dBm" : "-") + "</div></div>";
    html += "</div>";

    html += "<form method='POST' action='/sync_now'><button class='btn' type='submit'>Sync NTP Now</button></form>";
    html += "</div>";

    // Script AJAX pentru actualizare timp si LED-uri in timp real pe cadranul SVG
    html += "<script>";
    html += "function updateClock(){";
    html += "  fetch('/api/status').then(r=>r.json()).then(d=>{";
    html += "    document.getElementById('clockTime').innerText = d.time;";
    html += "    document.getElementById('clockDate').innerText = d.date;";
    html += "  });";
    html += "  fetch('/api/leds').then(r=>r.json()).then(colors=>{";
    html += "    for(let i=0; i<60; i++){";
    html += "      let el = document.getElementById('led'+i);";
    html += "      if(el) el.setAttribute('fill', colors[i]);";
    html += "    }";
    html += "  });";
    html += "}";
    html += "setInterval(updateClock, 1000);";
    html += "updateClock();";
    html += "</script>";

    html += getPageFooter();
    server.send(200, "text/html", html);
}

// ============================================================================
// 2. CONFIGURARE WI-FI
// ============================================================================
static void handleWifiPage() {
    String html = getPageHeader("Wi-Fi Settings", "wifi");

    html += "<div class='card'>";
    html += "<h3>Saved Wi-Fi Profiles</h3>";
    html += "<form method='POST' action='/save_wifi'>";

    for (int i = 0; i < 3; i++) {
        html += "<div style='border:1px solid #444;padding:12px;border-radius:6px;margin-bottom:12px;'>";
        html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
        html += "<strong style='color:#00bcd4;'>Profile " + String(i + 1) + "</strong>";
        html += "<label style='margin:0;'><input type='checkbox' name='en_" + String(i) + "' value='1' " + (config.wifi[i].enabled ? "checked" : "") + "> Enabled</label>";
        html += "</div>";

        html += "<label>Network SSID:</label>";
        html += "<input type='text' name='ssid_" + String(i) + "' value='" + String(config.wifi[i].ssid) + "' maxlength='32'>";

        html += "<label>Wi-Fi Password:</label>";
        html += "<input type='password' name='pass_" + String(i) + "' placeholder='(unchanged)' maxlength='64'>";

        html += "<label>Priority (1 = Highest):</label>";
        html += "<select name='prio_" + String(i) + "'>";
        for (int p = 1; p <= 3; p++) {
            html += "<option value='" + String(p) + "' " + (config.wifi[i].priority == p ? "selected" : "") + ">Priority " + String(p) + "</option>";
        }
        html += "</select>";
        html += "</div>";
    }

    html += "<button class='btn' type='submit'>Save &amp; Reconnect</button>";
    html += "</form>";
    html += "</div>";

    // Scan available networks
    html += "<div class='card'>";
    html += "<h3>Available Wi-Fi Networks (Scan)</h3>";
    html += "<button class='btn btn-sec' onclick='scanWifi()'>Scan Networks</button>";
    html += "<div id='scanResults' style='margin-top:10px;'></div>";
    html += "</div>";

    html += "<script>";
    html += "function scanWifi(){";
    html += "  let div = document.getElementById('scanResults');";
    html += "  div.innerHTML = '<p style=\"color:#888;\">Scanning networks...</p>';";
    html += "  fetch('/api/scan').then(r=>r.json()).then(data=>{";
    html += "    if(data.length == 0){ div.innerHTML = '<p>No networks found.</p>'; return; }";
    html += "    let out = '<table style=\"width:100%;border-collapse:collapse;\">';";
    html += "    out += '<tr style=\"border-bottom:1px solid #555;\"><th style=\"text-align:left;padding:6px;\">SSID</th><th>Signal</th></tr>';";
    html += "    data.forEach(n=>{";
    html += "      out += '<tr><td style=\"padding:6px;\"><a href=\"#\" onclick=\"setSSID(\\''+n.ssid+'\\');return false;\" style=\"color:#00e5ff;text-decoration:none;\">'+n.ssid+'</a></td><td style=\"text-align:center;\">'+n.rssi+' dBm</td></tr>';";
    html += "    });";
    html += "    out += '</table>';";
    html += "    div.innerHTML = out;";
    html += "  });";
    html += "}";
    html += "function setSSID(s){";
    html += "  let f = document.querySelector('input[name=\"ssid_0\"]');";
    html += "  if(f) f.value = s;";
    html += "}";
    html += "</script>";

    html += getPageFooter();
    server.send(200, "text/html", html);
}

static void handleSaveWifi() {
    for (int i = 0; i < 3; i++) {
        String ssidKey = "ssid_" + String(i);
        String passKey = "pass_" + String(i);
        String enKey   = "en_" + String(i);
        String prioKey = "prio_" + String(i);

        if (server.hasArg(ssidKey)) {
            strncpy(config.wifi[i].ssid, server.arg(ssidKey).c_str(), sizeof(config.wifi[i].ssid) - 1);
        }
        if (server.hasArg(passKey) && server.arg(passKey).length() > 0) {
            strncpy(config.wifi[i].pass, server.arg(passKey).c_str(), sizeof(config.wifi[i].pass) - 1);
        }
        config.wifi[i].enabled = (server.arg(enKey) == "1");
        if (server.hasArg(prioKey)) {
            config.wifi[i].priority = (uint8_t)server.arg(prioKey).toInt();
        }
    }

    configSave();
    wifiTriggerReconnect();

    sendRedirect("/wifi");
}

// ============================================================================
// 3. CONFIGURARE CEAS & NTP
// ============================================================================
static void handleClockPage() {
    String html = getPageHeader("Clock & NTP Settings", "clock");

    html += "<div class='card'>";
    html += "<h3>NTP Synchronization Settings</h3>";
    html += "<form method='POST' action='/save_clock'>";

    html += "<label>NTP Server 1:</label>";
    html += "<input type='text' name='ntp1' value='" + String(config.ntpServer1) + "' maxlength='63'>";

    html += "<label>NTP Server 2:</label>";
    html += "<input type='text' name='ntp2' value='" + String(config.ntpServer2) + "' maxlength='63'>";

    html += "<label>NTP Server 3:</label>";
    html += "<input type='text' name='ntp3' value='" + String(config.ntpServer3) + "' maxlength='63'>";

    html += "<label>Timezone Offset from UTC:</label>";
    html += "<div class='grid2'>";
    html += "<div><label style='font-size:0.8em;color:#aaa;'>Hours (+ / -):</label><select name='tz_h'>";
    for (int h = -12; h <= 14; h++) {
        String sign = (h > 0) ? "+" : "";
        html += "<option value='" + String(h) + "' " + (config.tzHours == h ? "selected" : "") + ">UTC " + sign + String(h) + " hours</option>";
    }
    html += "</select></div>";

    html += "<div><label style='font-size:0.8em;color:#aaa;'>Minutes:</label><select name='tz_m'>";
    html += "<option value='0' " + String(config.tzMinutes == 0 ? "selected" : "") + ">:00 min</option>";
    html += "<option value='15' " + String(config.tzMinutes == 15 ? "selected" : "") + ">:15 min</option>";
    html += "<option value='30' " + String(config.tzMinutes == 30 ? "selected" : "") + ">:30 min</option>";
    html += "<option value='45' " + String(config.tzMinutes == 45 ? "selected" : "") + ">:45 min</option>";
    html += "</select></div>";
    html += "</div>";

    html += "<label style='margin-top:10px;'><input type='checkbox' name='tz_dst' value='1' " + String(config.dstActive ? "checked" : "") + "> Daylight Saving Time (DST) active (+1 hour)</label>";
    html += "<div style='font-size:0.75em;color:#888;margin-top:2px;'>Check this box during summer time to advance clock by 1 hour.</div>";

    html += "<label>Gradual Time Correction (Slew) duration in seconds:</label>";
    html += "<input type='number' name='slew' min='0' max='300' value='" + String(config.slewDurationSec) + "'>";

    html += "<button class='btn' type='submit'>Save Settings</button>";
    html += "</form>";
    html += "</div>";

    html += getPageFooter();
    server.send(200, "text/html", html);
}

static void handleSaveClock() {
    if (server.hasArg("ntp1")) strncpy(config.ntpServer1, server.arg("ntp1").c_str(), sizeof(config.ntpServer1) - 1);
    if (server.hasArg("ntp2")) strncpy(config.ntpServer2, server.arg("ntp2").c_str(), sizeof(config.ntpServer2) - 1);
    if (server.hasArg("ntp3")) strncpy(config.ntpServer3, server.arg("ntp3").c_str(), sizeof(config.ntpServer3) - 1);
    
    if (server.hasArg("tz_h")) config.tzHours   = (int8_t)server.arg("tz_h").toInt();
    if (server.hasArg("tz_m")) config.tzMinutes = (int8_t)server.arg("tz_m").toInt();
    config.dstActive = (server.arg("tz_dst") == "1");

    if (server.hasArg("slew")) config.slewDurationSec = (uint16_t)server.arg("slew").toInt();

    configSave();
    ntpInit(); // Re-initialize with new timezone

    sendRedirect("/clock");
}

// ============================================================================
// 4. MOD LAMPA (ILUMINAT AMBIENTAL)
// ============================================================================
static void handleLampPage() {
    String html = getPageHeader("Ambient Lamp Mode", "lamp");

    html += "<div class='card'>";
    html += "<h3>Ambient Lighting (Lamp Mode)</h3>";
    html += "<p style='color:#bbb;font-size:0.9em;margin-top:0;'>Turn the LED ring into an ambient lamp or night light.</p>";

    html += "<form method='POST' action='/save_lamp'>";

    html += "<div style='background:#1f1f1f;padding:12px;border-radius:6px;margin-bottom:14px;border-left:3px solid #00bcd4;'>";
    html += "<label style='font-size:1.05em;color:#fff;margin:0;'><input type='checkbox' name='lp_en' value='1' " + String(config.lampEnabled ? "checked" : "") + "> <strong>Enable Lamp Mode</strong> (overrides clock display)</label>";
    html += "</div>";

    html += "<label>Lighting Effect:</label>";
    html += "<select name='lp_eff'>";
    html += "<option value='0' " + String(config.lampEffect == 0 ? "selected" : "") + ">Custom Static Color</option>";
    html += "<option value='1' " + String(config.lampEffect == 1 ? "selected" : "") + ">Warm White (~2700K)</option>";
    html += "<option value='2' " + String(config.lampEffect == 2 ? "selected" : "") + ">Cold White (~6000K)</option>";
    html += "<option value='3' " + String(config.lampEffect == 3 ? "selected" : "") + ">Rainbow Flow</option>";
    html += "<option value='4' " + String(config.lampEffect == 4 ? "selected" : "") + ">Rainbow Breathe</option>";
    html += "<option value='5' " + String(config.lampEffect == 5 ? "selected" : "") + ">Candle Flicker</option>";
    html += "</select>";

    html += "<div id='lampColorDiv' style='margin-top:10px;'>";
    html += "<label>Custom Color (for effect 0):</label>";
    html += "<input type='color' name='lp_col' value='" + colorToHexStr(config.lampColor) + "'>";
    html += "</div>";

    html += "<label>Lamp Brightness (" + String(config.lampBrightness) + "%):</label>";
    html += "<input type='range' name='lp_br' min='1' max='100' value='" + String(config.lampBrightness) + "'>";

    html += "<label>Animation Speed (Dynamic effects) (" + String(config.lampSpeed) + "%):</label>";
    html += "<input type='range' name='lp_spd' min='1' max='100' value='" + String(config.lampSpeed) + "'>";

    html += "<button class='btn' type='submit'>Save Lamp Settings</button>";
    html += "</form>";
    html += "</div>";

    // Quick presets
    html += "<div class='card'>";
    html += "<h3>Quick Lamp Presets</h3>";
    html += "<div class='grid2'>";
    html += "<button class='btn btn-sec' onclick='applyLampPreset(1,45,30)'>Warm White (45%)</button>";
    html += "<button class='btn btn-sec' onclick='applyLampPreset(5,35,40)'>Candle Flicker</button>";
    html += "<button class='btn btn-sec' onclick='applyLampPreset(3,50,40)'>Rainbow Party</button>";
    html += "<button class='btn btn-sec' onclick='applyLampPreset(0,25,10,\"#ff4081\")'>Pink Ambient (25%)</button>";
    html += "</div></div>";

    html += "<script>";
    html += "function applyLampPreset(eff,br,spd,col){";
    html += "  document.querySelector('select[name=\"lp_eff\"]').value = eff;";
    html += "  document.querySelector('input[name=\"lp_br\"]').value = br;";
    html += "  document.querySelector('input[name=\"lp_spd\"]').value = spd;";
    html += "  document.querySelector('input[name=\"lp_en\"]').checked = true;";
    html += "  if(col) document.querySelector('input[name=\"lp_col\"]').value = col;";
    html += "}";
    html += "</script>";

    html += getPageFooter();
    server.send(200, "text/html", html);
}

static void handleSaveLamp() {
    config.lampEnabled    = (server.arg("lp_en") == "1");
    if (server.hasArg("lp_eff")) config.lampEffect     = (uint8_t)server.arg("lp_eff").toInt();
    if (server.hasArg("lp_col")) config.lampColor      = hexStrToColor(server.arg("lp_col"));
    if (server.hasArg("lp_br"))  config.lampBrightness = (uint8_t)server.arg("lp_br").toInt();
    if (server.hasArg("lp_spd")) config.lampSpeed      = (uint8_t)server.arg("lp_spd").toInt();

    configSave();

    sendRedirect("/lamp");
}

// ============================================================================
// 5. PERSONALIZARE LOOK&FEEL (CULORI & TRANZITII)
// ============================================================================
static void handleLedsPage() {
    String html = getPageHeader("Look & Feel", "leds");

    html += "<div class='card'>";
    html += "<h3>Look &amp; Feel Settings (Colors &amp; Transitions)</h3>";
    html += "<form method='POST' action='/save_leds'>";

    // Hour Indicator
    html += "<div style='border-left:3px solid #fff;padding-left:10px;margin-bottom:14px;'>";
    html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
    html += "<strong style='color:#fff;'>HOUR Indicator</strong>";
    html += "<label style='margin:0;'><input type='checkbox' name='w3_h' value='1' " + String(config.hourWidth3 ? "checked" : "") + "> 3 LEDs (Center + Guard)</label>";
    html += "</div>";
    html += "<div class='grid2'>";
    html += "<div><label>Center Color:</label><input type='color' name='c_hour' value='" + colorToHexStr(config.colorHour) + "'></div>";
    html += "<div><label>Center Brightness (" + String(config.brightHour) + "%):</label><input type='range' name='b_hour' min='1' max='100' value='" + String(config.brightHour) + "'></div>";
    html += "</div>";
    html += "<div class='grid2' style='margin-top:6px;'>";
    html += "<div><label>Guard Color (Left/Right):</label><input type='color' name='cg_h' value='" + colorToHexStr(config.colorHourGuard) + "'></div>";
    html += "<div><label>Guard Brightness (" + String(config.brightHourGuard) + "%):</label><input type='range' name='bg_h' min='1' max='100' value='" + String(config.brightHourGuard) + "'></div>";
    html += "</div>";
    html += "<div style='margin-top:8px;'><label><input type='checkbox' name='sm_h' value='1' " + String(config.smoothHour ? "checked" : "") + "> <strong>Smooth HOUR transition</strong> (advances continuously with minutes)</label></div>";
    html += "</div>";

    // Minute Indicator
    html += "<div style='border-left:3px solid #f44336;padding-left:10px;margin-bottom:14px;'>";
    html += "<div style='display:flex;justify-content:space-between;align-items:center;'>";
    html += "<strong style='color:#f44336;'>MINUTE Indicator</strong>";
    html += "<label style='margin:0;'><input type='checkbox' name='w3_m' value='1' " + String(config.minWidth3 ? "checked" : "") + "> 3 LEDs (Center + Guard)</label>";
    html += "</div>";
    html += "<div class='grid2'>";
    html += "<div><label>Center Color:</label><input type='color' name='c_min' value='" + colorToHexStr(config.colorMin) + "'></div>";
    html += "<div><label>Center Brightness (" + String(config.brightMin) + "%):</label><input type='range' name='b_min' min='1' max='100' value='" + String(config.brightMin) + "'></div>";
    html += "</div>";
    html += "<div class='grid2' style='margin-top:6px;'>";
    html += "<div><label>Guard Color (Left/Right):</label><input type='color' name='cg_m' value='" + colorToHexStr(config.colorMinGuard) + "'></div>";
    html += "<div><label>Guard Brightness (" + String(config.brightMinGuard) + "%):</label><input type='range' name='bg_m' min='1' max='100' value='" + String(config.brightMinGuard) + "'></div>";
    html += "</div>";
    html += "<div style='margin-top:8px;'><label><input type='checkbox' name='sm_m' value='1' " + String(config.smoothMin ? "checked" : "") + "> <strong>Smooth MINUTE transition</strong> (smooth slide between minutes)</label></div>";
    html += "</div>";

    // Second Indicator
    html += "<div style='border-left:3px solid #2196f3;padding-left:10px;margin-bottom:14px;'>";
    html += "<strong style='color:#2196f3;'>SECOND Indicator</strong>";
    html += "<div class='grid2'>";
    html += "<div><label>Color:</label><input type='color' name='c_sec' value='" + colorToHexStr(config.colorSec) + "'></div>";
    html += "<div><label>Brightness (" + String(config.brightSec) + "%):</label><input type='range' name='b_sec' min='1' max='100' value='" + String(config.brightSec) + "'></div>";
    html += "</div>";
    html += "<div style='margin-top:8px;'><label><input type='checkbox' name='sm_s' value='1' " + String(config.smoothSec ? "checked" : "") + "> <strong>Smooth SECOND hand</strong> (unchecked = 1s discrete tick; checked = smooth slide)</label></div>";
    html += "</div>";

    // Global Brightness
    html += "<label>Global Brightness (" + String(config.globalBright) + "%):</label>";
    html += "<input type='range' name='b_glob' min='1' max='100' value='" + String(config.globalBright) + "'>";

    // Dial Behavior
    html += "<h3 style='margin-top:20px;'>Dial Behavior</h3>";

    html += "<label>Second Hand Display:</label>";
    html += "<select name='sec_en'>";
    html += "<option value='1' " + String(config.showSeconds ? "selected" : "") + ">Enabled</option>";
    html += "<option value='0' " + String(!config.showSeconds ? "selected" : "") + ">Disabled (No second hand)</option>";
    html += "</select>";

    html += "<label>Color Overlap Blending Mode (Priority order):</label>";
    html += "<select name='overlap'>";
    html += "<option value='0' " + String(config.overlapMode == OVERLAP_MODE_ADDITIVE ? "selected" : "") + ">Additive RGB Blending (Colors mix)</option>";
    html += "<option value='1' " + String(config.overlapMode == OVERLAP_MODE_HOUR_MIN_SEC ? "selected" : "") + ">Priority: Hour &gt; Minute &gt; Second</option>";
    html += "<option value='2' " + String(config.overlapMode == OVERLAP_MODE_HOUR_SEC_MIN ? "selected" : "") + ">Priority: Hour &gt; Second &gt; Minute</option>";
    html += "<option value='3' " + String(config.overlapMode == OVERLAP_MODE_MIN_SEC_HOUR ? "selected" : "") + ">Priority: Minute &gt; Second &gt; Hour</option>";
    html += "<option value='4' " + String(config.overlapMode == OVERLAP_MODE_SEC_MIN_HOUR ? "selected" : "") + ">Priority: Second &gt; Minute &gt; Hour</option>";
    html += "</select>";

    html += "<div class='grid2'>";
    html += "<div><label>Dial Offset (LEDs):</label><input type='number' name='offset' min='-30' max='30' value='" + String(config.dialOffset) + "'></div>";
    html += "<div><label>Rotation Direction:</label><select name='cw'><option value='1' " + String(config.clockwise ? "selected" : "") + ">Clockwise</option><option value='0' " + String(!config.clockwise ? "selected" : "") + ">Counterclockwise</option></select></div>";
    html += "</div>";

    html += "<label style='margin-top:10px;'>3-LED Guard Cutoff Threshold (%):</label>";
    html += "<input type='number' name='g_cut' min='1' max='20' value='" + String(config.guardCutoffPct) + "'>";
    html += "<div style='font-size:0.75em;color:#888;margin-top:2px;'>Default 5%. In smooth transition, when the trailing guard falls below this threshold, it snaps off and the leading guard snaps on, strictly maintaining 3 lit LEDs.</div>";

    // Night Mode
    html += "<h3 style='margin-top:20px;'>Night Mode</h3>";
    html += "<label><input type='checkbox' name='nm_en' value='1' " + String(config.nightModeEnabled ? "checked" : "") + "> Enable Night Mode</label>";
    html += "<div class='grid3'>";
    html += "<div><label>Start Hour:</label><input type='number' name='nm_sh' min='0' max='23' value='" + String(config.nightStartHour) + "'></div>";
    html += "<div><label>End Hour:</label><input type='number' name='nm_eh' min='0' max='23' value='" + String(config.nightEndHour) + "'></div>";
    html += "<div><label>Night Brightness (%):</label><input type='number' name='nm_br' min='1' max='50' value='" + String(config.nightBright) + "'></div>";
    html += "</div>";

    html += "<button class='btn' type='submit'>Save Look &amp; Feel Settings</button>";
    html += "</form>";
    html += "</div>";

    // Quick color presets
    html += "<div class='card'>";
    html += "<h3>Quick Color Presets</h3>";
    html += "<div class='grid2'>";
    html += "<button class='btn btn-sec' onclick='applyPreset(\"#ffffff\",30,\"#ff0000\",20,\"#0000ff\",10)'>Classic (White / Red / Blue)</button>";
    html += "<button class='btn btn-sec' onclick='applyPreset(\"#00ffff\",30,\"#0080ff\",20,\"#0000ff\",10)'>Ice (Cyan / Blue)</button>";
    html += "<button class='btn btn-sec' onclick='applyPreset(\"#ff9800\",30,\"#f44336\",20,\"#ffeb3b\",10)'>Sunset (Orange / Red)</button>";
    html += "<button class='btn btn-sec' onclick='applyPreset(\"#e91e63\",30,\"#00e676\",20,\"#00e5ff\",10)'>Neon (Magenta / Green)</button>";
    html += "</div>";
    html += "</div>";

    html += "<script>";
    html += "function applyPreset(ch,bh,cm,bm,cs,bs){";
    html += "  document.querySelector('input[name=\"c_hour\"]').value = ch;";
    html += "  document.querySelector('input[name=\"b_hour\"]').value = bh;";
    html += "  document.querySelector('input[name=\"c_min\"]').value = cm;";
    html += "  document.querySelector('input[name=\"b_min\"]').value = bm;";
    html += "  document.querySelector('input[name=\"c_sec\"]').value = cs;";
    html += "  document.querySelector('input[name=\"b_sec\"]').value = bs;";
    html += "}";
    html += "</script>";

    html += getPageFooter();
    server.send(200, "text/html", html);
}

static void handleSaveLeds() {
    config.hourWidth3 = (server.arg("w3_h") == "1");
    if (server.hasArg("c_hour")) config.colorHour        = hexStrToColor(server.arg("c_hour"));
    if (server.hasArg("b_hour")) config.brightHour       = (uint8_t)server.arg("b_hour").toInt();
    if (server.hasArg("cg_h"))   config.colorHourGuard   = hexStrToColor(server.arg("cg_h"));
    if (server.hasArg("bg_h"))   config.brightHourGuard  = (uint8_t)server.arg("bg_h").toInt();

    config.minWidth3 = (server.arg("w3_m") == "1");
    if (server.hasArg("c_min"))  config.colorMin         = hexStrToColor(server.arg("c_min"));
    if (server.hasArg("b_min"))  config.brightMin        = (uint8_t)server.arg("b_min").toInt();
    if (server.hasArg("cg_m"))   config.colorMinGuard    = hexStrToColor(server.arg("cg_m"));
    if (server.hasArg("bg_m"))   config.brightMinGuard   = (uint8_t)server.arg("bg_m").toInt();

    if (server.hasArg("c_sec"))  config.colorSec   = hexStrToColor(server.arg("c_sec"));
    if (server.hasArg("b_sec"))  config.brightSec  = (uint8_t)server.arg("b_sec").toInt();

    if (server.hasArg("b_glob")) config.globalBright = (uint8_t)server.arg("b_glob").toInt();

    // Smooth transition settings (Look&Feel)
    config.smoothHour = (server.arg("sm_h") == "1");
    config.smoothMin  = (server.arg("sm_m") == "1");
    config.smoothSec  = (server.arg("sm_s") == "1");

    // Dial Behavior
    if (server.hasArg("sec_en"))  config.showSeconds    = (server.arg("sec_en") == "1");
    if (server.hasArg("overlap")) config.overlapMode    = (uint8_t)server.arg("overlap").toInt();
    if (server.hasArg("offset"))  config.dialOffset     = (int8_t)server.arg("offset").toInt();
    if (server.hasArg("cw"))      config.clockwise      = (server.arg("cw") == "1");
    if (server.hasArg("g_cut"))   config.guardCutoffPct = (uint8_t)server.arg("g_cut").toInt();

    config.nightModeEnabled = (server.arg("nm_en") == "1");
    if (server.hasArg("nm_sh")) config.nightStartHour = (uint8_t)server.arg("nm_sh").toInt();
    if (server.hasArg("nm_eh")) config.nightEndHour   = (uint8_t)server.arg("nm_eh").toInt();
    if (server.hasArg("nm_br")) config.nightBright    = (uint8_t)server.arg("nm_br").toInt();

    configSave();

    sendRedirect("/leds");
}

// ============================================================================
// 5. SISTEM & ACTIUNI ADMINISTRATIVE
// ============================================================================
static void handleSystemPage() {
    String html = getPageHeader("System", "sys");

    html += "<div class='card'>";
    html += "<h3>Firmware &amp; Hardware Information</h3>";
    html += "<div class='stat-box'><div class='stat-title'>Firmware Version</div><div class='stat-val'>0.0.1</div></div>";
    String chipInfo = String(ESP.getChipModel()) + " (Flash " + String(ESP.getFlashChipSize() / (1024 * 1024)) + "MB)";
    html += "<div class='stat-box'><div class='stat-title'>Chip Model &amp; Flash</div><div class='stat-val'>" + chipInfo + "</div></div>";
    html += "<div class='stat-box'><div class='stat-title'>WS2812B DATA Pin</div><div class='stat-val'>GPIO " + String(PIN_LED_DATA) + "</div></div>";
    html += "<div class='stat-box'><div class='stat-title'>Free Heap Memory</div><div class='stat-val'>" + String(ESP.getFreeHeap() / 1024) + " KB</div></div>";
    html += "<div class='stat-box'><div class='stat-title'>System Uptime</div><div class='stat-val'>" + String(millis() / 60000) + " minutes</div></div>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<h3>System Actions</h3>";
    html += "<form method='POST' action='/reboot' onsubmit='return confirm(\"Are you sure you want to reboot the device?\");'>";
    html += "<button class='btn btn-sec' type='submit'>Reboot Device</button>";
    html += "</form>";

    html += "<form method='POST' action='/reset_defaults' onsubmit='return confirm(\"WARNING: All settings will be reset to factory defaults! Continue?\");'>";
    html += "<button class='btn btn-danger' type='submit'>Reset to Factory Defaults</button>";
    html += "</form>";
    html += "</div>";

    html += getPageFooter();
    server.send(200, "text/html", html);
}

static void handleSyncNow() {
    ntpTriggerSync();
    ledClockTriggerConnectedAnim();
    sendRedirect("/");
}

static void handleReboot() {
    String html = getPageHeader("Rebooting...", "sys");
    html += "<div class='card'><p>The device is rebooting. Please reconnect in about 10 seconds...</p></div>";
    html += getPageFooter();
    server.send(200, "text/html", html);
    delay(500);
    ESP.restart();
}

static void handleResetDefaults() {
    configResetDefaults();
    String html = getPageHeader("Resetting...", "sys");
    html += "<div class='card'><p>Settings have been reset to factory defaults. The device is rebooting...</p></div>";
    html += getPageFooter();
    server.send(200, "text/html", html);
    delay(500);
    ESP.restart();
}

// ============================================================================
// API ENDPOINTS (JSON)
// ============================================================================
static void handleApiStatus() {
    String json = "{";
    json += "\"time\":\"" + ntpGetFormattedTime() + "\",";
    json += "\"date\":\"" + ntpGetFormattedDate() + "\",";
    json += "\"synced\":" + String(ntpIsTimeSynced() ? "true" : "false") + ",";
    json += "\"wifi_connected\":" + String(wifiIsConnected() ? "true" : "false") + ",";
    json += "\"is_ap\":" + String(wifiIsAP() ? "true" : "false") + ",";
    json += "\"ssid\":\"" + wifiGetSSID() + "\",";
    json += "\"ip\":\"" + wifiGetIP() + "\",";
    json += "\"rssi\":" + String(wifiGetRSSI()) + ",";
    json += "\"last_sync\":\"" + ntpGetLastSyncTime() + "\",";
    json += "\"next_sync\":\"" + ntpGetNextSyncTime() + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

static void handleApiLeds() {
    uint32_t colors[NUM_LEDS];
    ledClockGetRingColors(colors);

    String json = "[";
    for (int i = 0; i < NUM_LEDS; i++) {
        char buf[10];
        snprintf(buf, sizeof(buf), "\"#%06X\"", colors[i]);
        json += String(buf);
        if (i < NUM_LEDS - 1) json += ",";
    }
    json += "]";
    server.send(200, "application/json", json);
}

static void handleApiScan() {
    String ssids[15];
    int rssis[15];
    int count = wifiScanNetworks(ssids, rssis, 15);

    String json = "[";
    for (int i = 0; i < count; i++) {
        json += "{\"ssid\":\"" + ssids[i] + "\",\"rssi\":" + String(rssis[i]) + "}";
        if (i < count - 1) json += ",";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void webPortalInit() {
    server.on("/", HTTP_GET, handleDashboard);
    server.on("/lamp", HTTP_GET, handleLampPage);
    server.on("/save_lamp", HTTP_POST, handleSaveLamp);
    server.on("/wifi", HTTP_GET, handleWifiPage);
    server.on("/save_wifi", HTTP_POST, handleSaveWifi);
    server.on("/clock", HTTP_GET, handleClockPage);
    server.on("/save_clock", HTTP_POST, handleSaveClock);
    server.on("/leds", HTTP_GET, handleLedsPage);
    server.on("/save_leds", HTTP_POST, handleSaveLeds);
    server.on("/system", HTTP_GET, handleSystemPage);

    server.on("/sync_now", HTTP_POST, handleSyncNow);
    server.on("/reboot", HTTP_POST, handleReboot);
    server.on("/reset_defaults", HTTP_POST, handleResetDefaults);

    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/leds", HTTP_GET, handleApiLeds);
    server.on("/api/scan", HTTP_GET, handleApiScan);

    server.begin();
#if DEBUG_WEB
    Serial.println("[WEB] Server Web pornit cu succes pe portul 80.");
#endif
}

void webPortalLoop() {
    server.handleClient();
}
