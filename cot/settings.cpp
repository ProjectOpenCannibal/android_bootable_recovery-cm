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

bool COTSettings::TestINI(Device* device) {
	dictionary * ini;
	char * ini_file = "/sdcard/test.ini";
	
	ini = iniparser_load(ini_file);
	if (ini == NULL) {
		ui->Print("Can't load /sdcard/test.ini!\n");
		return false;
	}
	char * testentry = iniparser_getstring(ini, "test:string", NULL);
	ui->Print(testentry);
	return true;
}

void COTSettings::ShowMainMenu(Device* device) {
    static const char* SettingsMenuHeaders[] = { "Settings",
        "",
        NULL
    };

    static const char* SettingsMenuItems[] = { "Nothing here yet",
        NULL
    };
    
    #define ZIP_OPTIONS 0

    for (;;) {
        int SettingsSelection = get_menu_selection(SettingsMenuHeaders, SettingsMenuItems, 0, 0, device);
        switch (SettingsSelection) {
            case ZIP_OPTIONS:
                //COTPackage::ShowZipOptionsMenu(device);
                if (COTSettings::TestINI(device)) {
					ui->Print("true\n");
				} else {
					ui->Print("false\n");
				}
                break;

            case Device::kGoBack:
                return;
        }
    }
}
