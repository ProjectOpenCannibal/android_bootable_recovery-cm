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

#define THEME_CHOICE_RED 0
#define THEME_CHOICE_BLUE 1

extern RecoveryUI* ui;
extern ScreenRecoveryUI* screen;

const char* COTTheme::theme_path = "bloodred/";

void COTTheme::ShowMainMenu(Device* device) {
    static const char* ThemeChooserHeaders[] = { "Choose Theme",
        "",
        NULL
    };

    static const char* ThemeChooserItems[] = { "Blood Red (default)",
		"Hydro Blue",
        NULL
    };

    for (;;) {
        int ThemeChoice = get_menu_selection(ThemeChooserHeaders, ThemeChooserItems, 0, 0, device);
        switch (ThemeChoice) {
            case THEME_CHOICE_RED:
				COTTheme::theme_path = "bloodred/";
				printf("COTTheme: set path to %s\n", COTTheme::theme_path);
				break;
				
            case THEME_CHOICE_BLUE:
				COTTheme::theme_path = "hydro/";
				printf("COTTheme: set path to %s\n", COTTheme::theme_path);
				break;

			case Device::kGoBack:
				return;
        }
    }
    screen->Redraw();
}

