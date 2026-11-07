#pragma once
#include <Arduino.h>
#include <time.h>
#include "setup.h"
#include "config_manager.h"

void ntpInit();
void ntpLoop();
void ntpTriggerSync();

bool ntpIsTimeSynced();
String ntpGetFormattedTime();
String ntpGetFormattedDate();
String ntpGetLastSyncTime();
String ntpGetNextSyncTime();

int ntpGetCurrentHour();

// Obtine pozitiile (0.00 .. 59.99) pentru indicatoarele de ora, minut, secunda
// Cu suport granular pentru tranzitie fluida per indicator
void ntpGetClockPositions(float &hourPos, float &minPos, float &secPos, bool smoothH, bool smoothM, bool smoothS);

