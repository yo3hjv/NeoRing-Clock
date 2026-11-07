#include "ntp_manager.h"
#include "wifi_manager.h"
#include <sys/time.h>
#include <esp_sntp.h>

static bool isSynced = false;
static time_t lastSyncTime = 0;
static int lastSyncHourTriggered = -1;

// Slew gradual
static double slewOffset = 0.0;          // Offset-ul curent aplicat la timp
static double slewTarget = 0.0;          // Tinta spre care converge offset-ul
static double slewRatePerSec = 0.0;      // Viteza de ajustare (secunde per secunda)
static unsigned long lastSlewUpdateMs = 0;
static bool slewActive = false;

// Callback apelat nativ de SNTP cand soseste un pachet NTP valid
static void timeSyncCallback(struct timeval *tv) {
    time_t now = tv->tv_sec;
    lastSyncTime = now;

    if (!isSynced) {
        // Prima sincronizare: aplicam direct timpul, fara slew
        isSynced = true;
        slewOffset = 0.0;
        slewActive = false;
#if DEBUG_NTP
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char buf[64];
        strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &timeinfo);
        Serial.printf("\n[NTP] Prima sincronizare reusita! Ora setata: %s\n", buf);
#endif
    } else {
        // Sincronizari ulterioare: corectie graduala (slew)
        // Daca diferenta este rezonabila (< 300 secunde), facem slew gradual pe durata configurata
        // tv contine timpul deja aplicat in sistem, calculam diferenta daca vrem sa netezim
        double duration = (config.slewDurationSec > 0) ? (double)config.slewDurationSec : 60.0;
        slewRatePerSec = 0.0; // ESP32 corecteaza nativ timpul in IDF daca se foloseste adjtime, 
                              // sau putem asigura o tranzitie lina prin software
        slewActive = false;

#if DEBUG_NTP
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char buf[64];
        strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &timeinfo);
        Serial.printf("[NTP] Resincronizare periodica receptionata: %s\n", buf);
#endif
    }
}

void ntpInit() {
    isSynced = false;
    lastSyncTime = 0;
    lastSyncHourTriggered = -1;

    // Configurare SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_set_time_sync_notification_cb(timeSyncCallback);
    
    // Calcul offset UTC in secunde (human-readable)
    long gmtOffset_sec = ((long)config.tzHours * 3600L) + ((long)config.tzMinutes * 60L);
    int daylightOffset_sec = config.dstActive ? 3600 : 0;

    // Aplicare timezone si servere
    configTime(gmtOffset_sec, daylightOffset_sec, config.ntpServer1, config.ntpServer2, config.ntpServer3);

#if DEBUG_NTP
    Serial.printf("[NTP] Initializat. Servere: 1='%s', 2='%s', 3='%s'\n",
                  config.ntpServer1, config.ntpServer2, config.ntpServer3);
    Serial.printf("[NTP] Fus orar: UTC%+d:%02d (DST / Ora de vara: %s)\n",
                  config.tzHours, abs(config.tzMinutes), config.dstActive ? "Activa (+1h)" : "Inactiva");
#endif
}

void ntpTriggerSync() {
#if DEBUG_NTP
    Serial.println("[NTP] Sincronizare manuala declansata la cerere...");
#endif
    sntp_stop();
    long gmtOffset_sec = ((long)config.tzHours * 3600L) + ((long)config.tzMinutes * 60L);
    int daylightOffset_sec = config.dstActive ? 3600 : 0;
    configTime(gmtOffset_sec, daylightOffset_sec, config.ntpServer1, config.ntpServer2, config.ntpServer3);
}

void ntpLoop() {
    static unsigned long lastWifiCheck = 0;
    unsigned long nowMs = millis();

    // Verificam la fiecare 5 secunde daca avem conexiune si SNTP ruleaza
    if (nowMs - lastWifiCheck >= 5000) {
        lastWifiCheck = nowMs;
        if (wifiIsConnected() && !isSynced) {
            // Daca suntem conectati dar inca nu avem ora, fortam o actualizare
            time_t now = time(nullptr);
            if (now > 1672531200) { // Daca timpul este dupa 01.01.2023, inseamna ca s-a sincronizat
                isSynced = true;
                lastSyncTime = now;
#if DEBUG_NTP
                Serial.println("[NTP] Sincronizare confirmata prin timpul de sistem.");
#endif
            }
        }
    }

    // Verificare sincronizare programata la 12 ore (03:00 AM si 15:00 / 03:00 PM)
    if (isSynced && wifiIsConnected()) {
        time_t now = time(nullptr);
        struct tm tmInfo;
        localtime_r(&now, &tmInfo);

        if ((tmInfo.tm_hour == NTP_SYNC_HOUR_AM || tmInfo.tm_hour == NTP_SYNC_HOUR_PM) && tmInfo.tm_min == 0) {
            if (lastSyncHourTriggered != tmInfo.tm_hour) {
                lastSyncHourTriggered = tmInfo.tm_hour;
#if DEBUG_NTP
                Serial.printf("[NTP] [Scheduler 12h] Trigger sincronizare la ora fixa: %02d:00\n", tmInfo.tm_hour);
#endif
                ntpTriggerSync();
            }
        } else {
            if (lastSyncHourTriggered != -1 && tmInfo.tm_min > 0) {
                lastSyncHourTriggered = -1; // Reset trigger pentru urmatorul interval
            }
        }
    }
}

bool ntpIsTimeSynced() {
    return isSynced;
}

int ntpGetCurrentHour() {
    time_t now = time(nullptr);
    struct tm tmInfo;
    localtime_r(&now, &tmInfo);
    return tmInfo.tm_hour;
}

String ntpGetFormattedTime() {
    if (!isSynced) return "--:--:--";
    time_t now = time(nullptr);
    struct tm tmInfo;
    localtime_r(&now, &tmInfo);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);
    return String(buf);
}

String ntpGetFormattedDate() {
    if (!isSynced) return "--.--.----";
    time_t now = time(nullptr);
    struct tm tmInfo;
    localtime_r(&now, &tmInfo);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d", tmInfo.tm_mday, tmInfo.tm_mon + 1, tmInfo.tm_year + 1900);
    return String(buf);
}

String ntpGetLastSyncTime() {
    if (lastSyncTime == 0) return "Niciodata";
    struct tm tmInfo;
    localtime_r(&lastSyncTime, &tmInfo);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d:%02d",
             tmInfo.tm_mday, tmInfo.tm_mon + 1, tmInfo.tm_year + 1900,
             tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);
    return String(buf);
}

String ntpGetNextSyncTime() {
    if (!isSynced) return "Dupa conectare Wi-Fi";
    time_t now = time(nullptr);
    struct tm tmInfo;
    localtime_r(&now, &tmInfo);

    int nextHour = (tmInfo.tm_hour < NTP_SYNC_HOUR_AM) ? NTP_SYNC_HOUR_AM :
                   (tmInfo.tm_hour < NTP_SYNC_HOUR_PM) ? NTP_SYNC_HOUR_PM : NTP_SYNC_HOUR_AM;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:00:00", nextHour);
    return String(buf);
}

void ntpGetClockPositions(float &hourPos, float &minPos, float &secPos, bool smoothH, bool smoothM, bool smoothS) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t sec = tv.tv_sec;
    struct tm tmInfo;
    localtime_r(&sec, &tmInfo);

    float secFraction = (float)tv.tv_usec / 1000000.0f;

    // 1. Secunda: fluida sau salt discret
    if (smoothS) {
        secPos = (float)tmInfo.tm_sec + secFraction;
    } else {
        secPos = (float)tmInfo.tm_sec;
    }

    // 2. Minutul: fluid sau discret
    if (smoothM) {
        float sForM = (float)tmInfo.tm_sec + secFraction;
        minPos = (float)tmInfo.tm_min + (sForM / 60.0f);
    } else {
        minPos = (float)tmInfo.tm_min;
    }

    // 3. Ora: fluida (progresiva dupa minute) sau fixa pe ora intreaga
    if (smoothH) {
        float mForH = (float)tmInfo.tm_min + (((float)tmInfo.tm_sec + secFraction) / 60.0f);
        hourPos = (((float)(tmInfo.tm_hour % 12)) + (mForH / 60.0f)) * 5.0f;
    } else {
        hourPos = ((float)(tmInfo.tm_hour % 12)) * 5.0f;
    }

    // Normalizare in intervalul [0.0 .. 60.0)
    while (secPos >= 60.0f) secPos -= 60.0f;
    while (minPos >= 60.0f) minPos -= 60.0f;
    while (hourPos >= 60.0f) hourPos -= 60.0f;
    while (secPos < 0.0f) secPos += 60.0f;
    while (minPos < 0.0f) minPos += 60.0f;
    while (hourPos < 0.0f) hourPos += 60.0f;
}
