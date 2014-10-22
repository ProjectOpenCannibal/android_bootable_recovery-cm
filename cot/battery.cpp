/* Copyright (C) 2014 Project Open Cannibal
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "../bootloader.h"
#include "../common.h"
#include "cutils/properties.h"
#include "cutils/android_reboot.h"
#include "../install.h"
#include "../minui/minui.h"
#include "../minzip/DirUtil.h"
#include "../roots.h"
#include "../ui.h"
#include "../screen_ui.h"
#include "../device.h"
#include "../flashutils/flashutils.h"

#include "cutils/properties.h"

#include "../voldclient/voldclient.h"

#include "includes.h"
#include "external.h"

#ifndef BOARD_BATTERY_CAP_FILE
#define BOARD_BATTERY_CAP_FILE "/sys/class/power_supply/battery/capacity"
#endif

extern RecoveryUI* ui;

int COTBattery::LastBatteryLevel;

void COTBattery::SetBatteryLevel() {
    static int level = -1;
    
    char value[4];
    FILE * capacity = fopen(BOARD_BATTERY_CAP_FILE,  "rt");
    if (capacity) {
        fgets(value,  4,  capacity);
        fclose(capacity);
        level = atoi(value);
        
        /* normalize levels */
        if (level > 100) {
            level = 100;
        }
        if (level < 0) {
            level = 0;
        }
        
        /* set COTTheme::BatteryLevel depending on percentage gap */
        if (level == 100) {
            COTTheme::BatteryLevel = "bat_100";
        }
        if (level > 89 && level != 100) {
            COTTheme::BatteryLevel = "bat_90";
        }
        if (level > 79 && level < 90) {
            COTTheme::BatteryLevel = "bat_80";
        }
        if (level > 69 && level < 80) {
            COTTheme::BatteryLevel = "bat_70";
        }
        if (level > 59 && level < 70) {
            COTTheme::BatteryLevel = "bat_60";
        }
        if (level > 49 && level < 60) {
            COTTheme::BatteryLevel = "bat_50";
        }
        if (level > 39 && level < 50) {
            COTTheme::BatteryLevel = "bat_40";
        }
        if (level > 29 && level < 40) {
            COTTheme::BatteryLevel = "bat_30";
        }
        if (level > 19 && level < 30) {
            COTTheme::BatteryLevel = "bat_20";
        }
        if (level > 9 && level < 20) {
            COTTheme::BatteryLevel = "bat_10";
        }
        if (level > 1 && level < 10) {
            COTTheme::BatteryLevel = "bat_0";
        }
    }
    return;
}