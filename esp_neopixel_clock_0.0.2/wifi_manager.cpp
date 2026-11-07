#include "wifi_manager.h"
#include <ESPmDNS.h>

static WifiState currentState = WIFI_STATE_IDLE;
static int currentProfileIndex = -1;
static unsigned long connectStartTime = 0;
static const unsigned long CONNECT_TIMEOUT_MS = 12000; // 12 secunde per profil
static unsigned long lastApRetryTime = 0;
static const unsigned long AP_RETRY_INTERVAL_MS = 90000; // La fiecare 90 secunde se reincearca conectarea

// Lista de indecsi ordonati dupa prioritate
static int sortedIndices[3] = {0, 1, 2};
static int activeProfileCount = 0;
static int currentTryStep = 0;

static void prepareSortedProfiles() {
    activeProfileCount = 0;
    for (int i = 0; i < 3; i++) {
        if (config.wifi[i].enabled && strlen(config.wifi[i].ssid) > 0) {
            sortedIndices[activeProfileCount++] = i;
        }
    }

    // Sortare dupa prioritate (1 = cea mai mare prioritate)
    for (int i = 0; i < activeProfileCount - 1; i++) {
        for (int j = i + 1; j < activeProfileCount; j++) {
            if (config.wifi[sortedIndices[j]].priority < config.wifi[sortedIndices[i]].priority) {
                int temp = sortedIndices[i];
                sortedIndices[i] = sortedIndices[j];
                sortedIndices[j] = temp;
            }
        }
    }
}

static void startApMode() {
    WiFi.disconnect(false);
    WiFi.mode(WIFI_AP_STA); // AP_STA permite si reincercari viitoare de scanare / conectare
    WiFi.softAPConfig(DEFAULT_AP_IP, DEFAULT_AP_GATEWAY, DEFAULT_AP_SUBNET);
    const char* apPass = (strlen(DEFAULT_AP_PASS) >= 8) ? DEFAULT_AP_PASS : nullptr;
    WiFi.softAP(DEFAULT_AP_SSID, apPass);

    currentState = WIFI_STATE_AP_MODE;
    lastApRetryTime = millis();

    // Activare mDNS si pentru modul AP
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
    }

#if DEBUG_WIFI
    Serial.println("\n[WIFI] ----------------------------------------");
    Serial.println("[WIFI] Toate incercarile de conectare au esuat.");
    Serial.printf("[WIFI] Pornit Access Point: SSID='%s' (Fara parola)\n", DEFAULT_AP_SSID);
    Serial.print("[WIFI] IP Access Point: http://");
    Serial.println(WiFi.softAPIP());
    Serial.println("[WIFI] ----------------------------------------\n");
#endif
}

static void tryNextProfile() {
    if (currentTryStep < activeProfileCount) {
        currentProfileIndex = sortedIndices[currentTryStep];
        const char* ssid = config.wifi[currentProfileIndex].ssid;
        const char* pass = config.wifi[currentProfileIndex].pass;

#if DEBUG_WIFI
        Serial.printf("[WIFI] Incercare conectare la profilul %d (prioritate %d): '%s'...\n",
                      currentProfileIndex + 1, config.wifi[currentProfileIndex].priority, ssid);
#endif

        WiFi.disconnect(false);
        WiFi.begin(ssid, pass);
        connectStartTime = millis();
        currentState = WIFI_STATE_CONNECTING;
        currentTryStep++;
    } else {
        // Au fost incercate toate profilurile si au esuat
        startApMode();
    }
}

void wifiInit() {
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_STA);

    prepareSortedProfiles();

    if (activeProfileCount == 0) {
#if DEBUG_WIFI
        Serial.println("[WIFI] Niciun profil Wi-Fi configurat. Pornire directa in AP Mode...");
#endif
        startApMode();
    } else {
        currentTryStep = 0;
        tryNextProfile();
    }
}

void wifiTriggerReconnect() {
    prepareSortedProfiles();
    if (activeProfileCount == 0) {
        startApMode();
    } else {
        currentTryStep = 0;
        tryNextProfile();
    }
}

void wifiLoop() {
    unsigned long now = millis();

    switch (currentState) {
        case WIFI_STATE_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                currentState = WIFI_STATE_CONNECTED;
                // Oprim AP daca era activat
                if (WiFi.getMode() != WIFI_STA) {
                    WiFi.mode(WIFI_STA);
                }
#if DEBUG_WIFI
                Serial.println("\n[WIFI] ========================================");
                Serial.printf("[WIFI] Conectat cu succes la: %s\n", config.wifi[currentProfileIndex].ssid);
                Serial.print("[WIFI] Adresa IP primita (DHCP): http://");
                Serial.println(WiFi.localIP());
                Serial.printf("[WIFI] Putere semnal (RSSI): %d dBm\n", WiFi.RSSI());

                // Activare mDNS (http://clock.local)
                if (MDNS.begin(MDNS_HOSTNAME)) {
                    MDNS.addService("http", "tcp", 80);
                    Serial.printf("[mDNS] Responder activat: http://%s.local\n", MDNS_HOSTNAME);
                }

                Serial.println("[WIFI] ========================================\n");
#endif
            } else if (now - connectStartTime >= CONNECT_TIMEOUT_MS) {
#if DEBUG_WIFI
                Serial.printf("[WIFI] Timeout conectare profil %d.\n", currentProfileIndex + 1);
#endif
                tryNextProfile();
            }
            break;

        case WIFI_STATE_CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
#if DEBUG_WIFI
                Serial.println("[WIFI] Conexiunea Wi-Fi a fost pierduta! Se reincearca reconectarea...");
#endif
                currentTryStep = 0;
                tryNextProfile();
            }
            break;

        case WIFI_STATE_AP_MODE:
            // Periodic verificam daca retelele salvate redevin disponibile
            if (activeProfileCount > 0 && (now - lastApRetryTime >= AP_RETRY_INTERVAL_MS)) {
                lastApRetryTime = now;
#if DEBUG_WIFI
                Serial.println("[WIFI] [AP-Background] Reincercare periodica de conectare la retelele salvate...");
#endif
                currentTryStep = 0;
                tryNextProfile();
            }
            break;

        default:
            break;
    }
}

bool wifiIsConnected() {
    return (currentState == WIFI_STATE_CONNECTED) && (WiFi.status() == WL_CONNECTED);
}

bool wifiIsAP() {
    return (currentState == WIFI_STATE_AP_MODE);
}

String wifiGetIP() {
    if (wifiIsConnected()) {
        return WiFi.localIP().toString();
    } else if (wifiIsAP()) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

String wifiGetSSID() {
    if (wifiIsConnected() && currentProfileIndex >= 0) {
        return String(config.wifi[currentProfileIndex].ssid);
    } else if (wifiIsAP()) {
        return String(DEFAULT_AP_SSID);
    }
    return "-";
}

int wifiGetRSSI() {
    if (wifiIsConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

int wifiScanNetworks(String ssids[], int rssis[], int maxCount) {
    int n = WiFi.scanNetworks(false, false);
    if (n <= 0) return 0;
    int count = (n < maxCount) ? n : maxCount;
    for (int i = 0; i < count; i++) {
        ssids[i] = WiFi.SSID(i);
        rssis[i] = WiFi.RSSI(i);
    }
    WiFi.scanDelete();
    return count;
}
