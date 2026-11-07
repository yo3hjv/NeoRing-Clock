#include "config_manager.h"
#include <Preferences.h>

AppConfig config;
static Preferences prefs;

static void setDefaults() {
    memset(&config, 0, sizeof(AppConfig));

    // Profile Wi-Fi goale implicit
    for (int i = 0; i < 3; i++) {
        config.wifi[i].ssid[0] = '\0';
        config.wifi[i].pass[0] = '\0';
        config.wifi[i].enabled = (i == 0); // doar profilul 1 e activ implicit
        config.wifi[i].priority = i + 1;
    }

    // Culori si luminozitati din setup.h
    config.colorHour        = DEFAULT_HOUR_COLOR;
    config.brightHour       = DEFAULT_HOUR_BRIGHTNESS;
    config.hourWidth3       = DEFAULT_HOUR_WIDTH_3;
    config.colorHourGuard   = DEFAULT_HOUR_GUARD_COLOR;
    config.brightHourGuard  = DEFAULT_HOUR_GUARD_BRIGHT;

    config.colorMin         = DEFAULT_MIN_COLOR;
    config.brightMin        = DEFAULT_MIN_BRIGHTNESS;
    config.minWidth3        = DEFAULT_MIN_WIDTH_3;
    config.colorMinGuard    = DEFAULT_MIN_GUARD_COLOR;
    config.brightMinGuard   = DEFAULT_MIN_GUARD_BRIGHT;

    config.colorSec         = DEFAULT_SEC_COLOR;
    config.brightSec        = DEFAULT_SEC_BRIGHTNESS;

    config.globalBright     = DEFAULT_GLOBAL_BRIGHT;
    config.showSeconds      = DEFAULT_SHOW_SECONDS;
    config.smoothHour       = DEFAULT_SMOOTH_HOUR;
    config.smoothMin        = DEFAULT_SMOOTH_MIN;
    config.smoothSec        = DEFAULT_SMOOTH_SEC;
    config.overlapMode      = DEFAULT_OVERLAP_MODE;
    config.guardCutoffPct   = DEFAULT_GUARD_CUTOFF_PCT;

    config.lampEnabled      = DEFAULT_LAMP_ENABLED;
    config.lampEffect       = DEFAULT_LAMP_EFFECT;
    config.lampColor        = DEFAULT_LAMP_COLOR;
    config.lampBrightness   = DEFAULT_LAMP_BRIGHTNESS;
    config.lampSpeed        = DEFAULT_LAMP_SPEED;

    config.dialOffset       = 0;
    config.clockwise        = CLOCK_IS_CLOCKWISE;

    config.nightModeEnabled = false;
    config.nightStartHour   = 23;
    config.nightEndHour     = 7;
    config.nightBright      = 5;

    strncpy(config.ntpServer1, DEFAULT_NTP_SERVER1, sizeof(config.ntpServer1) - 1);
    strncpy(config.ntpServer2, DEFAULT_NTP_SERVER2, sizeof(config.ntpServer2) - 1);
    strncpy(config.ntpServer3, DEFAULT_NTP_SERVER3, sizeof(config.ntpServer3) - 1);
    config.tzHours          = DEFAULT_TZ_HOURS;
    config.tzMinutes        = DEFAULT_TZ_MINUTES;
    config.dstActive        = DEFAULT_DST_ACTIVE;
    config.slewDurationSec  = DEFAULT_SLEW_SECONDS;
}

void configInit() {
    setDefaults();
    configLoad();
}

void configLoad() {
    if (!prefs.begin("clock_cfg", true)) { // Read-only
#if DEBUG_POST
        Serial.println("[CFG] NVS namespace 'clock_cfg' nu a putut fi deschis. Se folosesc valorile implicite.");
#endif
        return;
    }

    // Verificam daca exista o configuratie salvata
    if (prefs.isKey("cfg_init")) {
        for (int i = 0; i < 3; i++) {
            String prefix = "w" + String(i) + "_";
            String ssid = prefs.getString((prefix + "s").c_str(), "");
            String pass = prefs.getString((prefix + "p").c_str(), "");
            strncpy(config.wifi[i].ssid, ssid.c_str(), sizeof(config.wifi[i].ssid) - 1);
            strncpy(config.wifi[i].pass, pass.c_str(), sizeof(config.wifi[i].pass) - 1);
            config.wifi[i].enabled = prefs.getBool((prefix + "e").c_str(), i == 0);
            config.wifi[i].priority = prefs.getUChar((prefix + "pr").c_str(), i + 1);
        }

        config.colorHour        = prefs.getUInt("c_hour", DEFAULT_HOUR_COLOR);
        config.brightHour       = prefs.getUChar("b_hour", DEFAULT_HOUR_BRIGHTNESS);
        config.hourWidth3       = prefs.getBool("w3_h", DEFAULT_HOUR_WIDTH_3);
        config.colorHourGuard   = prefs.getUInt("cg_h", DEFAULT_HOUR_GUARD_COLOR);
        config.brightHourGuard  = prefs.getUChar("bg_h", DEFAULT_HOUR_GUARD_BRIGHT);

        config.colorMin         = prefs.getUInt("c_min", DEFAULT_MIN_COLOR);
        config.brightMin        = prefs.getUChar("b_min", DEFAULT_MIN_BRIGHTNESS);
        config.minWidth3        = prefs.getBool("w3_m", DEFAULT_MIN_WIDTH_3);
        config.colorMinGuard    = prefs.getUInt("cg_m", DEFAULT_MIN_GUARD_COLOR);
        config.brightMinGuard   = prefs.getUChar("bg_m", DEFAULT_MIN_GUARD_BRIGHT);

        config.colorSec         = prefs.getUInt("c_sec", DEFAULT_SEC_COLOR);
        config.brightSec        = prefs.getUChar("b_sec", DEFAULT_SEC_BRIGHTNESS);

        config.globalBright     = prefs.getUChar("b_glob", DEFAULT_GLOBAL_BRIGHT);
        config.showSeconds      = prefs.getBool("s_sec", DEFAULT_SHOW_SECONDS);
        config.smoothHour       = prefs.getBool("s_sm_h", DEFAULT_SMOOTH_HOUR);
        config.smoothMin        = prefs.getBool("s_sm_m", DEFAULT_SMOOTH_MIN);
        config.smoothSec        = prefs.getBool("s_sm_s", DEFAULT_SMOOTH_SEC);
        if (prefs.isKey("s_ov_m")) {
            config.overlapMode  = prefs.getUChar("s_ov_m", DEFAULT_OVERLAP_MODE);
        } else if (prefs.isKey("s_over")) {
            config.overlapMode  = prefs.getBool("s_over", true) ? OVERLAP_MODE_ADDITIVE : OVERLAP_MODE_HOUR_MIN_SEC;
        } else {
            config.overlapMode  = DEFAULT_OVERLAP_MODE;
        }
        config.guardCutoffPct   = prefs.getUChar("g_cut", DEFAULT_GUARD_CUTOFF_PCT);

        config.lampEnabled      = prefs.getBool("lp_en", DEFAULT_LAMP_ENABLED);
        config.lampEffect       = prefs.getUChar("lp_eff", DEFAULT_LAMP_EFFECT);
        config.lampColor        = prefs.getUInt("lp_col", DEFAULT_LAMP_COLOR);
        config.lampBrightness   = prefs.getUChar("lp_br", DEFAULT_LAMP_BRIGHTNESS);
        config.lampSpeed        = prefs.getUChar("lp_spd", DEFAULT_LAMP_SPEED);

        config.dialOffset       = (int8_t)prefs.getChar("d_off", 0);
        config.clockwise        = prefs.getBool("d_cw", CLOCK_IS_CLOCKWISE);

        config.nightModeEnabled = prefs.getBool("nm_en", false);
        config.nightStartHour   = prefs.getUChar("nm_sh", 23);
        config.nightEndHour     = prefs.getUChar("nm_eh", 7);
        config.nightBright      = prefs.getUChar("nm_br", 5);

        String s1 = prefs.getString("ntp_s1", DEFAULT_NTP_SERVER1);
        String s2 = prefs.getString("ntp_s2", DEFAULT_NTP_SERVER2);
        String s3 = prefs.getString("ntp_s3", DEFAULT_NTP_SERVER3);
        strncpy(config.ntpServer1, s1.c_str(), sizeof(config.ntpServer1) - 1);
        strncpy(config.ntpServer2, s2.c_str(), sizeof(config.ntpServer2) - 1);
        strncpy(config.ntpServer3, s3.c_str(), sizeof(config.ntpServer3) - 1);
        config.tzHours          = (int8_t)prefs.getChar("tz_h", DEFAULT_TZ_HOURS);
        config.tzMinutes        = (int8_t)prefs.getChar("tz_m", DEFAULT_TZ_MINUTES);
        config.dstActive        = prefs.getBool("tz_dst", DEFAULT_DST_ACTIVE);
        config.slewDurationSec  = prefs.getUShort("ntp_slew", DEFAULT_SLEW_SECONDS);

#if DEBUG_POST
        Serial.println("[CFG] Configuratie incarcata cu succes din NVS.");
#endif
    } else {
#if DEBUG_POST
        Serial.println("[CFG] Prima pornire: salvare valori implicite in NVS...");
#endif
        prefs.end();
        configSave();
        return;
    }

    prefs.end();
}

void configSave() {
    if (!prefs.begin("clock_cfg", false)) { // Read-Write
#if DEBUG_POST
        Serial.println("[CFG] EROARE la deschiderea NVS pentru scriere!");
#endif
        return;
    }

    prefs.putBool("cfg_init", true);

    for (int i = 0; i < 3; i++) {
        String prefix = "w" + String(i) + "_";
        prefs.putString((prefix + "s").c_str(), config.wifi[i].ssid);
        prefs.putString((prefix + "p").c_str(), config.wifi[i].pass);
        prefs.putBool((prefix + "e").c_str(), config.wifi[i].enabled);
        prefs.putUChar((prefix + "pr").c_str(), config.wifi[i].priority);
    }

    prefs.putUInt("c_hour", config.colorHour);
    prefs.putUChar("b_hour", config.brightHour);
    prefs.putBool("w3_h", config.hourWidth3);
    prefs.putUInt("cg_h", config.colorHourGuard);
    prefs.putUChar("bg_h", config.brightHourGuard);

    prefs.putUInt("c_min", config.colorMin);
    prefs.putUChar("b_min", config.brightMin);
    prefs.putBool("w3_m", config.minWidth3);
    prefs.putUInt("cg_m", config.colorMinGuard);
    prefs.putUChar("bg_m", config.brightMinGuard);

    prefs.putUInt("c_sec", config.colorSec);
    prefs.putUChar("b_sec", config.brightSec);

    prefs.putUChar("b_glob", config.globalBright);
    prefs.putBool("s_sec", config.showSeconds);
    prefs.putBool("s_sm_h", config.smoothHour);
    prefs.putBool("s_sm_m", config.smoothMin);
    prefs.putBool("s_sm_s", config.smoothSec);
    prefs.putUChar("s_ov_m", config.overlapMode);
    prefs.putUChar("g_cut", config.guardCutoffPct);

    prefs.putBool("lp_en", config.lampEnabled);
    prefs.putUChar("lp_eff", config.lampEffect);
    prefs.putUInt("lp_col", config.lampColor);
    prefs.putUChar("lp_br", config.lampBrightness);
    prefs.putUChar("lp_spd", config.lampSpeed);

    prefs.putChar("d_off", (int8_t)config.dialOffset);
    prefs.putBool("d_cw", config.clockwise);

    prefs.putBool("nm_en", config.nightModeEnabled);
    prefs.putUChar("nm_sh", config.nightStartHour);
    prefs.putUChar("nm_eh", config.nightEndHour);
    prefs.putUChar("nm_br", config.nightBright);

    prefs.putString("ntp_s1", config.ntpServer1);
    prefs.putString("ntp_s2", config.ntpServer2);
    prefs.putString("ntp_s3", config.ntpServer3);
    prefs.putChar("tz_h", (int8_t)config.tzHours);
    prefs.putChar("tz_m", (int8_t)config.tzMinutes);
    prefs.putBool("tz_dst", config.dstActive);
    prefs.putUShort("ntp_slew", config.slewDurationSec);

    prefs.end();
#if DEBUG_POST
    Serial.println("[CFG] Configuratie salvata persistent in NVS.");
#endif
}

void configResetDefaults() {
    setDefaults();
    configSave();
}

