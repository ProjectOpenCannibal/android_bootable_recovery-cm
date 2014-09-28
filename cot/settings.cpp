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
#include <unistd.h>

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

#include "cutils/properties.h"

#include "../voldclient/voldclient.h"

#include "includes.h"
#include "external.h"

extern RecoveryUI* ui;
extern ScreenRecoveryUI* screen;

// Keep the settings dictionary in memory so we can access it anywhere
dictionary * COTSettings::settingsini;

void COTSettings::CreateOrSaveSettings(int is_new) {
	if (is_new == 1) {
		LOGE("Creating new settings file...\n");
		FILE * ini;
		ini = fopen_path("/sdcard/cot/settings.ini", "w");
		fprintf(ini,
			"; COT Settings INI\n"
			";\n"
			"\n"
			"[settings]\n"
			"theme = default\n"
			"\n");
		fclose(ini);
		LOGE("Settings created!\n");
	} else {
		FILE * ini;
		ini = fopen_path("/sdcard/cot/settings.ini", "w");
		char* theme_name = iniparser_getstring(COTTheme::themeini, "theme:name", NULL);
		iniparser_set(COTSettings::settingsini, "settings", NULL);
		iniparser_set(COTSettings::settingsini, "settings:theme", COTTheme::current_theme);
		iniparser_dump_ini(COTSettings::settingsini, ini);
		fclose(ini);
		LOGE("Settings updated!\n");
	}
	return;
}

void COTSettings::LoadSettings() {
	ensure_path_mounted("/data/media");
	ensure_path_mounted("/sdcard");
	char * ini_file = "/sdcard/cot/settings.ini";
	
	COTSettings::settingsini = iniparser_load(ini_file);
	if (COTSettings::settingsini == NULL) {
		LOGE("Can't load /sdcard/cot/settings.ini, creating...\n");
		COTSettings::CreateOrSaveSettings(1);
		COTSettings::settingsini = iniparser_load("/sdcard/cot/settings.ini");
	}
	
	char * theme_name = iniparser_getstring(COTSettings::settingsini, "settings:theme", NULL);
	COTTheme::current_theme = iniparser_getstring(COTSettings::settingsini, "settings:theme", NULL);
	COTTheme::LoadTheme(theme_name);
	
	ui->ResetIcons();
}

void COTSettings::ShowMainMenu(Device* device) {
    static const char* SettingsMenuHeaders[] = { "Settings",
        "",
        NULL
    };

    static const char* SettingsMenuItems[] = { "Theme",
        NULL
    };
    
    #define THEME_OPTIONS 0

    for (;;) {
        int SettingsSelection = get_menu_selection(SettingsMenuHeaders, SettingsMenuItems, 0, 0, device);
        switch (SettingsSelection) {
            case THEME_OPTIONS:
                //COTPackage::ShowZipOptionsMenu(device);
                COTTheme::ChooseThemeMenu(device);
                break;
            case Device::kGoBack:
                return;
        }
    }
}
