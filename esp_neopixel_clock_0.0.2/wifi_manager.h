#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "setup.h"
#include "config_manager.h"

enum WifiState {
    WIFI_STATE_IDLE,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE
};

void wifiInit();
void wifiLoop();

bool wifiIsConnected();
bool wifiIsAP();
String wifiGetIP();
String wifiGetSSID();
int wifiGetRSSI();

void wifiTriggerReconnect();
int wifiScanNetworks(String ssids[], int rssis[], int maxCount);

